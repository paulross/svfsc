"""
MIT License

Copyright (c) 2020-2023 Paul Ross

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
import pytest

import svfs

ID = 'abc'


def ctor():
    return svfs.cSVF(id=ID, mod_time=1.0)


def test_svf_ctor(benchmark):
    benchmark(ctor)


def _simulate_write_uncoalesced(data, block_count):
    fpos = 0
    svf = svfs.cSVF(ID)
    for i in range(block_count):
        assert svf.count_write() == i
        svf.write(fpos, data)
        fpos += len(data) + 1
    return svf


@pytest.mark.slow
@pytest.mark.parametrize(
    'size, block_size',
    (
            (1024 * 1024, 1,),
            (1024 * 1024, 2,),
            (1024 * 1024, 4,),
            (1024 * 1024, 8,),
            (1024 * 1024, 16,),
            (1024 * 1024, 32,),
            (1024 * 1024, 64,),
            (1024 * 1024, 128,),
            (1024 * 1024, 256,),
            (1024 * 1024, 512,),
            (1024 * 1024, 1024,),
    ),
    ids=[
        '0001',
        '0002',
        '0004',
        '0008',
        '0016',
        '0032',
        '0064',
        '0128',
        '0256',
        '0512',
        '1024',
    ]
)
def test_svf_write_uncoal(size, block_size, benchmark):
    data = b' ' * block_size
    block_count = size // block_size
    svf = benchmark(_simulate_write_uncoalesced, data, block_count)
    assert svf.num_bytes() == size
    assert svf.count_write() == block_count
    assert len(svf.blocks()) == block_count


def _simulate_write_coalesced(data, block_count):
    fpos = 0
    svf = svfs.cSVF(ID)
    for i in range(block_count):
        # assert svf.count_write() == i
        svf.write(fpos, data)
        fpos += len(data)
    return svf


@pytest.mark.slow
@pytest.mark.parametrize(
    'size, block_size',
    (
            (1024 * 1024, 1,),
            (1024 * 1024, 2,),
            (1024 * 1024, 4,),
            (1024 * 1024, 8,),
            (1024 * 1024, 16,),
            (1024 * 1024, 32,),
            (1024 * 1024, 64,),
            (1024 * 1024, 128,),
            (1024 * 1024, 256,),
            (1024 * 1024, 512,),
            (1024 * 1024, 1024,),
    ),
    ids=[
        '0001',
        '0002',
        '0004',
        '0008',
        '0016',
        '0032',
        '0064',
        '0128',
        '0256',
        '0512',
        '1024',
    ]
)
def test_svf_write_coal(size, block_size, benchmark):
    data = b' ' * block_size
    block_count = size // block_size
    svf = benchmark(_simulate_write_coalesced, data, block_count)
    assert svf.num_bytes() == size
    assert svf.count_write() == block_count
    assert len(svf.blocks()) == 1


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
    file_system = svfs.cSVF(ID, 12.0)
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
@pytest.mark.slow
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
def test_svf_write_index(vr_count, lr_count, benchmark):
    result = benchmark(_sim_write_index, vr_count, lr_count)
    # assert result == vr_count * lr_count


def _write_uncoal(block_size: int, block_count: int) -> svfs.cSVF:
    data = b' ' * block_size
    svf = svfs.cSVF("ID")
    fpos = 0
    for i in range(block_count):
        svf.write(fpos, data)
        fpos += block_size + 1
    return svf


def _simulate_read(svf: svfs.cSVF):
    for fpos, length in svf.blocks():
        svf.read(fpos, length)


@pytest.mark.slow
@pytest.mark.parametrize(
    'size, block_size',
    (
            (1024 * 1024, 1,),
            (1024 * 1024, 2,),
            (1024 * 1024, 4,),
            (1024 * 1024, 8,),
            (1024 * 1024, 16,),
            (1024 * 1024, 32,),
            (1024 * 1024, 64,),
            (1024 * 1024, 128,),
            (1024 * 1024, 256,),
            (1024 * 1024, 512,),
            (1024 * 1024, 1024 * 1024,),
    ),
    ids=[
        '0001',
        '0002',
        '0004',
        '0008',
        '0016',
        '0032',
        '0064',
        '0128',
        '0256',
        '0512',
        '1e6_',
    ]
)
def test_svf_read_uncoal(size, block_size, benchmark):
    block_count = size // block_size
    svf = _write_uncoal(block_size, block_count)
    assert svf.num_bytes() == size
    assert svf.count_write() == block_count
    assert len(svf.blocks()) == block_count
    benchmark(_simulate_read, svf)


def _simulate_need(svf: svfs.cSVF, fpos: int, need_size: int, greedy_length: int):
    svf.need(fpos, need_size, greedy_length)


@pytest.mark.slow
@pytest.mark.parametrize(
    'size, block_size, need_fpos, need_size',
    (
            # File offset at 0
            # 1K need
            (1024 * 1024, 1, 0, 1024),
            (1024 * 1024, 2, 0, 1024),
            (1024 * 1024, 4, 0, 1024),
            (1024 * 1024, 8, 0, 1024),
            (1024 * 1024, 16, 0, 1024),
            (1024 * 1024, 32, 0, 1024),
            (1024 * 1024, 64, 0, 1024),
            (1024 * 1024, 128, 0, 1024),
            (1024 * 1024, 256, 0, 1024),
            (1024 * 1024, 512, 0, 1024),
            (1024 * 1024, 1024 * 1024, 0, 1024),
            # 64K need
            (1024 * 1024, 1, 0, 64 * 1024),
            (1024 * 1024, 2, 0, 64 * 1024),
            (1024 * 1024, 4, 0, 64 * 1024),
            (1024 * 1024, 8, 0, 64 * 1024),
            (1024 * 1024, 16, 0, 64 * 1024),
            (1024 * 1024, 32, 0, 64 * 1024),
            (1024 * 1024, 64, 0, 64 * 1024),
            (1024 * 1024, 128, 0, 64 * 1024),
            (1024 * 1024, 256, 0, 64 * 1024),
            (1024 * 1024, 512, 0, 64 * 1024),
            (1024 * 1024, 1024 * 1024, 0, 64 * 1024),
            # 1M need
            (1024 * 1024, 1, 0, 1024 * 1024),
            (1024 * 1024, 2, 0, 1024 * 1024),
            (1024 * 1024, 4, 0, 1024 * 1024),
            (1024 * 1024, 8, 0, 1024 * 1024),
            (1024 * 1024, 16, 0, 1024 * 1024),
            (1024 * 1024, 32, 0, 1024 * 1024),
            (1024 * 1024, 64, 0, 1024 * 1024),
            (1024 * 1024, 128, 0, 1024 * 1024),
            (1024 * 1024, 256, 0, 1024 * 1024),
            (1024 * 1024, 512, 0, 1024 * 1024),
            (1024 * 1024, 1024 * 1024, 0, 1024 * 1024),
            # File offset at 512K
            # 1K need
            (1024 * 1024, 1, 512 * 1024, 1024),
            (1024 * 1024, 2, 512 * 1024, 1024),
            (1024 * 1024, 4, 512 * 1024, 1024),
            (1024 * 1024, 8, 512 * 1024, 1024),
            (1024 * 1024, 16, 512 * 1024, 1024),
            (1024 * 1024, 32, 512 * 1024, 1024),
            (1024 * 1024, 64, 512 * 1024, 1024),
            (1024 * 1024, 128, 512 * 1024, 1024),
            (1024 * 1024, 256, 512 * 1024, 1024),
            (1024 * 1024, 512, 512 * 1024, 1024),
            (1024 * 1024, 1024 * 1024, 512 * 1024, 1024),
            # 64K need
            (1024 * 1024, 1, 512 * 1024, 64 * 1024),
            (1024 * 1024, 2, 512 * 1024, 64 * 1024),
            (1024 * 1024, 4, 512 * 1024, 64 * 1024),
            (1024 * 1024, 8, 512 * 1024, 64 * 1024),
            (1024 * 1024, 16, 512 * 1024, 64 * 1024),
            (1024 * 1024, 32, 512 * 1024, 64 * 1024),
            (1024 * 1024, 64, 512 * 1024, 64 * 1024),
            (1024 * 1024, 128, 512 * 1024, 64 * 1024),
            (1024 * 1024, 256, 512 * 1024, 64 * 1024),
            (1024 * 1024, 512, 512 * 1024, 64 * 1024),
            (1024 * 1024, 1024 * 1024, 512 * 1024, 64 * 1024),
            # 1M need
            (1024 * 1024, 1, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 2, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 4, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 8, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 16, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 32, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 64, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 128, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 256, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 512, 512 * 1024, 1024 * 1024),
            (1024 * 1024, 1024 * 1024, 512 * 1024, 1024 * 1024),
    ),
    ids=[
        # File offset at 0
        '0001K,0000K,0001',
        '0001K,0000K,0002',
        '0001K,0000K,0004',
        '0001K,0000K,0008',
        '0001K,0000K,0016',
        '0001K,0000K,0032',
        '0001K,0000K,0064',
        '0001K,0000K,0128',
        '0001K,0000K,0256',
        '0001K,0000K,0512',
        '0001K,0000K,1e6_',
        '0064K,0000K,0001',
        '0064K,0000K,0002',
        '0064K,0000K,0004',
        '0064K,0000K,0008',
        '0064K,0000K,0016',
        '0064K,0000K,0032',
        '0064K,0000K,0064',
        '0064K,0000K,0128',
        '0064K,0000K,0256',
        '0064K,0000K,0512',
        '0064K,0000K,1e6_',
        '1024K,0000K,0001',
        '1024K,0000K,0002',
        '1024K,0000K,0004',
        '1024K,0000K,0008',
        '1024K,0000K,0016',
        '1024K,0000K,0032',
        '1024K,0000K,0064',
        '1024K,0000K,0128',
        '1024K,0000K,0256',
        '1024K,0000K,0512',
        '1024K,0000K,1e6_',
        # File offset at 512K
        '0001K,0512K,0001',
        '0001K,0512K,0002',
        '0001K,0512K,0004',
        '0001K,0512K,0008',
        '0001K,0512K,0016',
        '0001K,0512K,0032',
        '0001K,0512K,0064',
        '0001K,0512K,0128',
        '0001K,0512K,0256',
        '0001K,0512K,0512',
        '0001K,0512K,1e6_',
        '0064K,0512K,0001',
        '0064K,0512K,0002',
        '0064K,0512K,0004',
        '0064K,0512K,0008',
        '0064K,0512K,0016',
        '0064K,0512K,0032',
        '0064K,0512K,0064',
        '0064K,0512K,0128',
        '0064K,0512K,0256',
        '0064K,0512K,0512',
        '0064K,0512K,1e6_',
        '1024K,0512K,0001',
        '1024K,0512K,0002',
        '1024K,0512K,0004',
        '1024K,0512K,0008',
        '1024K,0512K,0016',
        '1024K,0512K,0032',
        '1024K,0512K,0064',
        '1024K,0512K,0128',
        '1024K,0512K,0256',
        '1024K,0512K,0512',
        '1024K,0512K,1e6_',
    ]
)
def test_svf_need_uncoal(size, block_size, need_fpos, need_size, benchmark):
    block_count = size // block_size
    svf = _write_uncoal(block_size, block_count)
    assert svf.num_bytes() == size
    assert svf.count_write() == block_count
    assert len(svf.blocks()) == block_count
    benchmark(_simulate_need, svf, need_fpos, need_size, 0)


def _simulate_need_all(svf: svfs.cSVF, fpos: int, need_size: int, greedy_length: int):
    while fpos < svf.last_file_position():
        if not svf.has_data(fpos, need_size):
            svf.need(fpos, need_size, greedy_length)
        fpos += need_size


@pytest.mark.slow
@pytest.mark.parametrize(
    'size, block_size, need_size, greedy_length',
    (
            # 8 byte reads
            (64 * 1024, 1, 8, 1),
            (64 * 1024, 1, 8, 512),
            (64 * 1024, 1, 8, 1024),
            (64 * 1024, 1, 8, 1024 * 32),
            (64 * 1024, 1, 8, 1024 * 64),
            # 32 byte reads
            (64 * 1024, 1, 32, 1),
            (64 * 1024, 1, 32, 512),
            (64 * 1024, 1, 32, 1024),
            (64 * 1024, 1, 32, 1024 * 32),
            (64 * 1024, 1, 32, 1024 * 64),
            # 128 byte reads
            (64 * 1024, 1, 128, 1),
            (64 * 1024, 1, 128, 512),
            (64 * 1024, 1, 128, 1024),
            (64 * 1024, 1, 128, 1024 * 32),
            (64 * 1024, 1, 128, 1024 * 64),
            # 512 byte reads
            (64 * 1024, 1, 512, 1),
            (64 * 1024, 1, 512, 512),
            (64 * 1024, 1, 512, 1024),
            (64 * 1024, 1, 512, 1024 * 32),
            (64 * 1024, 1, 512, 1024 * 64),
    ),
    ids=[
        'Greedy:008,00001',
        'Greedy:008,00512',
        'Greedy:008,01024',
        'Greedy:008,32768',
        'Greedy:008,65536',
        'Greedy:032,00001',
        'Greedy:032,00512',
        'Greedy:032,01024',
        'Greedy:032,32768',
        'Greedy:032,65536',
        'Greedy:128,00001',
        'Greedy:128,00512',
        'Greedy:128,01024',
        'Greedy:128,32768',
        'Greedy:128,65536',
        'Greedy:512,00001',
        'Greedy:512,00512',
        'Greedy:512,01024',
        'Greedy:512,32768',
        'Greedy:512,65536',
    ]
)
def test_svf_need_uncoal_greedy(size, block_size, need_size, greedy_length, benchmark):
    """Populate a SVF with 64k one byte un-coalesced blocks then check need() on everything with different
    need sizes and greedy_lengths."""
    block_count = size // block_size
    svf = _write_uncoal(block_size, block_count)
    assert svf.num_bytes() == size
    assert svf.count_write() == block_count
    assert len(svf.blocks()) == block_count
    benchmark(_simulate_need_all, svf, 0, need_size, greedy_length)
