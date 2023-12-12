.. _future_work:

Future Work
#################################################

Ideas for Having a Cache Punt Strategy
==========================================


To Coalesce or not Coalesce?
-----------------------------

The current design has the following charateristics:

- The is no punting, the cache always expands.
- Blocks are coalesced where possible. This is enforced by integrity checks that do not allow adjacent blocks.

If we want punting do we continue to coalesce blocks?

To keep blocks un-coalesced allows for a finer grain punting strategy but requires quite a bit of code change
everywhere.

Some Ideas
------------

Add the following to SparseVirtualFileConfig:

.. code-block:: cpp

    /**
     * If \c true then coalesce blocks where possible.
     * This means that no block is ever deleted except directly by the user with \c clear() or \c erase().
     *
     * If \c false then blocks are not coalesced and a punting strategy can be used to flush older blocks.
     * See \c tSparseVirtualFileConfig::size_limit .
     */
    bool coalesce = true;
    /**
     * Blocks will be punted in order of age until the total data size is < size_limit or there is only one block
     * remaining. This is only effective if \c coalesce is \c false as coalesced blocks are never punted.
     * Hmm, could punting happen even with coalesced blocks? In that case how young is the coalesced block?
     * Or punt first then write coalescing as we go - NO, see problems with punting below.
     */
    size_t size_limit = 1024 * 1024;

Attributes are:

.. code-block:: cpp

    /// Typedef for the data.
    typedef std::vector<char> t_val;
    /// Typedef for the map of file blocks <file_position, data>.
    typedef std::map<t_fpos, t_val> t_map;
    /// The actual SVF.
    t_map m_svf;

.. code-block:: cpp

    /// Typedef for the data.
    typedef struct block {
        std::vector<char> data;
        double access_time;
    } t_block;
    /// Typedef for the map of file blocks <file_position, data>.
    typedef std::map<t_fpos, t_block> t_map;
    /// The actual SVF.
    t_map m_svf;

    /// Typedef for the map of file blocks <access_time, file_position>.
    typedef std::map<double, t_fpos> t_block_time_map;
    /// The ordered map of blocks with their access times.
    t_block_time_map m_block_time_map;

Add error condition:

.. code-block:: cpp

    /// A block is not in the m_block_time_map.
    ERROR_BLOCK_NOT_IN_ACCESS_TIME_MAP,

And in integrity:

.. code-block:: cpp

    // Check the entries in block time map.
    if (!m_block_time_map.empty()) {
        for (const auto &iter_block_time: m_block_time_map) {
            if (m_svf.find(iter_block_time.second) == m_svf.end()) {
                return ERROR_BLOCK_NOT_IN_ACCESS_TIME_MAP;
            }
        }
    }

Other API changes/additions:

.. code-block:: cpp

        // std::chrono::system_clock::now()
        void _update_block_time_map(t_fpos fpos);
        size_t _punt_blocks() noexcept;


Alternatively and more simpler perhaps have a separate ``std::map<size_t, t_fpos>``.
The key is written or updated from ``m_count_read`` which is monotonically increasing.
Punting would be from the min of the map upwards.


Problems With Punting
-----------------------

The basic use case is that the caller checks the SVF by calling ``SparseVirtualFile::has()`` and there will be one of
two outcomes:

All the Data is in the Cache
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Then the caller is free to call ``SparseVirtualFile::read()`` which is (must be) guaranteed to succeed.
If this call is immediate (or at least before any other call that might trigger punting) then this does not present a
challenge, merely the block with the data is marked with a fresh access time/integer.
There is no punting required.

If the call between ``SparseVirtualFile::has()`` and ``SparseVirtualFile::read()`` is interleaved with a call that
triggers a punt then the ``SparseVirtualFile::read()`` is not necessarily guaranteed to succeed.
This presents a problem for the caller.

Some of the Data is not in the Cache
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
---------------------------

This problems are especially acute in a multi-threaded environment.

Summary
-----------

The engineering problems are large and, without clear requirements, this will not be attempted right now.

Users can always call ``SparseVirtualFile::clear()`` or ``SparseVirtualFile::erase()`` if they are concerned about
memory usage.
Then they need to know that might cause exceptions in their own thread or another thread that is anticipating a
``SparseVirtualFile::read()`` to succeed.

Here is a way that a caller could manage an SVF, bearing in mind the effect on other threads, if any:

.. code-block:: python

    def reduce_svf(svf: svfsc.cSVF, limit: int):
        for fpos, block_size in svf.blocks():
            if svf.size_of() < limit:
                break
            svf.erase(fpos)
