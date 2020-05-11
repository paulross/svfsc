import base64
import json
import time
import typing


def split_four_bytes_for_vr(by: bytes) -> typing.Tuple[int, int]:
    if len(by) != 4:
        raise ValueError(f'Need four bytes not {by!r}')
    return by[0] << 8 | by[1], by[2] << 8 | by[3]


def split_four_bytes_for_lr(by: bytes) -> typing.Tuple[int, int, int]:
    if len(by) != 4:
        raise ValueError(f'Need four bytes not {by!r}')
    return by[0] << 8 | by[1], by[2], by[3]


def encode_bytes(by: bytes) -> str:
    encoded = base64.encodebytes(by)
    ret = encoded.decode('ascii')
    return ret


def decode_bytes(st: str) -> bytes:
    decoded = st.encode('ascii')
    ret = base64.decodebytes(decoded)
    return ret


class Timer:
    def __init__(self):
        self.time = time.perf_counter()

    def s(self):
        return time.perf_counter() - self.time

    def ms(self):
        return self.s() * 1000


class SeekRead:

    def __init__(self, sequence: typing.Sequence[typing.Tuple[int, int]] = tuple()):
        self.seek_read: typing.List[typing.Tuple[int, int]] = []
        if sequence:
            self.seek_read.extend(sequence)

    def append(self, value: typing.Tuple[int, int]) -> None:
        self.seek_read.append(value)

    def extend(self, sequence: typing.Sequence[typing.Tuple[int, int]]) -> None:
        self.seek_read.extend(sequence)

    def to_json(self) -> str:
        return json.dumps(self.seek_read)

    @staticmethod
    def from_json(json_data: str):
        return SeekRead(json.loads(json_data))

    def __eq__(self, other) -> bool:
        if self.__class__  == other.__class__:
            return self.seek_read == other.seek_read
        return NotImplemented

    def __len__(self) -> int:
        return len(self.seek_read)
