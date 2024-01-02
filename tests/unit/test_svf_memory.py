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

import psutil
import pytest

import svfsc

# 1000 * 10000 * 1000 is around 10Gb
SLOW_REPEAT = 1000
SLOW_BLOCKS = 10000
SLOW_BLOCK_SIZE = 1000
SLOW_BLOCK_TOTAL = SLOW_REPEAT * SLOW_BLOCKS * SLOW_BLOCK_SIZE


@pytest.mark.slow
def test_memory_SVF_write():
    proc = psutil.Process()
    # print()
    rss_start_overall = proc.memory_info().rss
    for repeat in range(SLOW_REPEAT):
        # rss_start = proc.memory_info().rss
        # print(f'RSS start: {proc.memory_info().rss:,d}', end='')
        s = svfsc.cSVF('id', 1.0)
        for block_index in range(SLOW_BLOCKS):
            # Not coalesced
            s.write(block_index * 2 * SLOW_BLOCK_SIZE, b' ' * SLOW_BLOCK_SIZE)
        # print(
        #     f' SVF: size_of(): {s.size_of():,d}'
        #     f' blocks {s.num_blocks()}'
        #     f' ∆: {proc.memory_info().rss - rss_start:,d}'
        # )
    rss_diff = proc.memory_info().rss - rss_start_overall
    print(
        f'RSS start: {rss_start_overall:,d}'
        f' end: {proc.memory_info().rss:,d}'
        f' ∆: {rss_diff:+,d}')
    assert rss_diff < 40e6
    # assert 0


@pytest.mark.slow
def test_memory_SVF_need():
    proc = psutil.Process()
    # print()
    rss_start_overall = proc.memory_info().rss
    s = svfsc.cSVF('id', 1.0)
    for block_index in range(SLOW_BLOCKS):
        # Not coalesced
        s.write(block_index * 2 * SLOW_BLOCK_SIZE, b' ' * SLOW_BLOCK_SIZE)
    for repeat in range(SLOW_REPEAT * 10):
        s.need(0, 1024 ** 3)
    rss_diff = proc.memory_info().rss - rss_start_overall
    print(
        f'RSS start: {rss_start_overall:,d}'
        f' end: {proc.memory_info().rss:,d}'
        f' ∆: {rss_diff:+,d}')
    assert rss_diff < 100e6


@pytest.mark.slow
def test_memory_SVF_read():
    proc = psutil.Process()
    # print()
    rss_start_overall = proc.memory_info().rss
    for repeat in range(SLOW_REPEAT):
        # rss_start = proc.memory_info().rss
        # print(f'RSS start: {proc.memory_info().rss:,d}', end='')
        s = svfsc.cSVF('id', 1.0)
        for block_index in range(SLOW_BLOCKS):
            # Not coalesced
            s.write(block_index * 2 * SLOW_BLOCK_SIZE, b' ' * SLOW_BLOCK_SIZE)
        for block_index in range(SLOW_BLOCKS):
            s.read(block_index * 2 * SLOW_BLOCK_SIZE, SLOW_BLOCK_SIZE)
        # print(
        #     f' SVF: size_of(): {s.size_of():,d}'
        #     f' blocks {s.num_blocks()}'
        #     f' ∆: {proc.memory_info().rss - rss_start:,d}'
        # )
    rss_diff = proc.memory_info().rss - rss_start_overall
    print(
        f'RSS start: {rss_start_overall:,d}'
        f' end: {proc.memory_info().rss:,d}'
        f' ∆: {rss_diff:+,d}')
    assert rss_diff < 50e6
    # assert 0


