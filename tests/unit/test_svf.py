"""
MIT License

Copyright (c) 2020-2024 Paul Ross

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""
import pickle
import pickletools
import sys
import threading
import time
import typing

import pytest

import svfsc


@pytest.mark.parametrize(
    'attribute',
    (
            'SVFS_CPP_VERSION',
            'SVFS_CPP_VERSION_MAJOR',
            'SVFS_CPP_VERSION_MINOR',
            'SVFS_CPP_VERSION_PATCH',
            'SVFS_CPP_VERSION_SUFFIX',
    )
)
def test_has_versions(attribute):
    assert hasattr(svfsc, attribute)


@pytest.mark.parametrize(
    'attribute, value',
    (
            ('SVFS_CPP_VERSION', '0.3.0'),
            ('SVFS_CPP_VERSION_MAJOR', 0),
            ('SVFS_CPP_VERSION_MINOR', 3),
            ('SVFS_CPP_VERSION_PATCH', 0),
            ('SVFS_CPP_VERSION_SUFFIX', ''),
    )
)
def test_value_versions(attribute, value):
    assert getattr(svfsc, attribute) == value


def test_SVF_ctor():
    svfsc.cSVF('id', 1.0)
    svfsc.cSVF(id='id', mod_time=1.0)


def test_SVF_id():
    s = svfsc.cSVF('id', 1.0)
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
    # Non-overlapping insert - reversed
    (
        (
            (12, b' '),
            (0, b' '),
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
    'Non-overlapping insert - reversed',
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
    s = svfsc.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.blocks() == expected_blocks


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_num_bytes(blocks, expected_blocks):
    s = svfsc.cSVF('id', 1.0)
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
    s = svfsc.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.num_blocks() == len(expected_blocks)


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    INSERT_FPOS_BYTES_EXPECTED_BLOCKS,
    ids=INSERT_FPOS_BYTES_EXPECTED_BLOCKS_IDS,
)
def test_SVF_has_data_true(blocks, expected_blocks):
    s = svfsc.cSVF('id', 1.0)
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
    s = svfsc.cSVF('id', 1.0)
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
    svf = svfsc.cSVF('id')
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
    s = svfsc.cSVF('id', 1.0)
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
                    'cp_SparseVirtualFile_erase(): Can not erase from a SVF. ERROR:'
                    ' SparseVirtualFile::erase(): Non-existent file position 0 at start of block.',
            ),

    ),
    ids=[
        'Simple erase raises',
    ],
)
def test_SVF_erase_raises(blocks, erase_fpos, expected_message):
    s = svfsc.cSVF('id', 1.0)
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
    s = svfsc.cSVF('id', 1.0)
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
    s = svfsc.cSVF('id', 1.0)
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
            #             //        ^==|
            #             //    |++++++|
            #             {"Before, all of one block and after",     {{8, 4}},          4,  15,  {{4,  4}, {12,  3}},},
            (
                    ((8, 4),),
                    4, 15,
                    [(4, 4), (12, 7), ],
            ),
            (
                    ((8, 3,),), 0, 20, [(0, 8,), (8 + 3, 20 - (8 + 3),), ],
            ),
    ),
    ids=[
        'Empty',
        'One block, all up to it',
        'Two blocks, all and beyond',
        "Before, all of one block and after",
        "Example for SVFS",
    ],
)
def test_SVF_need(blocks, need_fpos, need_length, expected_need):
    s = svfsc.cSVF('id', 1.0)
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
    s = svfsc.cSVF('id', 1.0)
    for fpos, length in blocks:
        s.write(fpos, b' ' * length)
    result = s.need(need_fpos, need_length, greedy_length=greedy_length)
    assert result == expected_need


@pytest.mark.parametrize(
    'blocks, seek_reads, expected_need',
    (
            (
                    (),
                    [(0, 6,), ],
                    [(0, 6,), ],
            ),
            (
                    ((4, 8,),),
                    [(0, 16,), ],
                    [(0, 4,), (12, 4), ],
            ),
    ),
    ids=[
        'Empty',
        'One block',
    ],
)
def test_SVF_need_many(blocks, seek_reads, expected_need):
    s = svfsc.cSVF('id', 1.0)
    for fpos, length in blocks:
        s.write(fpos, b' ' * length)
    result = s.need_many(seek_reads)
    assert result == expected_need


@pytest.mark.parametrize(
    'blocks, seek_reads, greedy_length, expected_need',
    (
            (
                    ((4, 8,),),
                    [(0, 16,), ],
                    256,
                    [(0, 256,), ],
            ),
    ),
    ids=[
        'One block greedy=256',
    ],
)
def test_SVF_need_many_greedy(blocks, seek_reads, greedy_length, expected_need):
    s = svfsc.cSVF('id', 1.0)
    for fpos, length in blocks:
        s.write(fpos, b' ' * length)
    result = s.need_many(seek_reads, greedy_length=greedy_length)
    assert result == expected_need


@pytest.mark.parametrize(
    'blocks_need, expected_error',
    (
            (
                    (),
                    'cp_SparseVirtualFile_need_many_internal: seek_reads is not a list.',
            ),
            (
                    [1, ],
                    'cp_SparseVirtualFile_need_many_internal: seek_reads[0] is not a tuple.',
            ),
            (
                    [(1, 2), 1, ],
                    'cp_SparseVirtualFile_need_many_internal: seek_reads[1] is not a tuple.',
            ),
            (
                    [(1, 2, 3), ],
                    'cp_SparseVirtualFile_need_many_internal: seek_reads[0] length 3 is not a tuple of length 2.',
            ),
            (
                    [('1', 2,), ],
                    'cp_SparseVirtualFile_need_many_internal: can not parse list element[0].',
            ),
    ),
    ids=[
        'Not a list',
        'list element[0] not a tuple',
        'list element[1] not a tuple',
        'list element[0] tuple length wrong',
        'list element[0] tuple values not int',
    ],
)
def test_SVF_need_many_raises(blocks_need, expected_error):
    svf = svfsc.cSVF('id', 1.0)
    with pytest.raises(TypeError) as err:
        svf.need_many(blocks_need)
    assert err.value.args[0] == expected_error


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
    svf = svfsc.cSVF('id', 1.0)
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
                    b'\x80\x04\x95Z\x00\x00\x00\x00\x00\x00\x00\x8c\x05'
                    b'svfsc\x94\x8c\x04'
                    b'cSVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94)\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
            (
                    ((1, b' '),),
                    b'\x80\x04\x95c\x00\x00\x00\x00\x00\x00\x00\x8c\x05'
                    b'svfsc\x94\x8c\x04'
                    b'cSVF\x94\x93\x94)\x81\x94}\x94(\x8c\x02'
                    b'id\x94\x8c\x02'
                    b'id\x94\x8c\r'
                    b'file_mod_time\x94G?\xf0\x00\x00\x00\x00\x00\x00\x8c\x06'
                    b'blocks\x94K\x01C\x01 \x94\x86\x94\x85\x94\x8c\x0e'
                    b'pickle_version\x94K\x01ub.',
            ),
            (
                    ((1, b' '), (12, b' '),),
                    b'\x80\x04\x95k\x00\x00\x00\x00\x00\x00\x00\x8c\x05'
                    b'svfsc\x94\x8c\x04'
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
    s = svfsc.cSVF('id', 1.0)
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
    s = svfsc.cSVF('id', 1.0)
    for fpos, data in blocks:
        s.write(fpos, data)
    assert s.blocks() == expected_blocks
    pickle_result = pickle.dumps(s)
    new_s = pickle.loads(pickle_result)
    assert new_s.blocks() == expected_blocks


@pytest.mark.parametrize(
    'block_count, block_size, expected_overhead',
    (
            (1, 1, 109),
            (1, 256, 112),
            (1, 4 * 1024, 112),
            (16, 1, 215),
            (16, 256, 278),
            (16, 4 * 1024, 287),
            (256, 1, 2023),
            (256, 256, 2927),
            (256, 4 * 1024, 3542),
            (4096, 1, 32743),
            (4096, 256, 52982),
            (4096, 4 * 1024, 55622),
    ),
)
def test_SVF_pickle_dumps_size(block_count, block_size, expected_overhead):
    s = svfsc.cSVF('id', 1.0)
    fpos = 0
    data_count = 0
    for i in range(block_count):
        data = b' ' * block_size
        s.write(fpos, data)
        fpos += 1 + block_size
        data_count += block_size
    pickle_result = pickle.dumps(s)
    assert len(pickle_result) - data_count == expected_overhead


def write_to_svf(svf: svfsc.cSVF, values: typing.Tuple[typing.Tuple[int, int], ...], offset: int):
    for fpos, length in values:
        svf.write(fpos + offset, b' ' * length)


@pytest.mark.parametrize(
    'number_of_threads, expected_bytes',
    (
            (0, 1024 ** 2),
            (1, 1024 ** 2),
            (2, 1024 ** 2),
            (4, 1024 ** 2),
            (8, 1024 ** 2),
            (16, 1024 ** 2),
            (32, 1024 ** 2),
            (64, 1024 ** 2),
    ),
)
def test_multi_threaded_write_coalesced_overwrite(number_of_threads, expected_bytes):
    """Tests multi-threaded write() with overwriting a single coalesced 1MB block writing 8 bytes at a time."""
    svf = svfsc.cSVF("Some ID")
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
    # print()
    # print(f'Number of threads: {number_of_threads:3d} time {time_exec * 1000:8.3f} (ms)')
    # print(f'{number_of_threads:d} {time_exec:12.6f}')
    assert svf.num_bytes() == expected_bytes
    assert svf.num_blocks() == 1


@pytest.mark.parametrize(
    'number_of_threads, expected_bytes',
    (
            (0, 1024 ** 2),
            (1, 1024 ** 2),
            (2, 1024 ** 2),
            (4, 1024 ** 2),
            (8, 1024 ** 2),
            (16, 1024 ** 2),
            (32, 1024 ** 2),
            (64, 1024 ** 2),
            (128, 1024 ** 2),
            (256, 1024 ** 2),
    ),
)
def test_multi_threaded_write_un_coalesced(number_of_threads, expected_bytes):
    """Tests multi-threaded write()."""
    svf = svfsc.cSVF("Some ID")
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
    # print()
    # print(f'Number of threads: {number_of_threads:3d} time {time_exec * 1000:8.3f} (ms)')
    # print(f'{number_of_threads:d} {time_exec:12.6f}')
    assert svf.num_bytes() == expected_bytes


@pytest.mark.parametrize(
    'number_of_writes',
    (
            0, 1, 2, 4, 8, 16,
    ),
)
def test_count_write(number_of_writes):
    svf = svfsc.cSVF("Some ID")
    block = b' '
    for i in range(number_of_writes):
        svf.write(0, block)
    assert svf.count_write() == number_of_writes


@pytest.mark.parametrize(
    'block_size, num_blocks',
    (
            (1, 1),
            # (2, 1),
            # (4, 1),
            # (1, 2),
            # (2, 2),
            # (4, 2),
    )
)
def test_SVF_size_of_overhead(block_size, num_blocks):
    s = svfsc.cSVF('id', 1.0)
    block = b' ' * block_size
    fpos = 0
    print()
    print(
        f'#{"block_size":<12} {"num_blocks":12} {"num_bytes()":<12} {"size_of()":>12} {"Overhead":>12} {"Per block":>12}')
    num_blocks = 1
    while num_blocks < 2048 * 8:
        for i in range(num_blocks):
            s.write(fpos, block)
            fpos += block_size
            # Make un-coalesced
            fpos += 1
        print(
            f'{block_size:<12d}'
            f' {num_blocks:12d}'
            f' {s.num_bytes():12d}'
            f' {s.size_of():12d}'
            f' {s.size_of() - s.num_bytes():12d}'
            f' {(s.size_of() - s.num_bytes()) // num_blocks:12d}'
        )
        num_blocks *= 2
    # assert 0


@pytest.mark.parametrize(
    'args, kwargs, expected',
    (
            ([], {}, {'compare_for_diff': True, 'overwrite_on_exit': False},),
            ([True, ], {}, {'compare_for_diff': True, 'overwrite_on_exit': True},),
            ([False, ], {}, {'compare_for_diff': True, 'overwrite_on_exit': False},),
            ([False, False, ], {}, {'compare_for_diff': False, 'overwrite_on_exit': False},),
            ([True, False, ], {}, {'compare_for_diff': False, 'overwrite_on_exit': True},),
            ([False, True, ], {}, {'compare_for_diff': True, 'overwrite_on_exit': False},),
            ([True, True, ], {}, {'compare_for_diff': True, 'overwrite_on_exit': True},),
            ([], {'compare_for_diff': False, 'overwrite_on_exit': True},
             {'compare_for_diff': False, 'overwrite_on_exit': True},),
    )
)
def test_SVF_ctor_config(args, kwargs, expected):
    # print()
    # print(args)
    # print(kwargs)
    svf = svfsc.cSVF('id', 1.0, *args, **kwargs)
    config = svf.config()
    # print(config)
    assert config == expected
    # assert 0


@pytest.mark.parametrize(
    'actions, expected',
    (
            (tuple(), dict()),
            (
                    (
                            ('write', (0, b' '),),
                    ),
                    {0: 0, },
            ),
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (128, b' '),),
                    ),
                    {0: 0, 1: 128},
            ),
            # Overwrite same, no change
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (0, b' '),),
                    ),
                    {1: 0},
            ),
            # Overwrite new with extended block
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (0, b'  '),),
                    ),
                    {1: 0},
            ),
            # Coalesce two blocks
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (1, b' '),),
                    ),
                    {1: 0},
            ),
            # Three separate blocks
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (4, b' '),),
                            ('write', (8, b' '),),
                    ),
                    {0: 0, 1: 4, 2: 8},
            ),
    ),
    ids=[
        'NoBlocks',
        'OneBlock',
        'TwoSeparateBlocks',
        'OverwriteSame',
        'OverwriteExtend',
        'CoalesceTwoBlocks',
        'ThreeSeparateBlocks',
    ],
)
def test_SVF_block_touches(actions, expected):
    svf = svfsc.cSVF('id', 1.0)
    for action, (fpos, data) in actions:
        if action == 'write':
            svf.write(fpos, data)
        elif action == 'read':
            by = svf.read(fpos, len(data))
            assert by == data
        else:
            assert 0
    result = svf.block_touches()
    assert result == expected


def test_SVF_lru_punt_strategy():
    """Example of a LRU cache punting strategy implemented by a caller."""
    svf = svfsc.cSVF('id', 1.0)
    fpos = 0
    block_size = 128
    block_count = 256
    for i in range(block_count):
        svf.write(fpos, b' ' * block_size)
        fpos += block_size
        fpos += 1
    assert svf.block_touch() == block_count
    cache_upper_bound = 1024
    assert svf.num_bytes() >= cache_upper_bound
    # Now the LRU punting strategy
    if svf.num_blocks() > 1 and svf.num_bytes() >= cache_upper_bound:
        touch_fpos_dict = svf.block_touches()
        for touch in sorted(touch_fpos_dict.keys()):
            if svf.num_blocks() > 1 and svf.num_bytes() >= cache_upper_bound:
                svf.erase(touch_fpos_dict[touch])
            else:
                break
    assert svf.num_bytes() < cache_upper_bound


@pytest.mark.parametrize(
    'block_size, block_count, cache_upper_bound, exp_removed, exp_block_count, exp_num_bytes',
    (
            # (128, 256, 1024, (128 * 256 - (7 * 128)), 7, 896),
            (1024 ** 2, 8, 4 * 1024 ** 2, (8 - 3) * 1024 ** 2, 3, 3 * 1024 ** 2),
    )
)
def test_SVF_lru_punt_strategy_builtin(block_size, block_count, cache_upper_bound, exp_removed, exp_block_count,
                                       exp_num_bytes):
    """Example of a LRU cache punting strategy as given."""
    svf = svfsc.cSVF('id', 1.0)
    fpos = 0
    for i in range(block_count):
        svf.write(fpos, b' ' * block_size)
        fpos += block_size
        fpos += 1
    assert svf.num_bytes() == block_count * block_size
    assert svf.num_blocks() == block_count
    assert svf.block_touch() == block_count
    assert svf.num_bytes() >= cache_upper_bound
    # Now punt.
    removed = svf.lru_punt(cache_upper_bound)
    # And test.
    assert removed == exp_removed
    assert svf.num_bytes() < cache_upper_bound
    assert svf.num_blocks() == exp_block_count
    assert svf.num_bytes() == exp_num_bytes


# RSS: 188.6 (Mb)
# SVF: Count writes 129 reads 67,107 bytes: 134,349,986 sizeof: 134,351,564
# SVF blocks [34]: (
SPECIAL_TEST_BLOCKS_A = (
    (0, 1048576),
    (1274964, 1048576),
    (2329191, 3145728),
    (5484979, 10485760),
    (16020006, 1048576),
    (17132997, 1048576),
    (18302437, 5242880),
    (23648788, 5242880),
    (29070964, 2097152),
    (31176230, 2097152),
    (33403482, 4194304),
    (37735482, 4194304),
    (42113503, 2097152),
    (44222699, 4194304),
    (48482288, 1048576),
    (49603931, 11534336),
    (61259039, 2097152),
    (63367944, 2097152),
    (65497682, 18874368),
    (84590243, 3145728),
    (87782535, 8388608),
    (96178624, 7340032),
    (103598694, 5242880),
    (108952894, 6291456),
    (115323719, 8388608),
    (123913655, 2097152),
    (126109371, 3145728),
    (129470964, 1048576),
    (130639513, 2097152),
    (134029850, 1048576),
    (185242180, 1048576),
    (199390964, 1048576),
    (202857928, 1048576),
    (203985588, 132258),
)


# Block touches: {0: 0, 1: 134029850, 2: 185242180, 3: 199390964, 4: 202857928, 5: 203985588, 6: 1274964, 10: 2329191, 20: 16020006, 21: 17132997, 27: 18302437, 32: 23648788, 34: 29070964, 36: 31176230, 40: 33403482, 44: 37735482, 46: 42113503, 50: 48482288, 62: 49603931, 64: 61259039, 66: 63367944, 84: 65497682, 87: 84590243, 95: 87782535, 102: 96178624, 107: 103598694, 113: 108952894, 121: 115323719, 123: 123913655, 126: 129470964, 129: 130639513}
# After punting to 4 * 1024**2
# Memory after punting 116,524,194 bytes:
# RSS: 23.7 (Mb)
# SVF: Count writes 129 reads 67,107 bytes: 17,825,792 sizeof: 17,826,130
# SVF blocks [3]: (
#     (5484979, 10485760)
#     (44222699, 4194304)
#     (126109371, 3145728)
# )
# Block touches: {20: 5484979, 50: 44222699, 126: 126109371}
#
# Why are there three blocks?
#
# Doing it again
# Memory before punt:
# RSS: 24.4 (Mb)
# SVF: Count writes 129 reads 67,107 bytes: 17,825,792 sizeof: 17,826,130
# SVF blocks [3]: (
#     (5484979, 10485760)
#     (44222699, 4194304)
#     (126109371, 3145728)
# )
# Block touches: {20: 5484979, 50: 44222699, 126: 126109371}
# Memory after punting 14,680,064 bytes:
# RSS: 24.4 (Mb)
# SVF: Count writes 129 reads 67,107 bytes: 3,145,728 sizeof: 3,145,986
# SVF blocks [1]: (
#     (126109371, 3145728)
# )
# Block touches: {126: 126109371}


def test_SVF_lru_punt_strategy_builtin_special_a():
    """Special problem with LRU cache punting."""
    svf = svfsc.cSVF('id', 1.0)
    cache_limit = 4 * 1024 ** 2
    for fpos, siz in SPECIAL_TEST_BLOCKS_A:
        svf.write(fpos, b' ' * siz)
    assert svf.num_bytes() == sum(v[1] for v in SPECIAL_TEST_BLOCKS_A)
    assert svf.num_blocks() == len(SPECIAL_TEST_BLOCKS_A)
    assert svf.block_touch() == len(SPECIAL_TEST_BLOCKS_A)
    # Now punt.
    print()
    print(f'Before punt touches: {svf.block_touches()}')
    removed = svf.lru_punt(cache_limit)
    print(f'After punt blocks: {svf.blocks()}')
    print(f'After punt touches: {svf.block_touches()}')
    print(f'After punt num_bytes: {svf.num_bytes()}')
    # And test.
    assert removed == 131_072_000
    assert svf.num_bytes() < cache_limit
    assert svf.num_blocks() == 4
    assert svf.num_bytes() == 3_277_986


# AWSReadEvents [129]:
# File Pos           Size Time (s) Est. Rate (Mb/s)
# 0x00000000    1,048,576    0.782            1.279
# 0x07fd221a    1,048,576    0.301            3.320
# 0x0b0a9244    1,048,576    0.364            2.746
# 0x0be276f4    1,048,576    0.369            2.709
# 0x0c175dc8    1,048,576    0.464            2.155
# 0x0c2892b4      132,258    0.134            0.942
# 0x00137454    1,048,576    1.315            0.760
# 0x00238a67    1,048,576    0.315            3.176
# 0x00338a67    1,048,576    0.395            2.533
# 0x00438a67    1,048,576    0.511            1.959
# 0x0053b1b3    1,048,576    0.464            2.157
# 0x0063b1b3    1,048,576    0.572            1.749
# 0x0073b1b3    1,048,576    0.454            2.202
# 0x0083b1b3    1,048,576    0.572            1.748
# 0x0093b1b3    1,048,576    0.441            2.269
# 0x00a3b1b3    1,048,576    0.538            1.859
# 0x00b3b1b3    1,048,576    0.620            1.613
# 0x00c3b1b3    1,048,576    0.595            1.681
# 0x00d3b1b3    1,048,576    0.491            2.038
# 0x00e3b1b3    1,048,576    0.477            2.098
# 0x00f47226    1,048,576    0.487            2.054
# 0x01056dc5    1,048,576    0.529            1.889
# 0x011745e5    1,048,576    0.524            1.907
# 0x012745e5    1,048,576    0.484            2.067
# 0x013745e5    1,048,576    0.504            1.983
# 0x014745e5    1,048,576    0.519            1.927
# 0x015745e5    1,048,576    0.458            2.183
# 0x0168da14    1,048,576    0.466            2.144
# 0x0178da14    1,048,576    0.643            1.555
# 0x0188da14    1,048,576    0.597            1.674
# 0x0198da14    1,048,576    0.568            1.761
# 0x01a8da14    1,048,576    0.526            1.900
# 0x01bb9674    1,048,576    0.471            2.125
# 0x01cb9674    1,048,576    0.470            2.130
# 0x01dbb626    1,048,576    0.484            2.067
# 0x01ebb626    1,048,576    0.470            2.128
# 0x01fdb25a    1,048,576    0.455            2.197
# 0x020db25a    1,048,576    0.487            2.053
# 0x021db25a    1,048,576    0.465            2.150
# 0x022db25a    1,048,576    0.474            2.110
# 0x023fcc3a    1,048,576    0.588            1.701
# 0x024fcc3a    1,048,576    0.592            1.689
# 0x025fcc3a    1,048,576    0.518            1.932
# 0x026fcc3a    1,048,576    0.521            1.921
# 0x028299df    1,048,576    0.536            1.864
# 0x029299df    1,048,576    0.480            2.082
# 0x02a2c8eb    1,048,576    0.565            1.770
# 0x02b2c8eb    1,048,576    0.603            1.658
# 0x02c2c8eb    1,048,576    0.589            1.697
# 0x02d2c8eb    1,048,576    0.628            1.592
# 0x02e3c7f0    1,048,576    0.534            1.872
# 0x02f4e55b    1,048,576    0.527            1.899
# 0x0304e55b    1,048,576    0.532            1.879
# 0x0314e55b    1,048,576    0.552            1.810
# 0x0324e55b    1,048,576    0.727            1.376
# 0x0334e55b    1,048,576    0.876            1.141
# 0x0344e55b    1,048,576    0.807            1.239
# 0x0354e55b    1,048,576    0.750            1.333
# 0x0364e55b    1,048,576    0.712            1.404
# 0x0374e55b    1,048,576    0.711            1.406
# 0x0384e55b    1,048,576    0.689            1.450
# 0x0394e55b    1,048,576    0.699            1.431
# 0x03a6bd1f    1,048,576    0.650            1.539
# 0x03b6bd1f    1,048,576    0.613            1.632
# 0x03c6eb08    1,048,576    0.590            1.694
# 0x03d6eb08    1,048,576    0.591            1.692
# 0x03e76a52    1,048,576    0.571            1.752
# 0x03f76a52    1,048,576    0.563            1.775
# 0x04076a52    1,048,576    0.512            1.952
# 0x04176a52    1,048,576    0.556            1.798
# 0x04276a52    1,048,576    0.584            1.712
# 0x04376a52    1,048,576    0.473            2.115
# 0x04476a52    1,048,576    0.489            2.046
# 0x04576a52    1,048,576    0.426            2.346
# 0x04676a52    1,048,576    0.533            1.875
# 0x04776a52    1,048,576    0.718            1.392
# 0x04876a52    1,048,576    0.495            2.021
# 0x04976a52    1,048,576    0.601            1.663
# 0x04a76a52    1,048,576    0.579            1.726
# 0x04b76a52    1,048,576    0.555            1.803
# 0x04c76a52    1,048,576    0.847            1.181
# 0x04d76a52    1,048,576    0.549            1.822
# 0x04e76a52    1,048,576    0.521            1.918
# 0x04f76a52    1,048,576    0.516            1.937
# 0x050abea3    1,048,576    0.523            1.911
# 0x051abea3    1,048,576    0.507            1.972
# 0x052abea3    1,048,576    0.536            1.866
# 0x053b7487    1,048,576    0.538            1.858
# 0x054b7487    1,048,576    0.514            1.946
# 0x055b7487    1,048,576    0.623            1.605
# 0x056b7487    1,048,576    0.722            1.385
# 0x057b7487    1,048,576    0.770            1.299
# 0x058b7487    1,048,576    0.725            1.379
# 0x059b7487    1,048,576    0.689            1.451
# 0x05ab7487    1,048,576    0.669            1.494
# 0x05bb91c0    1,048,576    0.676            1.480
# 0x05cb91c0    1,048,576    0.668            1.496
# 0x05db91c0    1,048,576    0.674            1.485
# 0x05eb91c0    1,048,576    0.762            1.313
# 0x05fb91c0    1,048,576    0.689            1.452
# 0x060b91c0    1,048,576    0.713            1.402
# 0x061b91c0    1,048,576    0.642            1.557
# 0x062cca66    1,048,576    0.633            1.579
# 0x063cca66    1,048,576    0.607            1.647
# 0x064cca66    1,048,576    0.612            1.635
# 0x065cca66    1,048,576    0.611            1.636
# 0x066cca66    1,048,576    1.036            0.965
# 0x067e7d3e    1,048,576    0.599            1.670
# 0x068e7d3e    1,048,576    0.570            1.754
# 0x069e7d3e    1,048,576    0.501            1.997
# 0x06ae7d3e    1,048,576    0.523            1.912
# 0x06be7d3e    1,048,576    0.465            2.148
# 0x06ce7d3e    1,048,576    0.458            2.186
# 0x06dfb347    1,048,576    0.442            2.262
# 0x06efb347    1,048,576    0.573            1.744
# 0x06ffb347    1,048,576    0.591            1.693
# 0x070fb347    1,048,576    0.579            1.726
# 0x071fb347    1,048,576    0.517            1.934
# 0x072fb347    1,048,576    0.523            1.911
# 0x073fb347    1,048,576    0.579            1.726
# 0x074fb347    1,048,576    0.608            1.645
# 0x0762c5b7    1,048,576    0.597            1.675
# 0x0772c5b7    1,048,576    0.607            1.647
# 0x078446bb    1,048,576    0.560            1.784
# 0x079446bb    1,048,576    0.526            1.901
# 0x07a446bb    1,048,576    0.525            1.904
# 0x07b791f4    1,048,576    0.572            1.750
# 0x07c96699    1,048,576    0.529            1.889
# 0x07d96699    1,048,576    0.512            1.953
SPECIAL_TEST_BLOCKS_B = (
    (0x00000000, 1_048_576,),
    (0x07fd221a, 1_048_576,),
    (0x0b0a9244, 1_048_576,),
    (0x0be276f4, 1_048_576,),
    (0x0c175dc8, 1_048_576,),
    (0x0c2892b4, 132_258,),
    (0x00137454, 1_048_576,),
    (0x00238a67, 1_048_576,),
    (0x00338a67, 1_048_576,),
    (0x00438a67, 1_048_576,),
    (0x0053b1b3, 1_048_576,),
    (0x0063b1b3, 1_048_576,),
    (0x0073b1b3, 1_048_576,),
    (0x0083b1b3, 1_048_576,),
    (0x0093b1b3, 1_048_576,),
    (0x00a3b1b3, 1_048_576,),
    (0x00b3b1b3, 1_048_576,),
    (0x00c3b1b3, 1_048_576,),
    (0x00d3b1b3, 1_048_576,),
    (0x00e3b1b3, 1_048_576,),
    (0x00f47226, 1_048_576,),
    (0x01056dc5, 1_048_576,),
    (0x011745e5, 1_048_576,),
    (0x012745e5, 1_048_576,),
    (0x013745e5, 1_048_576,),
    (0x014745e5, 1_048_576,),
    (0x015745e5, 1_048_576,),
    (0x0168da14, 1_048_576,),
    (0x0178da14, 1_048_576,),
    (0x0188da14, 1_048_576,),
    (0x0198da14, 1_048_576,),
    (0x01a8da14, 1_048_576,),
    (0x01bb9674, 1_048_576,),
    (0x01cb9674, 1_048_576,),
    (0x01dbb626, 1_048_576,),
    (0x01ebb626, 1_048_576,),
    (0x01fdb25a, 1_048_576,),
    (0x020db25a, 1_048_576,),
    (0x021db25a, 1_048_576,),
    (0x022db25a, 1_048_576,),
    (0x023fcc3a, 1_048_576,),
    (0x024fcc3a, 1_048_576,),
    (0x025fcc3a, 1_048_576,),
    (0x026fcc3a, 1_048_576,),
    (0x028299df, 1_048_576,),
    (0x029299df, 1_048_576,),
    (0x02a2c8eb, 1_048_576,),
    (0x02b2c8eb, 1_048_576,),
    (0x02c2c8eb, 1_048_576,),
    (0x02d2c8eb, 1_048_576,),
    (0x02e3c7f0, 1_048_576,),
    (0x02f4e55b, 1_048_576,),
    (0x0304e55b, 1_048_576,),
    (0x0314e55b, 1_048_576,),
    (0x0324e55b, 1_048_576,),
    (0x0334e55b, 1_048_576,),
    (0x0344e55b, 1_048_576,),
    (0x0354e55b, 1_048_576,),
    (0x0364e55b, 1_048_576,),
    (0x0374e55b, 1_048_576,),
    (0x0384e55b, 1_048_576,),
    (0x0394e55b, 1_048_576,),
    (0x03a6bd1f, 1_048_576,),
    (0x03b6bd1f, 1_048_576,),
    (0x03c6eb08, 1_048_576,),
    (0x03d6eb08, 1_048_576,),
    (0x03e76a52, 1_048_576,),
    (0x03f76a52, 1_048_576,),
    (0x04076a52, 1_048_576,),
    (0x04176a52, 1_048_576,),
    (0x04276a52, 1_048_576,),
    (0x04376a52, 1_048_576,),
    (0x04476a52, 1_048_576,),
    (0x04576a52, 1_048_576,),
    (0x04676a52, 1_048_576,),
    (0x04776a52, 1_048_576,),
    (0x04876a52, 1_048_576,),
    (0x04976a52, 1_048_576,),
    (0x04a76a52, 1_048_576,),
    (0x04b76a52, 1_048_576,),
    (0x04c76a52, 1_048_576,),
    (0x04d76a52, 1_048_576,),
    (0x04e76a52, 1_048_576,),
    (0x04f76a52, 1_048_576,),
    (0x050abea3, 1_048_576,),
    (0x051abea3, 1_048_576,),
    (0x052abea3, 1_048_576,),
    (0x053b7487, 1_048_576,),
    (0x054b7487, 1_048_576,),
    (0x055b7487, 1_048_576,),
    (0x056b7487, 1_048_576,),
    (0x057b7487, 1_048_576,),
    (0x058b7487, 1_048_576,),
    (0x059b7487, 1_048_576,),
    (0x05ab7487, 1_048_576,),
    (0x05bb91c0, 1_048_576,),
    (0x05cb91c0, 1_048_576,),
    (0x05db91c0, 1_048_576,),
    (0x05eb91c0, 1_048_576,),
    (0x05fb91c0, 1_048_576,),
    (0x060b91c0, 1_048_576,),
    (0x061b91c0, 1_048_576,),
    (0x062cca66, 1_048_576,),
    (0x063cca66, 1_048_576,),
    (0x064cca66, 1_048_576,),
    (0x065cca66, 1_048_576,),
    (0x066cca66, 1_048_576,),
    (0x067e7d3e, 1_048_576,),
    (0x068e7d3e, 1_048_576,),
    (0x069e7d3e, 1_048_576,),
    (0x06ae7d3e, 1_048_576,),
    (0x06be7d3e, 1_048_576,),
    (0x06ce7d3e, 1_048_576,),
    (0x06dfb347, 1_048_576,),
    (0x06efb347, 1_048_576,),
    (0x06ffb347, 1_048_576,),
    (0x070fb347, 1_048_576,),
    (0x071fb347, 1_048_576,),
    (0x072fb347, 1_048_576,),
    (0x073fb347, 1_048_576,),
    (0x074fb347, 1_048_576,),
    (0x0762c5b7, 1_048_576,),
    (0x0772c5b7, 1_048_576,),
    (0x078446bb, 1_048_576,),
    (0x079446bb, 1_048_576,),
    (0x07a446bb, 1_048_576,),
    (0x07b791f4, 1_048_576,),
    (0x07c96699, 1_048_576,),
    (0x07d96699, 1_048_576,),
)


def test_SVF_lru_punt_strategy_builtin_special_b():
    """Special problem with LRU cache punting."""
    svf = svfsc.cSVF('id', 1.0)
    cache_limit = 4 * 1024 ** 2
    for fpos, siz in SPECIAL_TEST_BLOCKS_B:
        svf.write(fpos, b' ' * siz)
    assert svf.num_bytes() == sum(v[1] for v in SPECIAL_TEST_BLOCKS_B)
    assert svf.num_blocks() == 34
    assert svf.block_touch() == len(SPECIAL_TEST_BLOCKS_B)
    # Now punt.
    print()
    print(f'Before punt touches: {svf.block_touches()}')
    removed = svf.lru_punt(cache_limit)
    print(f'After punt blocks: {svf.blocks()}')
    print(f'After punt touches: {svf.block_touches()}')
    print(f'After punt num_bytes: {svf.num_bytes()}')
    # And test.
    assert removed == 131_204_258
    assert svf.num_bytes() < cache_limit
    assert svf.num_blocks() == 2
    assert svf.num_bytes() == 3_145_728
