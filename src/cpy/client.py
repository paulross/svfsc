import json
import logging
import os
import typing

from src.cpy import connection, common


logger = logging.getLogger(__file__)


EOF = 0


def read_vr(file_object: typing.BinaryIO) -> typing.Tuple[int, bytes]:
    """Reads a VR from the current position returning the length of the VR and the four bytes of the VR header."""
    by = file_object.read(4)
    if len(by) != 4:
        return EOF, by
    length, version = common.split_four_bytes_for_vr(by)
    if version != 0xff01:
        # VERSION = 0xff01
        raise IOError(f'Wrong initialisation for record header from {by!r}')
    return length, by


def read_lr(file_object: typing.BinaryIO) -> typing.Tuple[int, bytes]:
    """Reads a LRSH from the current position returning the length of the LRSH and the four bytes of the LRSH header."""
    by = file_object.read(4)
    length, _attributes, _type = common.split_four_bytes_for_lr(by)
    return length, by


def scan_file_index(file_object: typing.BinaryIO) -> typing.List[typing.Tuple[int, bytes]]:
    """Scans a RP66V1 physical file and returns the index."""
    file_object.seek(0)
    py_index: typing.List[typing.Tuple[int, bytes]] = [
        (0, file_object.read(80)),
    ]
    while True:
        tell = file_object.tell()
        vr_length, vr_bytes = read_vr(file_object)
        if vr_length == EOF:
            break
        py_index.append((tell, vr_bytes))
        vr_tell_next = tell + vr_length
        while tell < vr_tell_next:
            tell = file_object.tell()
            lr_length, lr_bytes = read_lr(file_object)
            if lr_length == EOF:
                break
            py_index.append((tell, lr_bytes))
            lr_tell_next = tell + lr_length
            file_object.seek(lr_tell_next)
            if lr_tell_next == vr_tell_next:
                break
    return py_index


def scan_file_str_index(file_object: typing.BinaryIO) -> typing.List[typing.Tuple[int, str]]:
    """Scans a RP66V1 physical file and returns the index as encoded bytes."""
    py_index = scan_file_index(file_object)
    # print(f'TRACE:\n{py_index}')
    # enc_index = [(tell, common.encode_bytes(data)) for tell, data in py_index]
    enc_index = common.json_encode_seek_read_4_byte_optimised(py_index, True)
    return enc_index


def scan_file_json_index(file_object: typing.BinaryIO) -> bytes:
    """Scans a RP66V1 physical file and returns the index as JSON bytes."""
    enc_index = scan_file_str_index(file_object)
    json_bytes = json.dumps(enc_index)
    return json_bytes


def file_mod_time(file_path: str) -> float:
    return os.stat(file_path).st_mtime_ns / 1e9


class Client:
    LOGGER_PREFIX = 'CLIENT'

    def __init__(self, real_time: bool = True):
        self.connection = connection.Connection(real_time=real_time)
        self._function_map = {
            'seek_read':  self.seek_read,
        }

    def respond_to_server_comms(self, comms_from_server: common.CommunicationJSON) -> common.CommunicationJSON:
        logger.info(f'{self.LOGGER_PREFIX}: respond_to_server_comms():')
        timer = common.Timer()
        comms_to_server = common.CommunicationJSON()
        for function_name, args, server_function in comms_from_server.gen_call():
            function = self._function_map[function_name]
            result = function(*args)
            comms_to_server.add_call(server_function, *result)
        logger.info(f'{self.LOGGER_PREFIX}: respond_to_server_comms(): prepare JSON for server. Time {timer.ms():.3f} (ms).')
        json_data_to_server = comms_to_server.json_dumps()
        json_data_from_server = self.connection.client_to_server(json_data_to_server)
        comms_from_server = common.CommunicationJSON.json_reads(json_data_from_server)
        logger.info(
            f'{self.LOGGER_PREFIX}: respond_to_server_comms(): server response size {len(comms_from_server)}.'
            f' Time {timer.ms():.3f} (ms).'
        )
        return comms_from_server

    def _init_file(self, file_path: str, file_object: typing.BinaryIO) -> bool:
        logger.info(
            f'{self.LOGGER_PREFIX}: _init_file(): "{os.path.basename(file_path)}"'
            f' {os.path.getsize(file_path):,d} bytes.'
        )
        timer = common.Timer()
        total_timer = common.Timer()
        index_as_str = scan_file_str_index(file_object)
        logger.info(
            f'{self.LOGGER_PREFIX}: scan_file_str_index()'
            f' len index_as_str={len(index_as_str):,d}'
            f' time {timer.ms():.3f} (ms).'
        )

        timer = common.Timer()
        # logger.info(f'{self.LOGGER_PREFIX}: Scanning complete {len(json_to_server)} bytes. Adding file to server.')
        comms_to_server = common.CommunicationJSON()
        comms_to_server.add_call('add_file', file_path, file_mod_time(file_path), index_as_str)
        json_data_to_server = comms_to_server.json_dumps()
        logger.info(f'{self.LOGGER_PREFIX}: prepare JSON for server time {timer.ms():.3f} (ms).')
        json_data_from_server = self.connection.client_to_server(json_data_to_server)
        logger.info(f'{self.LOGGER_PREFIX}: the_server.add_file() time {timer.ms():.3f} (ms).')

        comms_from_server = common.CommunicationJSON.json_reads(json_data_from_server)
        while comms_from_server:
            comms_from_server = self.respond_to_server_comms(comms_from_server)
        logger.info(f'{self.LOGGER_PREFIX}: _init_file() total time {total_timer.ms():.3f} (ms).')

    def seek_read(self, file_path: str, mod_time: float,
                  seek_read: typing.List[typing.Tuple[int, int]]) -> \
            typing.Tuple[str, str, float, typing.List[typing.Tuple[int, str]]]:
        """Given a file_path and a list of seek/read values this returns a list of seek/encoded bytes as str."""
        timer = common.Timer()
        ret = []
        bytes_read = 0
        with open(file_path, 'rb') as file_object:
            for seek, read in seek_read:
                file_object.seek(seek)
                ret.append((seek, common.encode_bytes(file_object.read(read))))
                bytes_read += read
        logger.info(f'{self.LOGGER_PREFIX}: seek_read() bytes {bytes_read} time {timer.ms():.3f} (ms).')
        return file_path, file_mod_time(file_path), ret

    def add_file(self, file_path: str):
        logger.info(f'{self.LOGGER_PREFIX}: add_file(): Opening {file_path}')
        timer_overall = common.Timer()
        with open(file_path, 'rb') as file_object:
            result = self._init_file(file_path, file_object)
            # logger.info(f'{self.LOGGER_PREFIX}: File added to server, result {result}.')
            # TODO: Read EFLRs
            # json_data = self.connection.EFLR_id_as_fpos(file_path, client_lib.file_mod_time(file_path), '')
            # eflr_response = json.loads(json_data)
            # for id_as_fpos in eflr_response['data']:
        logger.info(f'{self.LOGGER_PREFIX}: add_file(): execution time {timer_overall.ms():.3f} (ms).')