@pytest.mark.slow
def test_memory_SVF_pickle_dumps():
    proc = psutil.Process()
    print()
    rss_start_overall = proc.memory_info().rss
    for repeat in range(SLOW_REPEAT):
        # rss_start = proc.memory_info().rss
        # print(f'RSS start: {proc.memory_info().rss:,d}', end='')
        s = svfsc.cSVF('id', 1.0)
        for block_index in range(SLOW_BLOCKS):
            # Not coalesced
            s.write(block_index * 2 * SLOW_BLOCK_SIZE, b' ' * SLOW_BLOCK_SIZE)
        # result = pickle.dumps(s)
        # print(
        #     f' SVF: size_of(): {s.size_of():12,d}'
        #     f' blocks {s.num_blocks():6d}'
        #     f' ∆: {proc.memory_info().rss - rss_start:12,d}'
        #     f' Pickle length: {len(result):12,d}'
        # )
        pickle.dumps(s)
    rss_diff = proc.memory_info().rss - rss_start_overall
    print(
        f'RSS start: {rss_start_overall:,d}'
        f' end: {proc.memory_info().rss:,d}'
        f' ∆: {rss_diff:+,d}')
    # Slightly weird, this diff is always about 120Mb even making SLOW_REPEAT 4000
    assert rss_diff < 400e6
    # assert 0


@pytest.mark.slow
def test_memory_SVF_pickle_loads():
    proc = psutil.Process()
    print()
    rss_start_overall = proc.memory_info().rss
    for repeat in range(SLOW_REPEAT):
        # rss_start = proc.memory_info().rss
        # print(f'RSS start: {proc.memory_info().rss:,d}', end='')
        s = svfsc.cSVF('id', 1.0)
        for block_index in range(SLOW_BLOCKS):
            # Not coalesced
            s.write(block_index * 2 * SLOW_BLOCK_SIZE, b' ' * SLOW_BLOCK_SIZE)
        # result = pickle.dumps(s)
        # print(
        #     f' SVF: size_of(): {s.size_of():12,d}'
        #     f' blocks {s.num_blocks():6d}'
        #     f' ∆: {proc.memory_info().rss - rss_start:12,d}'
        #     f' Pickle length: {len(result):12,d}'
        # )
        pickle_dumps = pickle.dumps(s)
        pickle.loads(pickle_dumps)
    rss_diff = proc.memory_info().rss - rss_start_overall
    print(
        f'RSS start: {rss_start_overall:,d}'
        f' end: {proc.memory_info().rss:,d}'
        f' ∆: {rss_diff:+,d}')
    # assert rss_diff < 300e6
    # assert 0


def test_memory_SVF_write_and_punt():
    """Write a load of blocks (100Mb) then punt down to two blocks (2,000) bytes checking RSS as we go."""
    proc = psutil.Process()
    rss_start = proc.memory_info().rss
    print()
    print(f'RSS start: {rss_start:,d}')
    ID = 'id'
    svf = svfsc.cSVF(ID, 1.0)
    size_of = svf.size_of()
    for block_index in range(SLOW_BLOCKS * 10):
        # Not coalesced
        svf.write(block_index * 2 * SLOW_BLOCK_SIZE, b' ' * SLOW_BLOCK_SIZE)
    d_rss = proc.memory_info().rss - rss_start
    print(
        f'Before punt. SVF: size_of(): {svf.size_of():12,d}'
        f' ∆: {svf.size_of() - size_of:+12,d}'
        f' blocks {svf.num_blocks():8,d}'
        f' bytes {svf.num_bytes():12,d}'
        f' ∆RSS: {d_rss:+12,d}'
    )
    # Check that delta RSS is less than the stored bytes +20MB
    assert d_rss < (svf.num_bytes() + 20_000_000)
    # Now punt down to two blocks, 2000 bytes.
    rss = proc.memory_info().rss
    size_of = svf.size_of()
    svf.lru_punt(2 * SLOW_BLOCK_SIZE + 1)
    d_rss = proc.memory_info().rss - rss
    print(
        f'After punt.  SVF: size_of(): {svf.size_of():12,d}'
        f' ∆: {svf.size_of() - size_of:+12,d}'
        f' blocks {svf.num_blocks():8,d}'
        f' bytes {svf.num_bytes():12,d}'
        f' ∆RSS: {d_rss:+12,d}'
    )
    d_rss = proc.memory_info().rss - rss_start
    assert d_rss < (svf.num_bytes() + 40_000_000)
    print(
        f'RSS start: {rss_start:12,d}'
        f' end: {proc.memory_info().rss:12,d}'
        f' ∆RSS: {d_rss:+12,d}')
    assert d_rss < 40e6
