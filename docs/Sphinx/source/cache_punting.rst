.. _cache_punting:

Cache Punting
#############

The current design has the following characteristics:

- The is no punting, the cache always expands.
- Blocks are coalesced where possible. This is enforced by integrity checks that do not allow adjacent blocks.
- The SVF is not a read-through cache, it is only advisory to the caller.

Problems With Punting
=====================

The basic use case is that the caller checks the SVF by calling ``SparseVirtualFile::has()`` and there will be one of
two outcomes:

All the Data is in the Cache
----------------------------

Then the caller is free to call ``SparseVirtualFile::read()`` which is (must be) guaranteed to succeed.
If this call is immediate (or at least before any other call that might trigger punting) then this does not present a
challenge, merely the block with the data is marked with a fresh access time/integer.
There is no punting required.

If the call between ``SparseVirtualFile::has()`` and ``SparseVirtualFile::read()`` is interleaved with a call that
triggers a punt then the ``SparseVirtualFile::read()`` is not necessarily guaranteed to succeed.
This presents a problem for the caller.

Some of the Data is not in the Cache
------------------------------------

If ``SparseVirtualFile::has()`` returns ``false`` then the canonical behaviour of the caller is then to:

- Call ``SparseVirtualFile::need()``
- Go and get the data somehow.
- Call ``SparseVirtualFile::write()``
- Call ``SparseVirtualFile::read()`` which gives the caller a copy of the data.

This sequence is expected to always succeed.
However we must consider that other sequences of events might exist, for example the caller decides not to write or
even read.
Alternatively a caller might go part way through that sequence but then call ``SparseVirtualFile::read()`` of another,
or smaller, part of the file.
This call might touch blocks previously touched by the original ``SparseVirtualFile::need()`` call.

One approach is that any punting must happen at the end of a ``SparseVirtualFile::read()`` once the copy has been made.
This would mean that the memory used might exceed the cache limit in the intervening ``SparseVirtualFile::write()``.

Another approach would be to mark the blocks somehow when ``SparseVirtualFile::need()`` is called and then unmark them
when ``SparseVirtualFile::read()`` is called affecting those blocks.
Meanwhile any punting during a ``SparseVirtualFile::write()`` has to ignore marked blocks.

Both mean that a second ``SparseVirtualFile::read()`` to the same place is *not* guaranteed to succeed.

Multi-threading Guarantees
--------------------------

This problems are especially acute in a multi-threaded environment.


Compromise Design
=================

The compromise is to provide APIs that can assist the caller who has full knowledge the callers state is and all of its
threads.
The caller is responsible for deciding when and how much to punt.

The SVF marks each block with and integer that represents the age of last use (the so-called 'touch' integer).
This integer starts at 0 and monotonically increases with each read/write so older blocks have lower values.
When blocks are coalesced the resulting block is marked as being newest regardless of the touch values of the previous
blocks.
The caller can implement Least Recently Used (LRU) cacheing by pruning older blocks which have low touch values until
the cache is the required size.

Relevant APIs
--------------

C++
^^^

.. code-block:: cpp

    // Gives exact number of data bytes held.
    [[nodiscard]] size_t num_bytes() const noexcept;
    // Number of blocks used.
    [[nodiscard]] size_t num_blocks() const noexcept;
    // The latest value of the block touch.
    [[nodiscard]] t_block_touch block_touch() const noexcept;
    // std::map<touch, file_position>.
    [[nodiscard]] t_block_touches block_touches() const noexcept;

Python
^^^^^^

.. code-block:: python

    def num_bytes() -> int: # Gives exact number of data bytes held.
        pass
    def num_blocks(): # Number of blocks used.
        pass
    def block_touch() -> int: # The latest value of the block touch.
        pass
    def block_touches() -> typing.Dict[int, int]: # Dict of {touch, file_position, ...}
        pass

Typical Implementations
-----------------------

The caller might implement a LRU cache punting strategy like this:

C++
^^^

See the test function ``Test::test_lru_block_punting_a``.
Note the use of integrity checks and a thread mutex to ensure consistency.

.. code-block:: cpp

    /* Remove old blocks until the total bytes is < the cache_upper_bound.
     * This always maintains at least one block.
     */
    void lru_punt(SVFS::SparseVirtualFile svf, size_t cache_upper_bound) {
        SVF_ASSERT(integrity() == ERROR_NONE);
    #ifdef SVF_THREAD_SAFE
        std::lock_guard<std::mutex> mutex(m_mutex);
    #endif
        if (svf.num_blocks() > 1 and svf.num_bytes() >= cache_upper_bound) {
            auto touch_fpos_map = svf.block_touches();
            for (const auto &iter: touch_fpos_map) {
                if (svf.num_blocks() > 1 and svf.num_bytes() >= cache_upper_bound) {
                    svf.erase(iter.second);
                } else {
                    break;
                }
            }
        }
    }

Python
^^^^^^

The equivalent in Python would be along the lines of:

.. code-block:: python

    def lru_punt(svf: svfsc.cSVF, cache_upper_bound: int):
        if svf.num_blocks() > 1 and svf.num_bytes() >= cache_upper_bound:
            touch_fpos_dict = svf.block_touches()
            for touch in sorted(touch_fpos_dict.keys()):
                if svf.num_blocks() > 1 and svf.num_bytes() >= cache_upper_bound:
                    svf.erase(touch_fpos_dict[touch])
                else:
                    break
