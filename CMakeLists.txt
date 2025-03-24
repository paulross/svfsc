cmake_minimum_required(VERSION 3.13)
project(cppSVF)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(SVF_THREAD_SAFE)
add_compile_definitions(SVFS_THREAD_SAFE)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("debug build")
    add_compile_definitions(DEBUG)
ELSE()
    message("release build")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

# To deal with:
# ld: warning: ignoring file /usr/local/Frameworks/Python.framework/Versions/3.11/lib/libpython3.11.dylib, building for macOS-arm64 but attempting to link with file built for macOS-x86_64
set(CMAKE_OSX_ARCHITECTURES "x86_64")

if(CMAKE_CXX_COMPILER_LOADED)
    message(STATUS "          Compiler path: ${CMAKE_CXX_COMPILER}")
    message(STATUS "            Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "       Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "Compiler is part of GCC: ${CMAKE_COMPILER_IS_GNUCXX}")
else()
    message(FATAL_ERROR "Compiler is not loaded.")
endif()

# Need to match with setup.py except where there Python specific compiler option such as
# '-Wno-c++11-compat-deprecated-writable-strings'
# extra_compile_args = [
#    ...,
#]
# See also https://stackoverflow.com/questions/724664/python-distutils-how-to-get-a-compiler-that-is-going-to-be-used
if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(
        # Additional flags to satisfy Linux/gcc 11.4.0
        "-Wno-missing-field-initializers"
        "-Wno-cast-function-type"
        "-Wno-unknown-pragmas"
    )
endif ()

# Need to match with setup.py except where there Python specific compiler option such as
# '-Wno-c++11-compat-deprecated-writable-strings'
# extra_compile_args = [
#    ...,
#]
add_compile_options(
    # Make all warnings into errors.
    "-Werror"
    # Fail at first error.
    "-Wfatal-errors"
    # ISO C/C++ compatibility.
    "-Wpedantic"
    # All warnings.
    "-Wall"
    # Addition warnings on top of -Wall
    "-Wextra"

    # Enable exception handling, we use these, possibly not needed as out CPython code is C++ but it is not harmful.
    "-fexceptions"
    "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
)

#link_directories(
#    /Library/Frameworks/Python.framework/Versions/3.11/lib
#)

add_executable(
        cppSVF
        main.cpp
        src/cpp/svf.h
        src/cpp/svf.cpp
        src/cpp/tests/test_svf.h
        src/cpp/tests/test_svf.cpp
        src/cpp/tests/test_svfs.h
        src/cpp/tests/test_svfs.cpp
        src/cpp/tests/test.h
        src/cpp/tests/test.cpp
        src/cpp/util/SaveStreamState.h
        src/cpp/svfs.cpp
        src/cpp/svfs.h
        src/cp/svfs_util.cpp
        src/cp/svfs_util.h
        src/cp/cSVFS.cpp
        # NOTE: src/cp/cSVFS.cpp #includes the following files.
#        src/cp/_cSVF.cpp
#        src/cp/_cSVFS.cpp
        src/cp/cp_svfs.h
        src/cpp/cpp_svfs.h
        src/cpp/cpp_svfs.cpp
        src/cpp/tests/test_cpp_svfs.h
        src/cpp/tests/test_cpp_svfs.cpp
)

include_directories(
        src/cpp
        src/cpp/tests
        src/cpp/util
)

FIND_PACKAGE(PythonLibs 3.10 REQUIRED)
IF(PYTHONLIBS_FOUND)
    message(status "    Python libraries: ${PYTHON_LIBRARIES}")
    message(status " Python include dirs: ${PYTHON_INCLUDE_DIRS}")
    message(status "      Python version: ${PYTHONLIBS_VERSION_STRING}")
    message(status "      Python library: ${PYTHON_LIBRARY}")
    message(status "  Python include dir: ${PYTHON_INCLUDE_DIR}")
    message(status " Python include dirs: ${PYTHON_INCLUDE_DIRs}")
    INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_DIRS}")
ELSE()
    MESSAGE(FATAL_ERROR "Unable to find Python libraries.")
ENDIF()

link_directories(${PYTHON_LIBRARY})

target_link_libraries(cppSVF ${PYTHON_LIBRARIES})

target_compile_options(cppSVF PRIVATE -Wall -Wextra -Wno-c99-extensions -pedantic)# -Werror)
