# Auto-generated from svfsc version 0.2.0 at 2023-12-22 15:17:46.887228 UTC
import typing
import datetime

PY_THREAD_SAFE: int
SVFS_CPP_VERSION: str
SVFS_CPP_VERSION_MAJOR: int
SVFS_CPP_VERSION_MINOR: int
SVFS_CPP_VERSION_PATCH: int
SVFS_CPP_VERSION_SUFFIX: str
SVFS_THREAD_SAFE: int
SVF_THREAD_SAFE: int

class cSVF:
    def block_touch(self) -> int: ...
    def block_touches(self) -> typing.Dict[int, int]: ...
    def blocks(self) -> typing.Tuple[typing.Tuple[int, int], ...]: ...
    def bytes_read(self) -> int: ...
    def bytes_write(self) -> int: ...
    def config(self) -> typing.Dict[str, bool]: ...
    def count_read(self) -> int: ...
    def count_write(self) -> int: ...
    def erase(self, file_position: int) -> None: ...
    def file_mod_time(self) -> float: ...
    def file_mod_time_matches(self, file_mod_time: float) -> bool: ...
    def has_data(self, file_position: int, length: int) -> bool: ...
    def id(self) -> str: ...
    def last_file_position(self) -> int: ...
    def lru_punt(self, cache_size_upper_bound: int) -> int: ...
    def need(self, file_position: int, length: int, greedy_length: int = 0) -> typing.Tuple[typing.Tuple[int, int], ...]: ...
    def num_blocks(self) -> int: ...
    def num_bytes(self) -> int: ...
    def read(self, file_position: int, length: int) -> bytes: ...
    def size_of(self) -> int: ...
    def time_read(self) -> typing.Optional[datetime.datetime]: ...
    def time_write(self) -> typing.Optional[datetime.datetime]: ...
    def write(self, file_position: int, data: bytes) -> None: ...

class cSVFS:
    def blocks(self, id: str) -> typing.Tuple[typing.Tuple[int, int], ...]: ...
    def bytes_read(self, id: str) -> int: ...
    def bytes_write(self, id: str) -> int: ...
    def config(self) -> typing.Dict[str, bool]: ...
    def count_read(self, id: str) -> int: ...
    def count_write(self, id: str) -> int: ...
    def erase(self, id: str, file_position: int) -> None: ...
    def file_mod_time(self, id: str) -> float: ...
    def file_mod_time_matches(self, id: str) -> bool: ...
    def has(self, id: str) -> bool: ...
    def has_data(self, id: str, file_position: int, length: int) -> bool: ...
    def insert(self, id: str) -> None: ...
    def keys(self) -> typing.List[str]: ...
    def need(self, id: str, file_position: int, length: int, greedy_length: int = 0) -> typing.Tuple[typing.Tuple[int, int], ...]: ...
    def num_blocks(self, id: str) -> int: ...
    def num_bytes(self, id: str) -> int: ...
    def read(self, id: str, file_position: int, length: int) -> bytes: ...
    def remove(self, id: str) -> None: ...
    def size_of(self, id: str) -> int: ...
    def time_read(self, id: str) -> typing.Optional[datetime.datetime]: ...
    def time_write(self, id: str) -> typing.Optional[datetime.datetime]: ...
    def total_blocks(self) -> int: ...
    def total_bytes(self) -> int: ...
    def total_size_of(self) -> int: ...
    def write(self, id: str, file_position: int, data: bytes) -> None: ...
