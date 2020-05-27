"""



    {
        'error': '',
        'function': f'replace_children',
        'arguments' : ['12345', ['<table> ...</table>'], ...],
    }



Example error response server->client::

    {
        'response': 'False',
        'error': f'No file of id={file_id}',
        'action': f'add_file',
    }


"""
import json
import logging
import pprint
import time
import typing

import svfs
from TotalDepth.RP66V1.core.LogicalRecord import EFLR

from src.cpy import common, connection
from TotalDepth.RP66V1.core import File

logger = logging.getLogger(__file__)


STORAGE_UNIT_LABEL_LENGTH = 80
VISIBLE_RECORD_HEADER_LENGTH = 4
LRSH_LENGTH = 4


class LRPosDesc(typing.NamedTuple):
    vr_position: int  # Position of the Visible Record immediately preceding this logical record.
    vr_length: int  # Length of the Visible Record immediately preceding this logical record.
    lr_position: int  # Position of the first LRSH of the logical record.
    lr_attributes: File.LogicalRecordSegmentHeaderAttributes  # Attributes of the first LRSH.
    lr_type: int  # Type of the Logical record from the first LRSH. 0 <= lr_type < 256
    # File position of the last byte of the complete Logical Record. The SVF has to contain contiguous data from
    # lr_position <= tell() < lr_position_end to read the entirety of the Logical Record.
    lr_position_end: int

    @property
    def lr_length(self) -> int:
        return self.lr_position_end - self.lr_position

    @property
    def vr_position_end(self) -> int:
        return self.vr_position + self.vr_length

    def __str__(self) -> str:
        return f'LRPosDesc VR: 0x{self.vr_position:08x} LRSH: 0x{self.lr_position:08x} {self.lr_attributes!s}' \
            f' type: {self.lr_type:3d} end: 0x{self.lr_position_end:08x}' \
            f' len: 0x{self.lr_position_end - self.lr_position:08x}'


