"""
"""
import collections
import json
import logging
import pprint
import psutil
import typing

import svfs
from TotalDepth.RP66V1.core import File
from TotalDepth.RP66V1.core.LogicalRecord import EFLR

from src.cpy import common, server_index

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

    @property
    def is_public(self) -> bool:
        return self.lr_type < 128

    def __str__(self) -> str:
        return f'LRPosDesc VR: 0x{self.vr_position:08x} LRSH: 0x{self.lr_position:08x} {self.lr_attributes!s}' \
            f' type: {self.lr_type:3d} end: 0x{self.lr_position_end:08x}' \
            f' len: 0x{self.lr_length:08x} {self.lr_length:,d}'


# TODO: Put this back into TotalDepth
class LogicalRecordSegmentHeaderBase:
    """RP66V1 Logical Record Segment Header. See See [RP66V1 2.2.2.1]"""
    HEAD_LENGTH = 4
    # MIN_LENGTH = LOGICAL_RECORD_SEGMENT_MINIMUM_SIZE

    def __init__(self, position: int, length: int, raw_attributes: int, record_type: int):
        """Constructor.
        position: The file position of the start of the LRSH.

        length: The *Logical Record Segment Length* is a two-byte, unsigned integer (Representation Code UNORM) that
            specifies the length, in bytes, of the Logical Record Segment. The Logical Record Segment Length is required
            to be even. The even length ensures that 2-byte checksums can be computed, when present, and permits some
            operating systems to handle DLIS data more efficiently without degrading performance with other systems.
            There is no limitation on a Logical Record length. Logical Record Segments must contain at least sixteen
            (16) bytes. This requirement facilitates mapping the Logical Format to those Physical Formats that require a
            minimum physical record length.

        attributes: The *Logical Record Segment Attributes* consist of a one-byte bit string that specifies the
            Attributes of the Logical Record Segment. Its structure is defined in Figure 2-3. Since its structure is
            defined explicitly in Figure 2-3, no Representation Code is assigned to it.

        record_type: The *Logical Record Type* is a one-byte, unsigned integer (Representation Code USHORT) that
            specifies the Type of the Logical Record. Its value indicates the general semantic content of the Logical
            Record. The same value must be used in all Segments of a Logical Record. Logical Record Types are specified
            in Appendix A.

            IFLRs: Numeric codes 0-127 are reserved for Public IFLRs. Codes 128-255 are reserved for Private IFLRs.
            0 is Frame Data, 1 is unformatted data.

            EFLRs: Numeric codes 0-127 are reserved for Public EFLRs. Codes 128-255 are reserved for Private EFLRs.
            0 is FILE-HEADER, 1 is ORIGIN and so on.
        """
        self.position = position
        self.length = length
        self.attributes = File.LogicalRecordSegmentHeaderAttributes(raw_attributes)
        self.record_type = record_type

    # def _read(self, fobj: typing.BinaryIO) -> typing.Tuple[int, int, LogicalRecordSegmentHeaderAttributes, int]:
    #     position = fobj.tell()
    #     try:
    #         length = read_two_bytes_big_endian(fobj)
    #         # TODO: Raise on minimum length. Maybe make this a read/write property or descriptor
    #         attributes = LogicalRecordSegmentHeaderAttributes(read_one_byte(fobj))
    #         # TODO: Raise on attribute conflicts, for example:
    #         # If encryption packet then encryption must be set
    #         # Compare successors with previous - trailing length must be all or nothing, encryption all or nothing.
    #         record_type = read_one_byte(fobj)
    #     except ExceptionEOF:
    #         raise ExceptionLogicalRecordSegmentHeaderEOF(f'LogicalRecordSegmentHeader EOF at 0x{position:x}')
    #     return position, length, attributes, record_type
    #
    # def read(self, fobj: typing.BinaryIO) -> None:
    #     """Read a new Logical Record Segment Header.
    #     This may throw a ExceptionVisibleRecord or ExceptionLogicalRecordSegmentHeaderEOF."""
    #     self.position, self.length, self.attributes, self.record_type = self._read(fobj)

    # def as_bytes(self) -> bytes:
    #     """The LRSH represented in raw bytes."""
    #     return two_bytes_big_endian(self.length) + bytes([self.attributes.attributes, self.record_type])

    def __str__(self) -> str:
        return '<LogicalRecordSegmentHeader: @ 0x{:x} len=0x{:x}' \
               ' attr=0x{:x} type={:d}>'.format(
            self.position, self.length, self.attributes.attributes, self.record_type
        )

    def long_str(self) -> str:
        return f'LRSH: @ 0x{self.position:x} len=0x{self.length:x}' \
            f' type={self.record_type:d} {self.attributes.attribute_str()}'

    @property
    def next_position(self) -> int:
        """File position of the start of the next Logical Record Segment Header."""
        return self.position + self.length

    @property
    def logical_data_position(self) -> int:
        """File position of the start of the Logical Data."""
        return self.position + self.HEAD_LENGTH

    @property
    def must_strip_padding(self) -> bool:
        return self.attributes.has_pad_bytes and not self.attributes.is_encrypted

    @property
    def logical_data_length(self):
        """Returns the length of the logical data, including padding but excluding the tail."""
        ret = self.length - self.HEAD_LENGTH
        if self.attributes.has_checksum:
            ret -= 2
        if self.attributes.has_trailing_length:
            ret -= 2
        return ret

    @property
    def is_public(self) -> bool:
        return self.record_type < 128


