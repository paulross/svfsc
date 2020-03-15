

import pytest

import svfs


def test_SVFS_ctor():
    svfs.SVFS()


def test_SVFS_insert():
    s = svfs.SVFS()
    s.insert('abc', 12.0)
    assert len(s) == 1




