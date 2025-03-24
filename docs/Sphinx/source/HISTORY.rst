History
##################

0.4.1 (2025-03-24)
=====================

- Documentation improvements.
- Add AWS cost to simulator.
- Add svfsc.cSVF.clear()
- Support for Python 3.8, 3.9, 3.10, 3.11, 3.12, 3.13.
- Development Status :: 5 - Production/Stable

0.4.0 (2024-02-11)
=====================

- Add counters for blocks/bytes erased and blocks/bytes punted and then their associated APIs.
- Use the ``SVFS_SVF_METHOD_SIZE_T_REGISTER`` macro in CPython to simplify registering CPython methods.
- Fix builds on Linux, mainly compiler flags.
    - Move to -std=c++17 to exploit ``[[nodiscard]]``.
    - Better alignment of compiler flags between CMakeLists.txt and setup.py
- Other minor fixes.
- Because of the extensive use of this in various projects this version 0.4 is moved to production status:
  Development Status :: 5 - Production/Stable

0.3.0 (2024-01-06)
=====================

- Add ``need_many()``.
- Fix bug in ``lru_punt()``.
- Development Status :: 4 - Beta

0.2.2 (2023-12-28)
=====================

- Minor fixes.
- Development Status :: 4 - Beta

0.2.1 (2023-12-27)
=====================

- Include stub file.
- Development Status :: 4 - Beta

0.2.0 (2023-12-24)
=====================

- Add cache punting.
- Make C docstrings type parsable (good for Sphinx) and add a script that can create a mypy stub file.
- Development Status :: 4 - Beta

0.1.2 (2023-10-03)
=====================

- First release.
- Development Status :: 3 - Alpha
