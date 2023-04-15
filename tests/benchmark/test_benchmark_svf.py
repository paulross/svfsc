import pytest

import svfs

ID = 'abc'

def ctor():
    return svfs.SVF(id=ID, mod_time=1.0)


def test_svf_ctor(benchmark):
    benchmark(ctor)


def _simulate_write_uncoalesced(size, block_size):
    # SIZE = 1024 #* 1024 * 1
    s = svfs.SVF(ID, 12.0)
    data = b' ' * block_size
    block_count = size // block_size
    for i in range(block_count):
        fpos = i * block_size + i
        s.write(fpos, data)
    return s


@pytest.mark.parametrize(
    'size, block_size',
    (
        (1024, 1,),
        (1024, 2,),
        (1024, 4,),
        (1024, 8,),
        (1024, 16,),
        (1024, 32,),
        (1024, 64,),
    )
)
def test_svf_sim_write_uncoal(size, block_size, benchmark):
    s = benchmark(_simulate_write_uncoalesced, size, block_size)
    assert s.num_bytes() == size
    assert s.count_write() == size // block_size
    assert len(s.blocks()) == size // block_size


def _simulate_write_coalesced(size, block_size):
    # SIZE = 1024 #* 1024 * 1
    s = svfs.SVF(ID, 12.0)
    data = b' ' * block_size
    block_count = size // block_size
    for i in range(block_count):
        fpos = i * block_size
        s.write(fpos, data)
    return s


@pytest.mark.parametrize(
    'size, block_size',
    (
        (1024, 1,),
        (1024, 2,),
        (1024, 4,),
        (1024, 8,),
        (1024, 16,),
        (1024, 32,),
        (1024, 64,),
    )
)
def test_svf_sim_write_coal(size, block_size, benchmark):
    s = benchmark(_simulate_write_coalesced, size, block_size)
    assert s.num_bytes() == size
    assert s.count_write() == size // block_size
    assert len(s.blocks()) == 1


#     // Simulate writing a low level RP66V1 index. Total bytes written around 1Mb.
#     // Represented file size is about 190 Mb
#     // 23831 * (4 + 10 * 4) is close to 1Mb
#     TestCount test_perf_write_sim_index(t_test_results &results) {
#         TestCount count;
#         SparseVirtualFile svf("", 0.0);
#         auto time_start = std::chrono::high_resolution_clock::now();
#
#         for (size_t vr = 0; vr < 23831; ++vr) {
#             t_fpos fpos = 80 + vr * 8004;
#             svf.write(fpos, data, 4);
#             fpos += 4;
#             for (int lrsh = 0; lrsh < 10; ++lrsh) {
#                 svf.write(fpos, data, 4);
#                 fpos += 800;
#             }
#         }


def _sim_write_index(vr_count, lr_count):
    file_system = svfs.SVF(ID, 12.0)
    vr_data = b' ' * 4
    lr_data = b' ' * 4
    count_write = 0
    bytes_write = 0
    # print()
    # t = time.perf_counter()
    assert file_system.count_write(ID) == count_write
    for vr in range(vr_count):
        fpos = 80 + vr * 8004
        # print(f'Write vr fpos={fpos}')
        file_system.write(fpos, vr_data)
        count_write += 1
        assert file_system.count_write() == count_write
        bytes_write += 4
        assert file_system.bytes_write() == bytes_write
        fpos += 4
        # print(f'TRACE: vr write at {time.perf_counter() - t}')
        # t = time.perf_counter()
        for lrsh in range(lr_count):
            # print(f'Write lr fpos={fpos}')
            file_system.write(fpos, lr_data)
            count_write += 1
            assert file_system.count_write() == count_write
            bytes_write += 4
            assert file_system.bytes_write() == bytes_write
            fpos += 8000 // lr_count
    assert file_system.count_write() == count_write
    assert file_system.bytes_write() == bytes_write
    assert file_system.count_write() == vr_count * lr_count + vr_count
    return file_system.count_write()


# Simulate writing a low level RP66V1 index. Total bytes written around 1Mb.
# Visible records are 8000 bytes long
# Represented file size is about 190 Mb
# 23831 * (4 + 10 * 4) is close to 1Mb
@pytest.mark.parametrize(
    'vr_count, lr_count',
    (
        (1, 10,),
        (10, 10,),
        (100, 10,),
        (1000, 10,),
        (10000, 10,),
        (23831, 10,),
    )
)
def test_svf_sim_write_index(vr_count, lr_count, benchmark):
    result = benchmark(_sim_write_index, vr_count, lr_count)
    # assert result == vr_count * lr_count

