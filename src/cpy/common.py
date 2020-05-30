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

    def total_data_bytes(self) -> int:
        return sum(v[1] for v in self.seek_read)


class CommunicationJSON:
    """Class that encapsulates the JSON communication.

    In JSON this looks something like the following.

    Client to server, 'call' -> pair::

        {
            'call' : [server_function_name, [ID, mod_time, args...]],
        }

    The server will process the arguments with the function. The server response will be sent to the client as:

    Server to client, 'call' -> triple, a NOP is an empty list::

        {
            'call' : [client_function_name, [ID, mod_time, args...], server_function_name],
        }

    The client will process those arguments with that function and call the server as above::

        {
            'call' : [server_function_name, [ID, mod_time, args...]],
        }

    """
    def __init__(self):
        self.data = {
            'call': []
        }

    def add_call(self, *args: typing.Tuple[typing.Any]):
        assert isinstance(args, tuple)
        self.data['call'].append(args)

    def gen_call(self) -> typing.Sequence[typing.Tuple[str, typing.List[typing.Any]]]:
        for value in self.data['call']:
            yield value
            # yield value[0], value[1:]

    def json_dumps(self) -> str:
        return json.dumps(self.data)

    @staticmethod
    def json_reads(json_data: str):
        ret = CommunicationJSON()
        ret.data = json.loads(json_data)
        return ret

    def __len__(self) -> int:
        return len(self.data['call'])

    def __bool__(self) -> bool:
        return len(self.data['call']) > 0
