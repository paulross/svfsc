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
            ('SVFS_CPP_VERSION', '0.2.1'),
            ('SVFS_CPP_VERSION_MAJOR', 0),
            ('SVFS_CPP_VERSION_MINOR', 2),
            ('SVFS_CPP_VERSION_PATCH', 2),
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
    print()
    # print(f'Number of threads: {number_of_threads:3d} time {time_exec * 1000:8.3f} (ms)')
    print(f'{number_of_threads:d} {time_exec:12.6f}')
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
                    {2: 0},
            ),
            # Overwrite new with extended block
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (0, b'  '),),
                    ),
                    {2: 0},
            ),
            # Coalesce two blocks
            (
                    (
                            ('write', (0, b' '),),
                            ('write', (1, b' '),),
                    ),
                    {2: 0},
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


def test_SVF_lru_punt_strategy_builtin():
    """Example of a LRU cache punting strategy as given."""
    svf = svfsc.cSVF('id', 1.0)
    fpos = 0
    block_size = 128
    block_count = 256
    for i in range(block_count):
        svf.write(fpos, b' ' * block_size)
        fpos += block_size
        fpos += 1
    assert svf.num_bytes() == block_count * block_size
    assert svf.num_blocks() == block_count
    assert svf.block_touch() == block_count
    cache_upper_bound = 1024
    assert svf.num_bytes() >= cache_upper_bound
    removed = svf.lru_punt(cache_upper_bound)
    assert removed == (block_size * block_count - 896)
    assert svf.num_bytes() < cache_upper_bound
    assert svf.num_bytes() == 896
    assert svf.num_blocks() == 896 // block_size
