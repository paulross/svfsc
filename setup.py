#!/bin/env python
import os

from setuptools import setup, find_packages
from distutils.core import Extension

# Makefile:
# CFLAGS = -Wall -Wextra
# CFLAGS_RELEASE = $(CFLAGS) -O2 -DNDEBUG
# CFLAGS_DEBUG = $(CFLAGS) -g3 -O0 -DDEBUG=1
extra_compile_args = [
    '-Wall',
    '-Wextra',
    '-Werror',
    '-Wfatal-errors',
    # Some internal Python library code does not like this.
    '-Wno-c++11-compat-deprecated-writable-strings',
    '-std=c++11',
    # '-Isrc/cpp',
    # We implement mutex with Python's thread locking so we don't want the
    # overhead of C++'s thread locking as well.
    # '-USVF_THREAD_SAFE',

    # Until we use m_coalesce
    '-Wno-unused-private-field',
]

DEBUG = False

if DEBUG:
    extra_compile_args.extend(['-g3', '-O0', '-DDEBUG=1', '-UNDEBUG'])
else:
    extra_compile_args.extend(['-O2', '-UDEBUG', '-DNDEBUG'])

svfs = Extension(
    "svfs",
    sources=[
        'src/cp/cSVFS.cpp',
        'src/cp/svfs_util.cpp',
        'src/cpp/svf.cpp',
        'src/cpp/svfs.cpp',
    ],
    include_dirs=[
        'src/cp',
        'src/cpp',
        'src/util',
        # '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1',
    ],
    library_dirs=[os.getcwd(), ],
    extra_compile_args=extra_compile_args,
    extra_link_args=['-lstdc++'],
    language='c++11',
)

with open('README.rst') as readme_file:
    readme = readme_file.read()

with open('HISTORY.rst') as history_file:
    history = history_file.read()

requirements = [
]

setup_requirements = [
    'pytest-runner',
]

test_requirements = [
    'pytest',
    'pytest-benchmark',
    'hypothesis',
]

setup(
    name='svfs',
    version='0.1.0',
    ext_modules=[svfs, ],
    description="Sparse Virtual File System.",
    long_description=readme + '\n\n' + history,
    author="Paul Ross",
    author_email='apaulross@gmail.com',
    url='https://github.com/paulross',
#     packages=find_packages('src'),
    license="MIT License",
    keywords=['svfs',],
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Natural Language :: English',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
    ],
    test_suite='tests',
    tests_require=test_requirements,
    setup_requires=setup_requirements,
)
