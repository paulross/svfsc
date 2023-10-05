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
    # We implement mutex with Python's thread locking:
    # AcquireLock _lock(self);
    # So we don't want the overhead of C++'s thread locking as well.
    '-USVF_THREAD_SAFE',
    '-USVFS_THREAD_SAFE',
]

DEBUG = False

if DEBUG:
    extra_compile_args.extend(['-g3', '-O0', '-DDEBUG=1', '-UNDEBUG'])
else:
    extra_compile_args.extend(['-O2', '-UDEBUG', '-DNDEBUG'])

svfs_extension = Extension(
    "svfsc",
    sources=[
        'src/cp/cSVFS.cpp',
        'src/cp/svfs_util.cpp',
        'src/cpp/cpp_svfs.cpp',
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
]

setup(
    name='svfsc',
    version='0.1.2',
    ext_modules=[svfs_extension, ],
    description="Sparse Virtual File System Cache implemented in C++.",
    long_description=readme + '\n\n' + history,
    long_description_content_type='text/x-rst',
    author="Paul Ross",
    author_email='apaulross@gmail.com',
    url='https://github.com/paulross',
    #     packages=find_packages('src'),
    license="MIT License",
    keywords=['svf', 'svfs','svfsc',],
    # https://pypi.org/classifiers/
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Natural Language :: English',
        'Operating System :: OS Independent',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Topic :: Software Development :: Libraries',
    ],
    test_suite='tests',
    tests_require=test_requirements,
    setup_requires=setup_requirements,
)
