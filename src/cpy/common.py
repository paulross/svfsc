import base64
import json
import pprint
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
    """Takes a bytes object, encodes it with base64 and returns that as an ASCII string."""
    encoded = base64.encodebytes(by)
    ret = encoded.decode('ascii')
    return ret


def decode_bytes(st: str) -> bytes:
    """Takes an ASCII string, decodes it with base64 and returns a bytes object."""
    decoded = st.encode('ascii')
    ret = base64.decodebytes(decoded)
    return ret


class IntValueLookup:
    """Assigns a unique code to an integer."""
    def __init__(self):
        self.value_to_index: typing.Dict[int, int] = {}

    def index(self, value: int) -> int:
        if value not in self.value_to_index:
            self.value_to_index[value] = len(self.value_to_index)
        return self.value_to_index[value]

    def index_to_value_map(self) -> typing.Dict[int, int]:
        """The reverse map to extract original integers from the code."""
        ret: typing.Dict[int, int] = {}
        for k, v in self.value_to_index.items():
            assert v not in ret
            ret[v] = k
        return ret


def json_encode_seek_read_4_byte_optimised(seek_read: typing.Sequence[typing.Tuple[int, bytes]],
                                           encode_integers: bool) -> str:
    """Encodes a sequence of seek/read results as JSON."""
    fpos_value_list = []
    int_lookup = IntValueLookup()
    for fpos, by in seek_read:
        fpos_value_list.append(fpos)
        if len(by) == 4:
            encoded_data = int.from_bytes(by, byteorder='little')
            if encode_integers:
                encoded_data = int_lookup.index(encoded_data)
        else:
            encoded_data = encode_bytes(by)
        fpos_value_list.append(encoded_data)
    assert len(fpos_value_list) % 2 == 0
    if encode_integers:
        obj = {
            'data': fpos_value_list,
            'int_map': int_lookup.index_to_value_map(),
        }
        ret = json.dumps(obj)
        # print(obj)
        # pprint.pprint(int_lookup.index_to_value_map())
        # pprint.pprint(obj)
    else:
        ret = json.dumps(fpos_value_list)
    return ret


def json_decode_seek_read_4_byte_optimised(json_data: str) -> typing.Sequence[typing.Tuple[int, bytes]]:
    json_decoded_object = json.loads(json_data)
    if isinstance(json_decoded_object, list):
        json_decode_list = json_decoded_object
        int_value_map = None
    elif isinstance(json_decoded_object, dict):
        json_decode_list = json_decoded_object['data']
        int_value_map = {int(k): v for k, v in json_decoded_object['int_map'].items()}
    ret = []
    assert len(json_decode_list) % 2 == 0
    for i in range(0, len(json_decode_list), 2):
        fpos = json_decode_list[i]
        encoded_data = json_decode_list[i + 1]
        if isinstance(encoded_data, int):
            if int_value_map is not None:
                encoded_data = int_value_map[encoded_data]
            value = encoded_data.to_bytes(length=4, byteorder='little')
        elif isinstance(encoded_data, str):
            value = decode_bytes(encoded_data)
        else:
            raise ValueError(f'At index {i+1} of type {type(encoded_data)} repr: {encoded_data!r} is not recognised')
        ret.append((fpos, value))
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
