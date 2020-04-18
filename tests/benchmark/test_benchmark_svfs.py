
import pytest

import svfs


def ctor():
    return svfs.SVFS()


# def test_ctor(benchmark):
#     benchmark(ctor)


ID = 'abc'

def _simulate_write_uncoalesced(size, block_size):
    # SIZE = 1024 #* 1024 * 1
    s = svfs.SVFS()
    s.insert(ID, 12.0)
    data = b' ' * block_size
    block_count = size // block_size
    for i in range(block_count):
        fpos = i * block_size + i
        s.write(ID, fpos, data)
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
def test_sim_write_uncoal(size, block_size, benchmark):
    s = benchmark(_simulate_write_uncoalesced, size, block_size)
    assert s.num_bytes(ID) == size
    assert s.count_write(ID) == size // block_size
    assert len(s.blocks(ID)) == size // block_size


def _simulate_write_coalesced(size, block_size):
    # SIZE = 1024 #* 1024 * 1
    s = svfs.SVFS()
    s.insert(ID, 12.0)
    data = b' ' * block_size
    block_count = size // block_size
    for i in range(block_count):
        fpos = i * block_size
        s.write(ID, fpos, data)
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
def test_sim_write_coal(size, block_size, benchmark):
    s = benchmark(_simulate_write_coalesced, size, block_size)
    assert s.num_bytes(ID) == size
    assert s.count_write(ID) == size // block_size
    assert len(s.blocks(ID)) == 1

