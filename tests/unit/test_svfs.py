import sys
import time

import pytest

import svfs


def test_SVFS_ctor():
    svfs.SVFS()


def test_SVFS_insert():
    s = svfs.SVFS()
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
    s = svfs.SVFS()
    for an_id, a_mod_time in insert:
        s.insert(an_id, a_mod_time)
    assert sorted(s.keys()) == sorted(expected_keys)
    assert len(s) == len(expected_keys)


@pytest.mark.parametrize(
    'insert, expected_keys', INSERT_KEYS_REMOVE_DATA
)
def test_SVFS_len(insert, expected_keys):
    s = svfs.SVFS()
    for an_id, a_mod_time in insert:
        s.insert(an_id, a_mod_time)
    assert len(s) == len(expected_keys)


@pytest.mark.parametrize(
    'insert, expected_keys', INSERT_KEYS_REMOVE_DATA
)
def test_SVFS_insert_remove(insert, expected_keys):
    s = svfs.SVFS()
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
    s = svfs.SVFS()
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
    s = svfs.SVFS()
    ID = 'abc'
    s.insert(ID, 1.0)
    for fpos, data in blocks:
        s.write(ID, fpos, data)
    assert s.blocks(ID) == expected_blocks


#         for (size_t block_size = 1; block_size <= 256; block_size *= 2) {
#             SparseVirtualFile svf("", 0.0);
#
#             auto time_start = std::chrono::high_resolution_clock::now();
#             for (t_fpos i = 0; i < (1024 * 1024 * 1) / block_size; ++i) {
#                 t_fpos fpos = i * block_size + i;
#                 svf.write(fpos, data, block_size);
#             }
#             std::chrono::duration<double> time_exec = std::chrono::high_resolution_clock::now() - time_start;
#
#             std::ostringstream os;
#             os << "1Mb, " << std::setw(3) << block_size << " sized blocks, uncoalesced";
#             auto result =TestResult(__FUNCTION__, std::string(os.str()), 0, "", time_exec.count(), svf.num_bytes());
#             count.add_result(result.result());
#             results.push_back(result);
#         }


@pytest.mark.parametrize(
    'block_size',
    (1, 2, 4, 8, 16, 32, 64),
)
def test_simulate_write_uncoalesced(block_size):
    SIZE = 1024 #* 1024 * 1
    ID = 'abc'
    s = svfs.SVFS()
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
    SIZE = 1024 #* 1024 * 1
    ID = 'abc'
    s = svfs.SVFS()
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
    file_system = svfs.SVFS()
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
    return file_system.count_write(ID)


def main():
    test_simulate_write_coalesced(1)
    test_simulate_write_coalesced(2)
    print('Bye, bye!')
    return 0


if __name__ == '__main__':
    sys.exit(main())
