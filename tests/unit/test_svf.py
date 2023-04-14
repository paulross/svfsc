import pickle
import pickletools
import sys
import time

import pytest

import svfs


def test_SVF_ctor():
    svfs.SVF('id', 1.0)
    svfs.SVF(id='id', mod_time=1.0)


def test_SVF_id():
    s = svfs.SVF('id', 1.0)
    assert s.id() == 'id'


# Tuple of ((fpos, bytes), ...) and the expected blocks() structure ((fpos, length), ...)
INSERT_FPOS_BYTES_EXPECTED_BLOCKS = (
    # No insert
    (
        tuple(), tuple(),
    ),
    # Empty insert
    (
        (
            (0, b''),
        ),
        tuple(),
    ),
    # Single insert
    (
        (
            (0, b' '),
        ),
        (
            (0, 1),
        ),
    ),
    # Duplicate insert
    (
        (
            (0, b' '),
            (0, b' '),
        ),
        (
            (0, 1),
        ),
    ),
    # Non-overlapping insert
    (
        (
            (0, b' '),
            (12, b' '),
        ),
        (
            (0, 1),
            (12, 1),
        ),
    ),
    # Insert with coalesce with previous
    (
        (
            (0, b' '),
            (1, b' '),
        ),
        (
            (0, 2),
        ),
    ),
    # Insert with coalesce with next
    (
        (
            (1, b' '),
            (0, b' '),
        ),
        (
            (0, 2),
        ),
    ),
    # Insert with coalesce with both
    (
        (
            (0, b' '),
            (1, b' '),
            (2, b' '),
        ),
        (
            (0, 3),
        ),
    ),
)
INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS = [
    'No insert',
    'Empty insert',
    'Single insert',
    'Duplicate insert',
    'Non-overlapping insert',
    'Insert coalesce previous',
    'Insert coalesce next',
    'Insert coalesce both',
]


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_write_success(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.blocks() == expected_blocks


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_num_bytes(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    num_bytes = 0
    for _fpos, size in expected_blocks:
        num_bytes += size
    assert s.num_bytes() == num_bytes


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_num_blocks(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.num_blocks() == len(expected_blocks)


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_has_data_true(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    for fpos, length in expected_blocks:
        assert s.has_data(fpos, length)
        assert s.has_data(file_position=fpos, length=length)


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_read(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    for fpos, length in expected_blocks:
        assert s.read(fpos, length) is not None
        assert s.read(file_position=fpos, length=length) is not None


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_need_nothing(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    for fpos, length in expected_blocks:
        assert s.need(fpos, length) == []
        assert s.need(file_position=fpos, length=length) == []


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_need(blocks, expected_blocks):
    # Empty SVF, needs everything.
    s = svfs.SVF('id', 1.0)
    for fpos, length in expected_blocks:
        assert s.need(fpos, length) == [(fpos, length)]
        assert s.need(file_position=fpos, length=length) == [(fpos, length)]


@pytest.mark.parametrize(
    'blocks, expected_pickle_bytes',
    (
            (
                    (),
                    b'\x80\x04\x95X\x00\x00\x00\x00\x00\x00\x00\x8c\x04'
                    b'svfs\x94\x8c\x03'
                    b'SVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94)\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
            (
                    ((1, b' '),),
                    b'\x80\x04\x95a\x00\x00\x00\x00\x00\x00\x00\x8c\x04'
                    b'svfs\x94\x8c\x03'
                    b'SVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94K\x01C\x01 \x94\x86\x94\x85\x94\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
            (
                    ((1, b' '), (12, b' '),),
                    b'\x80\x04\x95i\x00\x00\x00\x00\x00\x00\x00\x8c\x04'
                    b'svfs\x94\x8c\x03'
                    b'SVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94K\x01C\x01 \x94\x86\x94K\x0cC\x01 \x94\x86\x94\x86\x94\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
    ),
    ids=[
        'Empty',
        'One entry',
        'Two entries',
    ],
)
def test_SVF_pickle_dumps(blocks, expected_pickle_bytes):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    result = pickle.dumps(s)
    # print()
    # print(result)
    # pickletools.dis(result)
    # assert 0
    assert result == expected_pickle_bytes

@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_pickle_loads(blocks, expected_blocks):
    s = svfs.SVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.blocks() == expected_blocks
    pickle_result = pickle.dumps(s)
    new_s = pickle.loads(pickle_result)
    assert new_s.blocks() == expected_blocks


def main():
    # test_simulate_write_coalesced(1)
    # test_simulate_write_coalesced(2)
    # test_SVF_num_bytes()
    print('Bye, bye!')
    return 0


if __name__ == '__main__':
    sys.exit(main())