class Server:
    # TODO: Create a TotalDepth.RP66V1.core.pIndex.LogicalRecordIndex from this information.
    # TODO: Read specific EFLR at file position.
    # TODO: Read set of IFLRs

    LOGGER_PREFIX = 'SERVER'

    def __init__(self):
        logger.info(f'{self.LOGGER_PREFIX}: __init__()')
        self.svfs = svfs.SVFS()
        # Dict of {file_id, {LRSH_position: LRPosDesc, ...}, ...}
        self.low_level_index: typing.Dict[str, typing.Dict[int, LRPosDesc]] = {}
        self._function_map = {
            'add_file': self.add_file,
            'add_data': self.add_data,
        }

    def from_client(self, json_data: str) -> str:
        comms = common.CommunicationJSON.json_reads(json_data)
        ret = common.CommunicationJSON()
        for function_name, args in comms.gen_call():
            function = self._function_map[function_name]
            result = function(*args)
            if result:
                ret.add_call(*result)
        return ret.json_dumps()

    def add_file(self, file_id: str, mod_time: float,
                 seek_read: typing.List[typing.Tuple[int, str]]) -> typing.List[typing.Any]:
        """Add a file and return JSON encoded seek/read list."""
        logger.info(f'{self.LOGGER_PREFIX}: add_file() {file_id}')
        timer = common.Timer()
        if not self.svfs.has(file_id):
            self.svfs.insert(file_id, mod_time)
        self.add_data(file_id, mod_time, seek_read)
        logger.info(
            f'{self.LOGGER_PREFIX}: add_file() has {self.svfs.num_bytes(file_id)} bytes'
            f' {self.svfs.num_blocks(file_id)} blocks'
            f' size_of: {self.svfs.size_of(file_id)} bytes.'
        )
        logger.info(
            f'{self.LOGGER_PREFIX}: add_file() SVFS total {self.svfs.total_bytes()} bytes'
            f' total_size_of {self.svfs.total_size_of()} bytes.'
        )
        logger.info(f'{self.LOGGER_PREFIX}: added file took {timer.ms():.3f} (ms).')
        timer = common.Timer()

        # for lr_position_description in self._iterate_logical_record_positions(file_id):
        #     logger.info(f'SERVER: {lr_position_description}')

        eflr_count = 0
        for e, eflr_position_description in enumerate(self._iterate_EFLR(file_id)):
            # logger.info(f'SERVER: EFLR [{e:6d}] {eflr_position_description}')
            eflr_count += 1
        logger.info(f'{self.LOGGER_PREFIX}: EFLR count {eflr_count}')

        # for i, iflr_position_description in enumerate(self._iterate_IFLR(file_id)):
        #     logger.info(f'SERVER: IFLR [{i:6d}] {iflr_position_description}')

        self.low_level_index[file_id] = {
            lr_pos.lr_position: lr_pos for lr_pos in self._iterate_logical_record_positions(file_id)
        }

        seek_read = common.SeekRead()
        for lrsh_pos in sorted(self.low_level_index[file_id].keys()):
            index_entry = self.low_level_index[file_id][lrsh_pos]
            if index_entry.lr_attributes.is_eflr:
                need = self.svfs.need(file_id, index_entry.lr_position, index_entry.lr_length)
                seek_read.extend(need)
        logger.info(f'{self.LOGGER_PREFIX}: need[{len(seek_read)}] blocks total {seek_read.total_data_bytes()} bytes.')
        # pprint.pprint(seek_read)
        # ret = seek_read.to_json()
        logger.info(f'{self.LOGGER_PREFIX}: Creating EFLR data set took {timer.ms():.3f} (ms).')
        ret = ['seek_read', file_id, seek_read.seek_read]
        return ret

    def add_data(self, file_id: str, mod_time: float, seek_read: typing.List[typing.Tuple[int, str]]) -> str:
        """Add data to the SVFS. Returns JSON: {"response" : True} or False on failure"""
        timer = common.Timer()
        # TODO: Error management
        # if not self.svfs.has(file_id):
        #     return json.dumps(
        #         {
        #             'response': 'False',
        #             'error': f'No file of id={file_id}',
        #             'action': f'add_file',
        #         }
        #     )
        # if not self.svfs.file_mod_time_matches(file_id, mod_time):
        #     # TODO: Option do not remove but continue?
        #     self.svfs.remove(file_id)
        #     return json.dumps(
        #         {
        #             'response': 'False',
        #             'error': f'File id={file_id} mod time={mod_time} but expected {self.svfs.file_mod_time(file_id)}',
        #             'action': f'add_file',
        #         }
        #     )
        for fpos, str_data in seek_read:
            fpos = int(fpos)
            data = common.decode_bytes(str_data)
            self.svfs.write(file_id, fpos, data)
        logger.info(f'{self.LOGGER_PREFIX}: add_data() took {timer.ms():.3f} (ms).')
        return []

    def _iterate_visible_record_positions(self, file_id: str) -> typing.Iterable[typing.Tuple[int, int]]:
        fpos = STORAGE_UNIT_LABEL_LENGTH
        while self.svfs.has_data(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH):
            by = self.svfs.read(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH)
            vr_length, _vr_version = common.split_four_bytes_for_vr(by)
            yield fpos, vr_length
            fpos += vr_length

    def _iterate_logical_record_positions(self, file_id: str) -> typing.Sequence[LRPosDesc]:
        # NOTE: This is very similar to TotalDepth.RP66V1.core.pFile.FileRead#iter_logical_record_positions
        # lr_position_first_header = 80 + 4
        fpos_first_lr = -1
        for vr_position, vr_length in self._iterate_visible_record_positions(file_id):
            fpos = vr_position + VISIBLE_RECORD_HEADER_LENGTH
            while fpos < vr_position + vr_length:
                by = self.svfs.read(file_id, fpos, LRSH_LENGTH)
                lr_length, raw_lr_attributes, lr_type = common.split_four_bytes_for_lr(by)
                lr_attributes = File.LogicalRecordSegmentHeaderAttributes(raw_lr_attributes)
                if lr_attributes.is_first:
                    fpos_first_lr = fpos
                if lr_attributes.is_last:
                    assert fpos_first_lr != -1
                    yield LRPosDesc(vr_position, vr_length, fpos_first_lr, lr_attributes, lr_type, fpos + lr_length)
                    fpos_first_lr = -1
                fpos += lr_length

    def _iterate_EFLR(self, file_id: str) -> typing.Sequence[File.LRPosDesc]:
        for lr_pos_desc in self._iterate_logical_record_positions(file_id):
            if lr_pos_desc.lr_attributes.is_eflr and not lr_pos_desc.lr_attributes.is_encrypted:
                yield lr_pos_desc

    def _iterate_IFLR(self, file_id: str) -> typing.Sequence[File.LRPosDesc]:
        for lr_pos_desc in self._iterate_logical_record_positions(file_id):
            if not lr_pos_desc.lr_attributes.is_eflr and not lr_pos_desc.lr_attributes.is_encrypted:
                yield lr_pos_desc


    def EFLR_id_as_fpos(self, file_id: str, mod_time: float) -> str:
        ids = sorted(self.low_level_index[file_id].keys())
        return json.dumps(
            {
                'response': 'True',
                'data': ids,
            }
        )

    def _read_logical_record_data(self, file_id: str, id_as_fpos: int,
                                  offset: int = 0, length: int = -1) -> File.LogicalData:
        """
        Read Logical Data from the SVFS for the specified file and the LRSH header location.

        :param file_id: The file ID. This must be in the SVFS (and self.low_level_index).
            The modification time is assumed to be correct and this is not checked.
        :param id_as_fpos: The file position of the starting LRSH. This must be in self.low_level_index[file_id]
        :param offset: Offset into the logical data, must be >= 0
        :param length: Length of the logical data, by default all of it.
        :return: The Logical Data as a File.LogicalData.
        """
        timer = common.Timer()
        assert self.svfs.has(file_id)
        assert id_as_fpos in self.low_level_index[file_id]
        index_entry = self.low_level_index[file_id][id_as_fpos]

        vr_position_next = index_entry.vr_position_end
        fpos = index_entry.lr_position + LRSH_LENGTH
        lr_attributes = index_entry.lr_attributes
        assert lr_attributes.is_first
        lr_data_length = index_entry.lr_length - LRSH_LENGTH
        lr_data_chunks = []

        while fpos < vr_position_next:
            lr_data_chunk = self.svfs.read(file_id, fpos, lr_data_length)
            fpos += lr_data_length
            assert len(lr_data_chunk) == lr_data_length
            if lr_attributes.has_pad_bytes:
                pad_length = lr_data_chunk[-1]
                assert pad_length >= 0
                assert pad_length < lr_data_length
                lr_data_chunk = lr_data_chunk[:-pad_length]
            lr_data_chunks.append(lr_data_chunk)
            if lr_attributes.is_last:
                break
            if fpos == vr_position_next:
                # Consume visible record
                by = self.svfs.read(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH)
                vr_length, _vr_version = common.split_four_bytes_for_vr(by)
                vr_position_next = fpos + vr_length
                fpos += VISIBLE_RECORD_HEADER_LENGTH
            # Consume LRSH
            by = self.svfs.read(file_id, fpos, LRSH_LENGTH)
            lr_length, raw_lr_attributes, lr_type = common.split_four_bytes_for_lr(by)
            lr_attributes = File.LogicalRecordSegmentHeaderAttributes(raw_lr_attributes)
            lr_data_length = lr_length - LRSH_LENGTH
            fpos += LRSH_LENGTH
            assert not lr_attributes.is_first, str(lr_attributes)
            assert lr_data_length >= 0
        lr_data = b''.join(lr_data_chunks)
        return File.LogicalData(lr_data)

    def render_EFLR(self, file_id: str, mod_time: float, json_bytes: str) -> str:
        id_as_fpos: int = 0
        logical_data = self._read_logical_record_data(file_id, id_as_fpos)
        eflr = EFLR.ExplicitlyFormattedLogicalRecord(0, logical_data)
        print(eflr)
        # TODO: EFLR as HTML
