.. _testing:

Testing
============

Assuming you have installed ``svfsc`` (instructions are :ref:`here <installation>`).

C++
--------------

Building and running ``main()`` will exercise all the C++ tests (this can take a very long time for a debug version
since there are then very thorough internal integrity checks being run):

.. code-block:: console

    $ cmake-build-release/cppSVF
    Testing SVF and SVFS...
    Testing SVF all...
    Testing SVFS all...
    Function                                                                   ----           Bytes              ms           ms/Mb            Mb/s Test
    virtual TestResult SVFS::TestCaseWrite::run() const                        pass             101        0.014646         152.054             6.6 Special (A)
    virtual TestResult SVFS::TestCaseWrite::run() const                        pass             113        0.001349          12.518            79.9 Special (B)
    virtual TestResult SVFS::TestCaseWrite::run() const                        pass               0        0.000051           0.000             0.0 Write no blocks
    virtual TestResult SVFS::TestCaseWrite::run() const                        pass               4        0.000234          61.342            16.3 Write single block

    8<---- snip ---->8

    TestCount SVFS::_test_perf_write_sim_index_svfs(size_t, size_t, t_test_results &)pass           44000        2.594098          61.821            16.2 Sim SVFS: vr=1000 lr=10
    TestCount SVFS::_test_perf_write_sim_index_svfs(size_t, size_t, t_test_results &)pass          440000       31.918755          76.066            13.1 Sim SVFS: vr=10000 lr=10
    TestCount SVFS::_test_perf_write_sim_index_svfs(size_t, size_t, t_test_results &)pass         1048564       76.019625          76.020            13.2 Sim SVFS: vr=23831 lr=10
    TestCount SVFS::_test_perf_write_sim_index_svfs(size_t, size_t, t_test_results &)pass         4400000      354.623306          84.511            11.8 Sim SVFS: vr=100000 lr=10
    Test results [143] Pass: 143 Fail: 0 - PASSED
    main() execution time: 40.9194 (s)
    Bye bye!

    Process finished with exit code 0

Python
--------------

Running the Python tests:

.. code-block:: console

    $ pytest tests/
    ======================================================================================= test session starts ========================================================================================
    platform darwin -- Python 3.10.8, pytest-7.3.1, pluggy-1.0.0
    benchmark: 4.0.0 (defaults: timer=time.perf_counter disable_gc=False min_rounds=5 min_time=0.000005 max_time=1.0 calibration_precision=10 warmup=False warmup_iterations=100000)
    rootdir: /Users/engun/CLionProjects/cppSVF
    configfile: setup.cfg
    plugins: benchmark-4.0.0
    collected 212 items

    tests/benchmark/test_benchmark_svf.py .sssssssssssssssssssssssssssssssssssssss                                                                                                               [ 18%]
    tests/benchmark/test_benchmark_svfs.py .ssssssssssssssssssss                                                                                                                                 [ 28%]
    tests/unit/test_svf.py ........................................................................................................                                                              [ 77%]
    tests/unit/test_svf_memory.py sssss                                                                                                                                                          [ 80%]
    tests/unit/test_svfs.py ..........................................                                                                                                                           [100%]


    ---------------------------------------------------------------------------------------- benchmark: 2 tests ----------------------------------------------------------------------------------------
    Name (time in ns)          Min                    Max                Mean              StdDev              Median                IQR            Outliers  OPS (Mops/s)            Rounds  Iterations
    ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    test_svfs_ctor        426.0073 (1.0)      72,900.9917 (1.26)     495.6092 (1.0)      808.7077 (1.0)      455.9988 (1.0)      21.0130 (1.0)      159;3722        2.0177 (1.0)       58005           1
    test_svf_ctor         657.0044 (1.54)     57,755.0090 (1.0)      771.3970 (1.56)     977.8003 (1.21)     722.0078 (1.58)     36.0014 (1.71)     245;2141        1.2963 (0.64)      75592           1
    ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    Legend:
      Outliers: 1 Standard Deviation from Mean; 1.5 IQR (InterQuartile Range) from 1st Quartile and 3rd Quartile.
      OPS: Operations Per Second, computed as 1 / Mean
    ================================================================================= 148 passed, 64 skipped in 7.23s ==================================================================================

To run the full test suite including memory tests and benchmarks:

.. code-block:: console

    $ pytest tests/ --runslow --benchmark-sort=name

Building and Testing Everything
----------------------------------

There is a shell script ``build_all.sh`` at the project root that:

- Builds the C++ code from clean and runs all the C++ tests.
- Creates a Python virtual environment for each Python version of interest.
- For each Python version of interest builds the Extension and runs the Python tests, including with ``--runslow``.
- Builds the documentation using ``docs/build_docs.sh`` that creates both Doxygen and Sphinx (HTML, PDF) documentation.
- Creates and checks the Python binary and source distributions.

It takes about five minutes per Python version.
