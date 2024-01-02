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
import sys
import time

import pytest

import svfsc


def test_SVFS_ctor():
    svfsc.cSVFS()


def test_SVFS_insert():
    s = svfsc.cSVFS()
    s.insert('abc', 12.0)
    assert len(s) == 1


INSERT_KEYS_REMOVE_DATA = (
    (tuple(), []),
    (
        (
            ('abc', 1.2),
        ),
        [
            'abc',
        ]
    ),
    (
        (
            ('abc', 1.2),
            ('def', 1.5),
        ),
        [
            'abc', 'def',
        ]
    ),
)


@pytest.mark.parametrize(
    'insert, expected_keys', INSERT_KEYS_REMOVE_DATA
)
def test_SVFS_keys(insert, expected_keys):
    s = svfsc.cSVFS()
    for an_id, a_mod_time in insert:
        s.insert(an_id, a_mod_time)
    assert sorted(s.keys()) == sorted(expected_keys)
    assert len(s) == len(expected_keys)


@pytest.mark.parametrize(
    'insert, expected_keys', INSERT_KEYS_REMOVE_DATA
)
def test_SVFS_len(insert, expected_keys):
    s = svfsc.cSVFS()
    for an_id, a_mod_time in insert:
        s.insert(an_id, a_mod_time)
    assert len(s) == len(expected_keys)


@pytest.mark.parametrize(
    'insert, expected_keys', INSERT_KEYS_REMOVE_DATA
)
def test_SVFS_insert_remove(insert, expected_keys):
    s = svfsc.cSVFS()
    for an_id, a_mod_time in insert:
        s.insert(an_id, a_mod_time)
    for an_id, _a_mod_time in insert:
        s.remove(an_id)
    assert s.keys() == []
    assert len(s) == 0


@pytest.mark.parametrize(
    'insert, expected_keys', INSERT_KEYS_REMOVE_DATA
)
def test_SVFS_has(insert, expected_keys):
    s = svfsc.cSVFS()
    for an_id, a_mod_time in insert:
        s.insert(an_id, a_mod_time)
    for an_id, _a_mod_time in insert:
        assert s.has(an_id)


