
Technical Notes
################

Pickling
========

A Sparse Virtual File (``svfs.cSVF``) can be pickled:

.. code-block:: python

    import pickle
    import svfs

    svf = svfs.cSVF('id')
    svf.write(21, b'ABCDEF')
    pickle_result = pickle.dumps(svf)
    # Save pickle_result somewhere

And to un-pickle:

.. code-block:: python

    import pickle
    import svfs

    svf = svfs.cSVF('id')
    svf.write(21, b'ABCDEF')
    pickle_result = pickle.dumps(s)
    new_svf = pickle.loads(pickle_result)
    assert id(new_svf) != id(svf)
    assert new_svf.id() == svf.id()
    assert new_svf.file_mod_date() == svf.file_mod_date()
    assert new_svf.blocks() == svf.blocks()

Pickling is *versioned* by an integer number.
As `svfs` progresses this ensures that pickles from previous ``svfs`` versions can be detected and either rejected or read as modified.

