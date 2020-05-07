import json
import logging
import time
import typing

import svfs

from src.cpy import common, connection
from TotalDepth.RP66V1.core import File

logger = logging.getLogger(__file__)


STORAGE_UNIT_LABEL_LENGTH = 80
VISIBLE_RECORD_HEADER_LENGTH = 4
LRSH_LENGTH = 4
BITS_PER_BYTE = 8


class Server:
    # TODO: Create a TotalDepth.RP66V1.core.pIndex.LogicalRecordIndex from this information.
    # TODO: Read specific EFLR at file position.
    # TODO: Read set of IFLRs
    def __init__(self):
        logger.info(f'SERVER: SVFS.__init__()')
        self.svfs = svfs.SVFS()

    def _sim_delay(self, json_bytes: str) -> None:
        sleep_time = connection.LATENCY_S + \
                     connection.PAYLOAD_OVERHEAD * len(json_bytes) * BITS_PER_BYTE / connection.BANDWIDTH_BPS
        logger.info(f'SERVER: _sim_delay len={len(json_bytes):12,d} sleep={sleep_time * 1000:.3f} (ms)')
        time.sleep(sleep_time)

    def add_file(self, file_id: str, mod_time: float, json_bytes: bytes) -> bool:
        logger.info(f'SERVER: add_file() {file_id}')
        self._sim_delay(json_bytes)
        if not self.svfs.has(file_id):
            self.svfs.insert(file_id, mod_time)
        self.add_data(file_id, mod_time, json_bytes)
        logger.info(
            f'SERVER: add_file() has {self.svfs.num_bytes(file_id)} bytes'
            f' {self.svfs.num_blocks(file_id)} blocks'
            f' size_of: {self.svfs.size_of(file_id)} bytes.'
        )
        logger.info(
            f'SERVER: add_file() SVFS total {self.svfs.total_bytes()} bytes'
            f' total_size_of {self.svfs.total_size_of()} bytes.'
        )
        # for lr_position_description in self._iterate_logical_record_positions(file_id):
        #     logger.info(f'SERVER: {lr_position_description}')

        for e, eflr_position_description in enumerate(self._iterate_EFLR(file_id)):
            logger.info(f'SERVER: EFLR [{e}] {eflr_position_description}')

        for i, iflr_position_description in enumerate(self._iterate_IFLR(file_id)):
            logger.info(f'SERVER: IFLR [{i}] {iflr_position_description}')

        return True

        # Now cycle through the logical records reading the first 64 bytes of each record.
        # Or EFLRs and IFLR first few bytes.

        #
        seek_read: typing.List[typing.Tuple[int, int]] = []

        json_bytes = json.dumps(seek_read)
        self._sim_delay(json_bytes)
        return json_bytes

    def add_data(self, file_id: str, mod_time: float, json_bytes: bytes) -> None:
        assert self.svfs.has(file_id)
        assert self.svfs.file_mod_time_matches(file_id, mod_time)
        py_index = json.loads(json_bytes)
        for fpos, str_data in py_index:
            fpos = int(fpos)
            data = common.decode_bytes(str_data)
            self.svfs.write(file_id, fpos, data)

    def _iterate_visible_record_positions(self, file_id: str) -> typing.Iterable[typing.Tuple[int, int]]:
        fpos = STORAGE_UNIT_LABEL_LENGTH
        while self.svfs.has_data(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH):
            by = self.svfs.read(file_id, fpos, VISIBLE_RECORD_HEADER_LENGTH)
            vr_length, vr_version = common.split_four_bytes_for_vr(by)
            yield fpos, vr_length
            fpos += vr_length

    def _iterate_logical_record_positions(self, file_id: str) -> typing.Sequence[File.LRPosDesc]:
        # NOTE: This is very similar to TotalDepth.RP66V1.core.pFile.FileRead#iter_logical_record_positions
        # lr_position_first_header = 80 + 4
        fpos_first_lr = -1
        for vr_position, vr_length in self._iterate_visible_record_positions(file_id):
            fpos = vr_position + VISIBLE_RECORD_HEADER_LENGTH
            while fpos < vr_position + vr_length:
                by = self.svfs.read(file_id, fpos, LRSH_LENGTH)
                lr_length, _lr_attributes, lr_type = common.split_four_bytes_for_lr(by)
                lr_attributes = File.LogicalRecordSegmentHeaderAttributes(_lr_attributes)
                if lr_attributes.is_first:
                    fpos_first_lr = fpos
                if lr_attributes.is_last:
                    assert fpos_first_lr != -1
                    # TODO: Write our own POD classes.
                    yield File.LRPosDesc(
                        File.LogicalRecordPositionBase(vr_position, fpos),
                        File.LogicalDataDescription(lr_attributes, lr_type, fpos + lr_length - fpos_first_lr),
                    )
                    fpos_first_lr = -1
                fpos += lr_length

    def _iterate_EFLR(self, file_id: str) -> typing.Sequence[File.LRPosDesc]:
        for lr_pos_desc in self._iterate_logical_record_positions(file_id):
            if lr_pos_desc.description.attributes.is_eflr:
                yield lr_pos_desc

    def _iterate_IFLR(self, file_id: str) -> typing.Sequence[File.LRPosDesc]:
        for lr_pos_desc in self._iterate_logical_record_positions(file_id):
            if not lr_pos_desc.description.attributes.is_eflr:
                yield lr_pos_desc


