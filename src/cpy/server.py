import json
import logging
import time
import typing

import svfs

from src.cpy import common, connection

logger = logging.getLogger(__file__)


class LogicalRecordSegmentHeaderAttributes:
    def __init__(self, attributes: int):
        if 0 <= attributes <= 0xff:
            self.attributes = attributes
        else:
            raise ValueError(f'Attributes must be in the range of an unsigned char not 0x{attributes:x}')

    def __eq__(self, other):
        if self.__class__ == other.__class__:
            return self.attributes == other.attributes
        return NotImplemented

    def __str__(self) -> str:
        return f'LRSH attr: 0x{self.attributes:02x}'

    # Attribute access
    @property
    def is_eflr(self) -> bool:
        return self.attributes & 0x80 != 0

    @property
    def is_first(self) -> bool:
        return self.attributes & 0x40 == 0

    @property
    def is_last(self) -> bool:
        return self.attributes & 0x20 == 0

    @property
    def is_encrypted(self) -> bool:
        return self.attributes & 0x10 != 0

    @property
    def has_encryption_packet(self) -> bool:
        return self.attributes & 0x08 != 0

    @property
    def has_checksum(self) -> bool:
        return self.attributes & 0x04 != 0

    @property
    def has_trailing_length(self) -> bool:
        return self.attributes & 0x02 != 0

    @property
    def has_pad_bytes(self) -> bool:
        """Note: Pad bytes will not be visible if the record is encrypted."""
        return self.attributes & 0x01 != 0

    def attribute_str(self) -> str:
        """Returns a long string of the important attributes."""
        ret = [
            'EFLR' if self.is_eflr else 'IFLR',
        ]
        if self.is_first:
            ret.append('first')
        if self.is_last:
            ret.append('last')
        if self.is_encrypted:
            ret.append('encrypted')
        if self.has_checksum:
            ret.append('checksum')
        if self.has_trailing_length:
            ret.append('trailing length')
        if self.has_pad_bytes:
            ret.append('padding')
        return '-'.join(ret)


class Server:
    # TODO: Create a TotalDepth.RP66V1.core.pIndex.LogicalRecordIndex from this information.
    # TODO: Read specific EFLR at file position.
    # TODO: Read set of IFLRs
    def __init__(self):
        logger.info(f'SERVER: SVFS.__init__()')
        self.svfs = svfs.SVFS()

    def _sim_delay(self, json_bytes: str) -> None:
        sleep_time = connection.LATENCY_S + connection.PAYLOAD_OVERHEAD * len(json_bytes) * 8 / connection.BANDWIDTH_BPS
        logger.info(f'SERVER: _sim_delay len={len(json_bytes):12,d} sleep={sleep_time * 1000:.3f} (ms)')
        time.sleep(sleep_time)

    def add_file(self, file_id: str, mod_time: float, json_bytes: bytes) -> bytes:
        logger.info(f'SERVER: add_file() {file_id}')
        self._sim_delay(json_bytes)
        if not self.svfs.has(file_id):
            self.svfs.insert(file_id, mod_time)
        self.add_data(file_id, mod_time, json_bytes)
        for fpos, lr_attributes, lr_type, ld_length in self._iterate_logical_record_positions(file_id):
            logger.info(
                f'SERVER: fpos=0x{fpos:08x} {lr_attributes} {lr_type:3d} {lr_attributes.is_eflr!r:5} {ld_length:6d}'
            )


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
        fpos = 80
        while self.svfs.has_data(file_id, fpos, 4):
            by = self.svfs.read(file_id, fpos, 4)
            vr_length, vr_version = common.split_four_bytes_for_vr(by)
            yield fpos, vr_length
            fpos += vr_length

    def _iterate_logical_record_positions(self, file_id: str) \
            -> typing.Sequence[
                typing.Tuple[
                    int, LogicalRecordSegmentHeaderAttributes, int, typing.List[typing.Tuple[int, int]]
                ]
            ]:
        # lr_position_first_header = 80 + 4
        for vr_position, vr_length in self._iterate_visible_record_positions(file_id):
            fpos = vr_position + 4
            logical_data_length = 0
            while fpos < vr_position + vr_length:
                by = self.svfs.read(file_id, fpos, 4)
                lr_length, _lr_attributes, lr_type = common.split_four_bytes_for_lr(by)
                lr_attributes = LogicalRecordSegmentHeaderAttributes(_lr_attributes)
                # if lr_attributes & 0x ? then cycle through to last and yield lr_position_first_header and logical data
                # length or seek read
                # yield (position, lr_attributes, lr_type, [(seek, read), ...])
                # seek_read = self.svfs.need(file_id, lr_position_first_header, fpos)
                if lr_attributes.is_first:
                    yield fpos, lr_attributes, lr_type, logical_data_length
                    logical_data_length = 0
                fpos += lr_length
                logical_data_length += lr_length - 4
