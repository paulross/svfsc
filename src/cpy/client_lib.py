import json
import logging
import os
import typing

from src.cpy import common

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


def scan_file_json_index(file_object: typing.BinaryIO) -> bytes:
    """Scans a RP66V1 physical file and returns the index as JSON bytes."""
    file_object.seek(0)
    py_index: typing.List[typing.Tuple[int, str]] = [
        (0, common.encode_bytes(file_object.read(80))),
    ]
    while True:
        tell = file_object.tell()
        vr_length, vr_bytes = read_vr(file_object)
        if vr_length == EOF:
            break
        py_index.append((tell, common.encode_bytes(vr_bytes)))
        vr_tell_next = tell + vr_length
        while tell < vr_tell_next:
            tell = file_object.tell()
            lr_length, lr_bytes = read_lr(file_object)
            if lr_length == EOF:
                break
            py_index.append((tell, common.encode_bytes(lr_bytes)))
            lr_tell_next = tell + lr_length
            file_object.seek(lr_tell_next)
            if lr_tell_next == vr_tell_next:
                break
    json_bytes = json.dumps(py_index)
    return json_bytes


def file_mod_time(file_path: str) -> float:
    return os.stat(file_path).st_mtime_ns / 1e9
