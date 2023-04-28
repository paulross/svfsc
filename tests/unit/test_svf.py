import pickle
import pickletools
import sys
import threading
import time
import typing

import psutil
import pytest

import svfs


def test_SVF_ctor():
    svfs.cSVF('id', 1.0)
    svfs.cSVF(id='id', mod_time=1.0)


def test_SVF_id():
    s = svfs.cSVF('id', 1.0)
    assert s.id() == 'id'


# Tuple of ((fpos, bytes), ...) and the expected blocks() structure ((fpos, length), ...)
INSERT_FPOS_BYTES_EXPECTED_BLOCKS = (
    # No insert
    (
        tuple(), tuple(),
    ),
    # Empty insert, do nothing.
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
    s = svfs.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.blocks() == expected_blocks


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_num_bytes(blocks, expected_blocks):
    s = svfs.cSVF('id', 1.0)
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
    s = svfs.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.num_blocks() == len(expected_blocks)


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_has_data_true(blocks, expected_blocks):
    s = svfs.cSVF('id', 1.0)
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
    s = svfs.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    for fpos, length in expected_blocks:
        assert s.read(fpos, length) is not None
        assert s.read(file_position=fpos, length=length) is not None


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    (
            (
                    ((0, 1024), (291_809_396, 1024)),
                    (),
            ),
    ),
    # ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_read_special(blocks, expected_blocks):
    """This bug from the simulator:

    2023-04-25 12:05:24,776 -             simulator.py#71   - INFO     - CLIENT:  blocks was: ['(0 : 1,024 : 1,024)', '(291,809,396 : 1,024 : 291,810,420)']
    2023-04-25 12:05:24,776 -             simulator.py#72   - INFO     - CLIENT: demands fpos      291,810,392 length  2,429 (     291,812,821)
    2023-04-25 12:05:24,776 -             simulator.py#81   - INFO     - CLIENT:   needs fpos      291,810,420 length  2,401 (     291,812,821)
    2023-04-25 12:05:24,799 -             simulator.py#90   - INFO     - CLIENT:   wrote fpos      291,810,420 length  2,401 (     291,812,821)
    2023-04-25 12:05:24,799 -             simulator.py#92   - ERROR    - CLIENT: demands fpos      291,810,392 length  2,429 (     291,812,821)
    2023-04-25 12:05:24,799 -             simulator.py#96   - ERROR    - CLIENT:  blocks now: ['(0 : 1,024 : 1,024)', '(291,809,396 : 3,397 : 291,812,793)']
    """
    svf = svfs.cSVF('id')
    for fpos, length in blocks:
        svf.write(fpos, b' ' * length)
    assert svf.blocks() == ((0, 1_024), (291_809_396, 1_024))
    assert not svf.has_data(291_810_392, 2_429)
    assert svf.need(291_810_392, 2_429) == [(291_810_420, 2_401), ]
    svf.write(291_810_420, b' ' * 2_401)
    # FIXED: Why is the above reporting this wrong block? Off by 28 bytes.
    # assert svf.blocks() == ((0, 1_024), (291_809_396, 3_397))
    # Actual (correct) value in this test.
    assert svf.blocks() == ((0, 1_024), (291_809_396, 3_425))
    assert svf.has_data(291_810_392, 2_429)


@pytest.mark.parametrize(
    'blocks, erase_fpos',
    (
            (
                    ((12, b' '),),
                    12,
            ),

    ),
    ids=[
        'Simple erase',
    ],
)
def test_SVF_erase(blocks, erase_fpos):
    s = svfs.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.has_data(erase_fpos, 1)
    s.erase(erase_fpos)
    assert not s.has_data(erase_fpos, 1)


@pytest.mark.parametrize(
    'blocks, erase_fpos, expected_message',
    (
            (
                    ((12, b' '),),
                    0,
                    'cp_SparseVirtualFile_erase()#407: Can not erase from a SVF. ERROR:'
                    ' SparseVirtualFile::erase(): Non-existent file position 0.',
            ),

    ),
    ids=[
        'Simple erase raises',
    ],
)
def test_SVF_erase_raises(blocks, erase_fpos, expected_message):
    s = svfs.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    with pytest.raises(IOError) as err:
        s.erase(erase_fpos)
    assert err.value.args[0] == expected_message


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_need_nothing(blocks, expected_blocks):
    s = svfs.cSVF('id', 1.0)
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
def test_SVF_need_all(blocks, expected_blocks):
    # Empty SVF, needs everything.
    s = svfs.cSVF('id', 1.0)
    for fpos, length in expected_blocks:
        assert s.need(fpos, length) == [(fpos, length)]
        assert s.need(file_position=fpos, length=length) == [(fpos, length)]


@pytest.mark.parametrize(
    'blocks, need_fpos, need_length, expected_need',
    (
            (
                    (),
                    0, 6,
                    [(0, 6), ],
            ),
            (
                    ((6, 12),),
                    0, 6,
                    [(0, 6), ],
            ),
            (
                    ((6, 12), (20, 8),),
                    0, 32,
                    [(0, 6), (18, 2), (28, 4), ],
            ),
    ),
    ids=[
        'Empty',
        'One block, all up to it',
        'Two blocks, all and beyond'
    ],
)
def test_SVF_need(blocks, need_fpos, need_length, expected_need):
    s = svfs.cSVF('id', 1.0)
    for fpos, length in blocks:
        s.write(fpos, b' ' * length)
    result = s.need(need_fpos, need_length)
    assert result == expected_need


@pytest.mark.parametrize(
    'blocks, need_fpos, need_length, greedy_length, expected_need',
    (
            (
                    (),
                    0, 6, 0,
                    [(0, 6), ],
            ),
            (
                    (),
                    0, 6, 32,
                    [(0, 32), ],
            ),
            # Based on C++ tests
            #             TestCaseNeedGreedy(
            #                     "Need (greedy=0)",
            #                     {{8,  4}, {16, 4}, {32, 4}},
            #                     8, 40, 0,
            #                     {{12, 4}, {20, 12}, {36, 12},}
            #             ),
            (
                    ((8, 4), (16, 4), (32, 4),),
                    8, 40, 0,
                    [(12, 4), (20, 12), (36, 12), ],
            ),
            #             TestCaseNeedGreedy(
            #                     "Need (greedy=8)",
            #                     {{8,  4}, {16, 4}, {32, 4}},
            #                     8, 40, 8,
            #                     {{12, 20}, {36, 12},}
            #             ),
            (
                    ((8, 4), (16, 4), (32, 4),),
                    8, 40, 64,
                    [(12, 64), ],
            ),
    ),
    ids=[
        'Empty greedy=0',
        'Empty greedy=32',
        'Three blocks greedy=0',
        'Three blocks greedy=64',
    ],
)
def test_SVF_need_greedy(blocks, need_fpos, need_length, greedy_length, expected_need):
    s = svfs.cSVF('id', 1.0)
    for fpos, length in blocks:
        s.write(fpos, b' ' * length)
    result = s.need(need_fpos, need_length, greedy_length=greedy_length)
    assert result == expected_need


def test_SVF_need_write_special():
    """Special case with error found in RaPiVot tiff_dump.py when using a SVF:

    TRACE: Needs: [(3102, 12)] blocks were ((0, 8), (3028, 74), (3214, 19)):
    TRACE: Needs: block was (0, 8, 8) (77, 77, 0, 42, 0, 0, 11, 212)
    TRACE: Needs: block was (3028, 74, 3102) (0, 15, 1, 0, 0, 3, 0, 0, 0, 1, 0, 157, 0, 0, 1, 1, 0, 3, 0, 0, 0, 1, 0, 151, 0, 0, 1, 2, 0, 3, 0, 0, 0, 1, 0, 1, 0, 0, 1, 3, 0, 3, 0, 0, 0, 1, 0, 1, 0, 0, 1, 6, 0, 3, 0, 0, 0, 1, 0, 3, 0, 0, 1, 13, 0, 2, 0, 0, 0, 19, 0, 0, 12, 142)
    TRACE: Needs: block was (3214, 19, 3233) (112, 97, 108, 101, 116, 116, 101, 45, 49, 99, 45, 49, 98, 46, 116, 105, 102, 102, 0)
    TRACE: Needs: Write: 3102 12 (3114) (1, 17, 0, 4, 0, 0, 0, 1, 0, 0, 0, 8)
    TRACE: Needs: [(3102, 12)] blocks now ((0, 8), (3028, 105)):
    TRACE: Needs: block now (0, 8, 8) (77, 77, 0, 42, 0, 0, 11, 212)
    TRACE: Needs: block now (3028, 105, 3133) (0, 15, 1, 0, 0, 3, 0, 0, 0, 1, 0, 157, 0, 0, 1, 1, 0, 3, 0, 0, 0, 1, 0, 151, 0, 0, 1, 2, 0, 3, 0, 0, 0, 1, 0, 1, 0, 0, 1, 3, 0, 3, 0, 0, 0, 1, 0, 1, 0, 0, 1, 6, 0, 3, 0, 0, 0, 1, 0, 3, 0, 0, 1, 13, 0, 2, 0, 0, 0, 19, 0, 0, 12, 142, 1, 17, 0, 4, 0, 0, 0, 1, 0, 0, 0, 8, 112, 97, 108, 101, 116, 116, 101, 45, 49, 99, 45, 49, 98, 46, 116, 105, 102, 102, 0)
    """
    svf = svfs.cSVF('id', 1.0)
    svf.write(0, b'A' * 8)
    svf.write(3028, b'B' * 74)
    svf.write(3214, b'C' * 19)
    assert svf.blocks() == ((0, 8), (3028, 74), (3214, 19))
    # This should not coalesce the last two blocks as 3102 + 12 = 3114 which is less than 3214
    # It should extend the second block as 3028 + 74 = 3102
    svf.write(3102, b'D' * 12)
    # 3028 + 86 = 3114
    assert svf.blocks() == ((0, 8), (3028, 86), (3214, 19))


@pytest.mark.parametrize(
    'blocks, expected_pickle_bytes',
    (
            (
                    (),
                    b'\x80\x04\x95Y\x00\x00\x00\x00\x00\x00\x00\x8c\x04'
                    b'svfs\x94\x8c\x04'
                    b'cSVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94)\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
            (
                    ((1, b' '),),
                    b'\x80\x04\x95b\x00\x00\x00\x00\x00\x00\x00\x8c\x04'
                    b'svfs\x94\x8c\x04'
                    b'cSVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94K\x01C\x01 \x94\x86\x94\x85\x94\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
            (
                    ((1, b' '), (12, b' '),),
                    b'\x80\x04\x95j\x00\x00\x00\x00\x00\x00\x00\x8c\x04'
                    b'svfs\x94\x8c\x04'
                    b'cSVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
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
    s = svfs.cSVF('id', 1.0)
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
    s = svfs.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.blocks() == expected_blocks
    pickle_result = pickle.dumps(s)
    new_s = pickle.loads(pickle_result)
    assert new_s.blocks() == expected_blocks


def write_to_svf(svf: svfs.cSVF, values: typing.Tuple[typing.Tuple[int, int], ...], offset: int):
    for fpos, length in values:
        svf.write(fpos + offset, b' ' * length)


@pytest.mark.parametrize(
    'number_of_threads, expected_bytes',
    (
            (0, 1024**2),
            (1, 1024**2),
            (2, 1024**2),
            (4, 1024**2),
            (8, 1024**2),
            (16, 1024**2),
            (32, 1024**2),
            (64, 1024**2),
    ),
)
def test_multit_hreaded_write_coalesced_overwrite(number_of_threads, expected_bytes):
    """Tests multi-threaded write() with overwriting a single coalesced 1MB block writing 8 bytes at a time."""
    svf = svfs.cSVF("Some ID")
    # Blocks are adjacent
    blocks = tuple((fpos, 8) for fpos in range(0, 1024 * 1024, 8))
    if number_of_threads:
        threads = [
            threading.Thread(
                name='write[{:6d}]'.format((i + 2) * 1000),
                target=write_to_svf,
                # offset 0 so each thread overwrites the block.
                args=(svf, blocks, 0)
            ) for i in range(number_of_threads)
        ]
        time_start = time.perf_counter()
        for thread in threads:
            thread.start()
        main_thread = threading.current_thread()
        for t in threading.enumerate():
            if t is not main_thread:
                t.join()
        time_exec = time.perf_counter() - time_start
    else:
        time_start = time.perf_counter()
        write_to_svf(svf, blocks, 0)
        time_exec = time.perf_counter() - time_start
    print()
    # print(f'Number of threads: {number_of_threads:3d} time {time_exec * 1000:8.3f} (ms)')
    print(f'{number_of_threads:d} {time_exec:12.6f}')
    assert svf.num_bytes() == expected_bytes
    assert svf.num_blocks() == 1

@pytest.mark.parametrize(
    'number_of_threads, expected_bytes',
    (
            (0, 1024**2),
            (1, 1024**2),
            (2, 1024**2),
            (4, 1024**2),
            (8, 1024**2),
            (16, 1024**2),
            (32, 1024**2),
            (64, 1024**2),
            (128, 1024**2),
            (256, 1024**2),
    ),
)
def test_multi_threaded_write_un_coalesced(number_of_threads, expected_bytes):
    """Tests multi-threaded write()."""
    svf = svfs.cSVF("Some ID")
    limit = 1024 * 1024
    if number_of_threads > 1:
        limit //= number_of_threads
    blocks = tuple((fpos * 2, 8) for fpos in range(0, limit, 8))
    if number_of_threads:
        threads = [
            threading.Thread(
                name='write[{:6d}]'.format((i + 2) * 1000),
                target=write_to_svf,
                args=(svf, blocks, i * 2_000_000)
            ) for i in range(number_of_threads)
        ]
        time_start = time.perf_counter()
        for thread in threads:
            thread.start()
        main_thread = threading.current_thread()
        for t in threading.enumerate():
            if t is not main_thread:
                t.join()
        time_exec = time.perf_counter() - time_start
    else:
        time_start = time.perf_counter()
        write_to_svf(svf, blocks, 0)
        time_exec = time.perf_counter() - time_start
    print()
    # print(f'Number of threads: {number_of_threads:3d} time {time_exec * 1000:8.3f} (ms)')
    print(f'{number_of_threads:d} {time_exec:12.6f}')
    assert svf.num_bytes() == expected_bytes


@pytest.mark.parametrize(
    'number_of_writes',
    (
        0, 1, 2, 4, 8, 16,
    ),
)
def test_count_write(number_of_writes):
    svf = svfs.cSVF("Some ID")
    block = b' '
    for i in range(number_of_writes):
        svf.write(0, block)
    assert svf.count_write() == number_of_writes