class Server:
    # TODO: Read specific EFLR at file position.
    # TODO: Read set of IFLRs

    LOGGER_PREFIX = 'SERVER'

    def __init__(self):
        logger.info(f'{self.LOGGER_PREFIX}: __init__()')
        self.svfs = svfs.SVFS()
        # TODO: Single dict with low and mid level indexes
        # Dict of {file_id, {position as a LRPosDesc, ...}, ...}
        self.low_level_index: typing.Dict[str, typing.Dict[int, LRPosDesc]] = {}
        self.mid_level_index: typing.Dict[str, server_index.MidLevelIndex] = {}
        self._function_map = {
            'add_file': self.add_file,
            'add_eflrs': self.add_eflrs,
        }
        self.log_server_details()

    def from_client(self, json_data: str) -> str:
        comms_from_client = common.CommunicationJSON.json_reads(json_data)
        comms_to_client = common.CommunicationJSON()
        for function_name, *args in comms_from_client.gen_call():
            function = self._function_map[function_name]
            result = function(*args)
            if result:
                comms_to_client.add_call(*result)
        return comms_to_client.json_dumps()

    def add_file(self, file_id: str, mod_time: float,
                 seek_read: typing.List[typing.Tuple[int, str]]) -> typing.List[typing.Any]:
        """Add a file and return JSON encoded seek/read list."""
        logger.info(f'{self.LOGGER_PREFIX}: add_file() {file_id}')
        timer = common.Timer()
        if not self.svfs.has(file_id):
            self.svfs.insert(file_id, mod_time)
        seek_read = common.json_decode_seek_read_4_byte_optimised(seek_read)
        self._add_data(file_id, mod_time, seek_read)
        self.log_svf_details(file_id, mod_time)
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

        self.low_level_index[file_id] = {
            lr_pos.lr_position: lr_pos for lr_pos in self._iterate_logical_record_positions(file_id)
        }
        # self.log_details(file_id, mod_time)
        self.mid_level_index[file_id] = server_index.MidLevelIndex()

        # Read all the data to construct all the EFLRs
        seek_read = common.SeekRead()
        for lrsh_pos in sorted(self.low_level_index[file_id].keys()):
            index_entry = self.low_level_index[file_id][lrsh_pos]
            if self._use_this_EFLR(index_entry):
                need = self.svfs.need(file_id, index_entry.lr_position, index_entry.lr_length)
                seek_read.extend(need)
        self.log_svf_details(file_id, mod_time)
        logger.info(f'{self.LOGGER_PREFIX}: need[{len(seek_read)}] blocks total {seek_read.total_data_bytes()} bytes.')
        logger.info(f'{self.LOGGER_PREFIX}: Creating data set for EFLRs took {timer.ms():.3f} (ms).')
        ret = ['seek_read', [file_id, mod_time, seek_read.seek_read], 'add_eflrs']
        return ret

    def file_id_and_mod_date_is_valid(self, file_id: str, mod_time: float) -> bool:
        return self.svfs.has(file_id) and self.svfs.file_mod_time_matches(file_id, mod_time)

    def _add_data(self, file_id: str, mod_time: float, seek_read: typing.List[typing.Tuple[int, str]]) -> None:
        """Add data to the SVFS. Returns JSON: {"response" : True} or False on failure"""
        assert self.svfs.has(file_id)
        assert self.svfs.file_mod_time_matches(file_id, mod_time)
        timer = common.Timer()
        for i, (fpos, data) in enumerate(seek_read):
            fpos = int(fpos)
            # data = common.decode_bytes(str_data)
            if isinstance(data, str):
                data = common.decode_bytes(data)
            self.svfs.write(file_id, fpos, data)
        logger.info(f'{self.LOGGER_PREFIX}: _add_data() took {timer.ms():.3f} (ms).')

    def add_eflrs(self, file_id: str, mod_time: float, seek_read: typing.List[typing.Tuple[int, str]]):
        logger.info(f'{self.LOGGER_PREFIX}: add_eflrs().')
        timer = common.Timer()
        self._add_data(file_id, mod_time, seek_read)
        for position in self._iterate_EFLR(file_id):
            logical_data = self._read_logical_record_data(file_id, position)
            eflr = EFLR.ExplicitlyFormattedLogicalRecord(position.lr_type, logical_data)
            # print(eflr)
            # print(eflr.str_long())
            self.mid_level_index[file_id].add_eflr(position.lr_position, eflr)
        # print(f'Mid level index {self.mid_level_index[file_id].long_str()}')
        # print(f'XXXX {self.svf_details(file_id, mod_time)}')
        # Log what we know about the _possible_ EFLR set
        possible_eflr_dict = self._count_EFRL_attributes(file_id)
        msg = ', '.join(f'{k}: {possible_eflr_dict[k]}' for k in sorted(possible_eflr_dict.keys()))
        logger.info(f'{self.LOGGER_PREFIX}: add_eflrs(): EFLR attrs: {msg}')

        self.log_svf_details(file_id, mod_time)
        self.log_server_details()
        logger.info(f'{self.LOGGER_PREFIX}: add_eflrs(): {self.mid_level_index[file_id]} took {timer.ms():.3f} (ms).')

    def _read_visible_record(self, file_id: str, fpos: int) -> typing.Tuple[int, int]:
        assert self.svfs.has(file_id)
        assert self.svfs.has_data(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH)

        by = self.svfs.read(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH)
        vr_length, vr_version = common.split_four_bytes_for_vr(by)
        return vr_length, vr_version

    def _iterate_visible_record_positions(self, file_id: str) -> typing.Iterable[typing.Tuple[int, int]]:
        fpos = STORAGE_UNIT_LABEL_LENGTH
        while self.svfs.has_data(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH):
            # by = self.svfs.read(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH)
            # vr_length, _vr_version = common.split_four_bytes_for_vr(by)
            vr_length, _vr_version = self._read_visible_record(file_id, fpos)
            yield fpos, vr_length
            fpos += vr_length

    def _read_logical_record_segment_header(self, file_id: str, fpos: int) -> LogicalRecordSegmentHeaderBase:
        assert self.svfs.has(file_id)
        assert self.svfs.has_data(file_id, fpos, LRSH_LENGTH)

        by = self.svfs.read(file_id, fpos, LRSH_LENGTH)
        lr_length, raw_lr_attributes, lr_type = common.split_four_bytes_for_lr(by)
        # lr_attributes = File.LogicalRecordSegmentHeaderAttributes(raw_lr_attributes)
        return LogicalRecordSegmentHeaderBase(fpos, lr_length, raw_lr_attributes, lr_type)

    def _iterate_logical_record_positions(self, file_id: str) -> typing.Sequence[LRPosDesc]:
        # NOTE: This is very similar to TotalDepth.RP66V1.core.pFile.FileRead#iter_logical_record_positions
        # lr_position_first_header = 80 + 4
        # fpos_first_lr = -1
        lr_first_attrs = None
        for vr_position, vr_length in self._iterate_visible_record_positions(file_id):
            fpos = vr_position + VISIBLE_RECORD_HEADER_LENGTH
            while fpos < vr_position + vr_length:
                # by = self.svfs.read(file_id, fpos, LRSH_LENGTH)
                # lr_length, raw_lr_attributes, lr_type = common.split_four_bytes_for_lr(by)
                # lr_attributes = File.LogicalRecordSegmentHeaderAttributes(raw_lr_attributes)
                lrsh = self._read_logical_record_segment_header(file_id, fpos)
                if lrsh.attributes.is_first:
                    lr_first_attrs = vr_position, vr_length, fpos, lrsh.attributes, lrsh.record_type
                if lrsh.attributes.is_last:
                    assert lr_first_attrs is not None
                    yield LRPosDesc(*lr_first_attrs, fpos + lrsh.length)
                    lr_first_attrs = None
                fpos += lrsh.length

    def _iterate_low_level_index(self, file_id: str, first_lrsh_only: bool) -> typing.Sequence[LRPosDesc]:
        assert file_id in self.low_level_index, f'_iterate_low_level_index(): Missing file_id "{file_id}"'
        index = self.low_level_index[file_id]
        # Relies on sort order of index
        for value in index.values():
            if value.lr_attributes.is_first or not first_lrsh_only:
                yield value

    def _count_EFRL_attributes(self, file_id: str) -> typing.Dict[str, int]:
        assert file_id in self.low_level_index, f'_iterate_low_level_index(): Missing file_id "{file_id}"'
        ret = collections.Counter()
        for lr_pos_desc in self._iterate_low_level_index(file_id, first_lrsh_only=True):
            if lr_pos_desc.lr_attributes.is_eflr:
                key = (f'Encrypt: {lr_pos_desc.lr_attributes.is_encrypted}', f'Public: {lr_pos_desc.is_public}')
                ret.update([key])
        return ret

    def _use_this_EFLR_IFLR(self, lr_pos_desc: LRPosDesc) -> bool:
        attr = lr_pos_desc.lr_attributes
        assert attr.is_first
        return not attr.is_encrypted and lr_pos_desc.is_public

    def _use_this_EFLR(self, lr_pos_desc: LRPosDesc) -> bool:
        attr = lr_pos_desc.lr_attributes
        assert attr.is_first
        return attr.is_eflr and self._use_this_EFLR_IFLR(lr_pos_desc)

    def _use_this_IFLR(self, lr_pos_desc: LRPosDesc) -> bool:
        attr = lr_pos_desc.lr_attributes
        assert attr.is_first
        return not attr.is_eflr and self._use_this_EFLR_IFLR(lr_pos_desc)

    def _iterate_EFLR(self, file_id: str) -> typing.Sequence[LRPosDesc]:
        for lr_data in self._iterate_low_level_index(file_id, first_lrsh_only=True):
            if self._use_this_EFLR(lr_data):
                yield lr_data

    def _iterate_IFLR(self, file_id: str) -> typing.Sequence[LRPosDesc]:
        for lr_data in self._iterate_low_level_index(file_id):
            if self._use_this_IFLR(lr_data):
                yield lr_data

    def _read_logical_record_data(self, file_id: str, lr_pos_desc: LRPosDesc,
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
        logger.debug(f'TRACE: _read_logical_record_data(): lr_pos_desc={lr_pos_desc}')
        assert self.svfs.has(file_id), f'SVFS has no file ID: {file_id}'
        assert self.svfs.has_data(file_id, lr_pos_desc.vr_position, VISIBLE_RECORD_HEADER_LENGTH), \
            f'SVFS {file_id} has no VR data @ 0x{lr_pos_desc.vr_position:x} len={VISIBLE_RECORD_HEADER_LENGTH}'
        assert self.svfs.has_data(file_id, lr_pos_desc.lr_position, lr_pos_desc.lr_length), \
            f'SVFS {file_id} has no LD data @ 0x{lr_pos_desc.lr_position:x} len={lr_pos_desc.lr_length}'
        assert lr_pos_desc.lr_attributes.is_first, f'LRSH is not first at {lr_pos_desc}'

        timer = common.Timer()
        lr_data_chunks = []
        vr_length, _vr_version = self._read_visible_record(file_id, lr_pos_desc.vr_position)
        vr_position_next = lr_pos_desc.vr_position + vr_length
        lrsh = self._read_logical_record_segment_header(file_id, lr_pos_desc.lr_position)
        fpos = lr_pos_desc.lr_position + LRSH_LENGTH

        while fpos < vr_position_next:
            # print(f'Reading LD   at 0x{fpos:x} {lrsh.attributes} len={lrsh.logical_data_length}')
            lr_data_chunk = self.svfs.read(file_id, fpos, lrsh.logical_data_length)
            assert len(lr_data_chunk) == lrsh.logical_data_length
            fpos += lrsh.logical_data_length
            if lrsh.must_strip_padding:
                pad_length = lr_data_chunk[-1]
                # print(f'Stripping padding of {pad_length} bytes')
                assert pad_length >= 0
                assert pad_length < lrsh.logical_data_length
                lr_data_chunk = lr_data_chunk[:-pad_length]
            lr_data_chunks.append(lr_data_chunk)
            if lrsh.attributes.is_last:
                break
            if fpos == vr_position_next:
                # Consume visible record
                vr_length, _vr_version = self._read_visible_record(file_id, fpos)
                # assert _vr_version == 0xff01, f'Fpos: 0x{fpos:x} VR: 0x{vr_length:x} 0x{_vr_version:x}'
                vr_position_next = fpos + vr_length
                # print(f'TRACE: VR @ 0x{fpos:x} length 0x{vr_length:x} next @ 0x{vr_position_next:x}')
                fpos += VISIBLE_RECORD_HEADER_LENGTH
            # Consume LRSH
            lrsh = self._read_logical_record_segment_header(file_id, fpos)
            # print(f'Reading LRSH at 0x{fpos:x} {lrsh.attributes}')
            assert not lrsh.attributes.is_first, \
                f'Fpos: 0x{fpos:x} Attrs: {lrsh.attributes} VR next 0x{vr_position_next:x} Len: {lrsh.logical_data_length}'
            assert lrsh.logical_data_length >= 0
            fpos += LRSH_LENGTH
        # print(f'Logical data chunks: {[len(b) for b in lr_data_chunks]}')
        lr_data = b''.join(lr_data_chunks)
        # print(f'Logical data length: {len(lr_data)}')
        return File.LogicalData(lr_data)

    def render_EFLR(self, file_id: str, mod_time: float, json_bytes: str) -> str:
        id_as_fpos: int = 0
        logical_data = self._read_logical_record_data(file_id, id_as_fpos)
        eflr = EFLR.ExplicitlyFormattedLogicalRecord(0, logical_data)
        print(eflr)
        # TODO: EFLR as HTML

    def _svf_details(self, file_id: str, mod_time:float) -> typing.Dict[str, typing.Union[int, str]]:
        """Returns an internal SVF summary for a file as a JSON string."""
        assert self.svfs.has(file_id)
        assert self.svfs.file_mod_time_matches(file_id, mod_time)
        data = {
            'size_of': self.svfs.size_of(file_id),
            'num_blocks': self.svfs.num_blocks(file_id),
            'num_bytes': self.svfs.num_bytes(file_id),
            'count_write': self.svfs.count_write(file_id),
            'count_read': self.svfs.count_read(file_id),
            'bytes_write': self.svfs.bytes_write(file_id),
            'bytes_read': self.svfs.bytes_read(file_id),
        }
        t = self.svfs.time_write(file_id)
        if t is not None:
            data['time_write'] = t.strftime('%Y-%m-%d %H:%M:%S.%f')
        else:
            data['time_write'] = 'None'
        t = self.svfs.time_read(file_id)
        if t is not None:
            data['time_read'] = t.strftime('%Y-%m-%d %H:%M:%S.%f')
        else:
            data['time_read'] = 'None'
        return data

    def log_svf_details(self, file_id: str, mod_time:float) -> None:
        details = self._svf_details(file_id, mod_time)
        detail_str = (
            f'bytes r/w: {details["bytes_read"]}/{details["bytes_write"]}'
            f', count r/w: {details["count_read"]}/{details["count_write"]}'
            f', blocks: {details["num_blocks"]}'
            f', bytes: {details["num_bytes"]}'
            f', sizeof: {details["size_of"]}'
            f', time r/w: {details["time_read"]}/{details["time_write"]}'
        )
        logger.info(f'{self.LOGGER_PREFIX}: svf_details(): {detail_str}')

    def _server_details(self):
        proc = psutil.Process()
        ret = {
            'svfs_file_count': len(self.svfs.keys()),
            'rss': proc.memory_info().rss,
        }
        return ret

    def log_server_details(self) -> None:
        details = self._server_details()
        details_str = []
        for k in sorted(details.keys()):
            if isinstance(details[k], int):
                details_str.append(f'{k}={details[k]:,d}')
            else:
                details_str.append(f'{k}={details[k]}')
        details_str = ', '.join(details_str)
        logger.info(f'{self.LOGGER_PREFIX}: server_details(): {details_str}')
