import base64
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
