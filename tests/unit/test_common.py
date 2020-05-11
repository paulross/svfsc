

import pytest

from src.cpy import common


def test_seek_read_load():
    sr = common.SeekRead()
    sr.append((8, 4))
    assert len(sr) == 1


SEEK_READ_JSON = (
    (
        [],
        '[]',
    ),
    (
        [
            [8, 4],
        ],
        '[[8, 4]]',
    ),
    (
        [
            [8, 4],
            [128, 40],
        ],
        '[[8, 4], [128, 40]]',
    ),
)


@pytest.mark.parametrize(
    'seek_reads, expected_json_str',
    SEEK_READ_JSON,
)
def test_seek_read_to_json(seek_reads, expected_json_str):
    sr = common.SeekRead()
    # print(seek_reads)
    for seek, read in seek_reads:
        sr.append((seek, read))
    result = sr.to_json()
    assert result == expected_json_str


@pytest.mark.parametrize(
    'expected_seek_reads, json_str',
    SEEK_READ_JSON,
)
def test_seek_read_from_json(expected_seek_reads, json_str):
    sr = common.SeekRead.from_json(json_str)
    assert sr.seek_read == expected_seek_reads