@pytest.mark.parametrize(
    'blocks, expected_blocks',
    (
            # No inserts
            (
                    tuple(), tuple(),
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
    )
)
def test_SVFS_write(blocks, expected_blocks):
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    for fpos, data in blocks:
        s.write(ID, fpos, data)
    assert s.blocks(ID) == expected_blocks


@pytest.mark.parametrize(
    'blocks, need_fpos, need_length, expected_need',
    (
            # No inserts
            (
                    tuple(), 0, 20, [(0, 20,), ],
            ),
            (
                    ((8, b'   ',),), 0, 20, [(0, 8,), (8 + 3, 20 - (8 + 3),), ],
            ),
    )
)
def test_SVFS_need(blocks, need_fpos, need_length, expected_need):
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    for fpos, data in blocks:
        s.write(ID, fpos, data)
    assert s.need(ID, need_fpos, need_length) == expected_need


@pytest.mark.parametrize(
    'block_size',
    (1, 2, 4, 8, 16, 32, 64),
)
def test_simulate_write_uncoalesced(block_size):
    SIZE = 1024  # * 1024 * 1
    ID = 'abc'
    s = svfsc.cSVFS()
    s.insert(ID, 12.0)
    data = b' ' * block_size
    block_count = SIZE // block_size
    for i in range(block_count):
        fpos = i * block_size + i
        # print(f'Writing to {fpos}')
        s.write(ID, fpos, data)
    assert s.num_bytes(ID) == SIZE
    assert s.count_write(ID) == block_count
    assert len(s.blocks(ID)) == block_count


@pytest.mark.parametrize(
    'block_size',
    (1, 2, 4, 8, 16, 32, 64),
)
def test_simulate_write_coalesced(block_size):
    SIZE = 1024  # * 1024 * 1
    ID = 'abc'
    s = svfsc.cSVFS()
    s.insert(ID, 12.0)
    data = b' ' * block_size
    block_count = SIZE // block_size
    for i in range(block_count):
        fpos = i * block_size
        s.write(ID, fpos, data)
    assert s.num_bytes(ID) == SIZE
    assert s.count_write(ID) == block_count
    assert len(s.blocks(ID)) == 1


ID = 'abc'


@pytest.mark.parametrize(
    'vr_count, lr_count',
    (
            (1, 10,),
            (2, 10,),
            (3, 10,),
            (10, 10,),
            (100, 10,),
            # (1000, 10,),
            # (23831, 10,),
    )
)
def test_sim_write_index(vr_count, lr_count):
    file_system = svfsc.cSVFS()
    file_system.insert(ID, 12.0)
    vr_data = b' ' * 4
    lr_data = b' ' * 4
    count_write = 0
    bytes_write = 0
    # print()
    assert file_system.count_write(ID) == count_write
    for vr in range(vr_count):
        fpos = 80 + vr * 8004
        # print(f'Write vr fpos={fpos}')
        t_vr = time.perf_counter()
        file_system.write(ID, fpos, vr_data)
        # print(f'TRACE: vr write at fpos={fpos:10d} {1e3 * (time.perf_counter() - t_vr):8.3f} (ms)')
        # t_vr = time.perf_counter()
        count_write += 1
        assert file_system.count_write(ID) == count_write
        bytes_write += 4
        assert file_system.bytes_write(ID) == bytes_write
        fpos += 4
        for lrsh in range(lr_count):
            # print(f'Write lr fpos={fpos}')
            # t_lr = time.perf_counter()
            file_system.write(ID, fpos, lr_data)
            # print(f'TRACE:   lr write at fpos={fpos:10d} {1e3 * (time.perf_counter() - t_lr):8.3f} (ms)')
            count_write += 1
            assert file_system.count_write(ID) == count_write
            bytes_write += 4
            assert file_system.bytes_write(ID) == bytes_write
            fpos += 8000 // lr_count
    assert file_system.count_write(ID) == count_write
    assert file_system.bytes_write(ID) == bytes_write
    assert file_system.count_write(ID) == vr_count * lr_count + vr_count


def test_SVFS_num_bytes():
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    s.write(ID, 0, b'    ')
    assert s.num_bytes(ID) == 4


def test_SVFS_erase():
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    s.write(ID, 0, b'    ')
    assert s.num_bytes(ID) == 4
    s.erase(ID, 0)
    assert s.num_bytes(ID) == 0


def test_SVFS_erase_total_size_of():
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    original_size = s.total_size_of()
    s.write(ID, 0, b'    ')
    assert s.total_size_of() > original_size
    s.erase(ID, 0)
    assert s.total_size_of() == original_size


def test_SVFS_erase_total_bytes():
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    s.write(ID, 0, b'    ')
    assert s.total_bytes() == 4
    s.erase(ID, 0)
    assert s.total_bytes() == 0


def test_SVFS_erase_total_blocks():
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    s.write(ID, 0, b'    ')
    assert s.total_blocks() == 1
    s.erase(ID, 0)
    assert s.total_blocks() == 0


def test_SVFS_erase_raises():
    s = svfsc.cSVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    with pytest.raises(IOError) as err:
        s.erase(ID, 0)
    assert err.value.args[0] == (
        'cp_SparseVirtualFileSystem_svf_erase: Can not erase block from a SVF id= '
        '"abc". ERROR: SparseVirtualFile::erase(): Non-existent file position 0 at start of block.'
    )


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
def test_SVFS_ctor_config(args, kwargs, expected):
    # print()
    # print(args)
    # print(kwargs)
    svf = svfsc.cSVFS(*args, **kwargs)
    config = svf.config()
    # print(config)
    assert config == expected
    # assert 0


def test_SVFS_lru_punt():
    """Example of a LRU cache punting strategy as given."""
    # svf = svfsc.cSVF('id', 1.0)
    svfs = svfsc.cSVFS()
    ID = 'abc'
    svfs.insert(ID, 1.0)
    fpos = 0
    block_size = 128
    block_count = 256
    for i in range(block_count):
        svfs.write(ID, fpos, b' ' * block_size)
        fpos += block_size
        fpos += 1
    assert svfs.num_bytes(ID) == block_count * block_size
    assert svfs.num_blocks(ID) == block_count
    assert len(svfs.block_touches(ID)) == block_count
    cache_upper_bound = 1024
    assert svfs.num_bytes(ID) >= cache_upper_bound
    removed = svfs.lru_punt(ID, cache_upper_bound)
    assert removed == (block_size * block_count - 896)
    assert svfs.num_bytes(ID) < cache_upper_bound
    assert svfs.num_bytes(ID) == 896
    assert svfs.num_blocks(ID) == 896 // block_size


def test_SVFS_lru_punt_all():
    """Near duplicate of test_SVFS_lru_punt()."""
    # svf = svfsc.cSVF('id', 1.0)
    svfs = svfsc.cSVFS()
    ID = 'abc'
    svfs.insert(ID, 1.0)
    fpos = 0
    block_size = 128
    block_count = 256
    for i in range(block_count):
        svfs.write(ID, fpos, b' ' * block_size)
        fpos += block_size
        fpos += 1
    assert svfs.num_bytes(ID) == block_count * block_size
    assert svfs.num_blocks(ID) == block_count
    assert len(svfs.block_touches(ID)) == block_count
    cache_upper_bound = 1024
    assert svfs.num_bytes(ID) >= cache_upper_bound
    removed = svfs.lru_punt_all(cache_upper_bound)
    assert removed == (block_size * block_count - 896)
    assert svfs.num_bytes(ID) < cache_upper_bound
    assert svfs.num_bytes(ID) == 896
    assert svfs.num_blocks(ID) == 896 // block_size


def main():
    # test_simulate_write_coalesced(1)
    # test_simulate_write_coalesced(2)
    test_SVFS_num_bytes()
    print('Bye, bye!')
    return 0


if __name__ == '__main__':
    sys.exit(main())
