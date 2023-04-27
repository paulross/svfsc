cmake_minimum_required(VERSION 3.13)
project(cppSVF)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_C_FLAGS "-std=c99")

add_compile_definitions(SVF_THREAD_SAFE)
add_compile_definitions(SVFS_THREAD_SAFE)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("debug build")
    add_compile_definitions(DEBUG)
ELSE()
    message("release build")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)


add_compile_options(
    "-Wall" "-Wpedantic" "-Wextra" "-fexceptions"
    "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
)

link_directories(
    /Library/Frameworks/Python.framework/Versions/3.8/lib
)

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
        src/cp/cp_svfs.h)

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


target_link_libraries(cppSVF ${PYTHON_LIBRARIES})

target_compile_options(cppSVF PRIVATE -Wall -Wextra -Wno-c99-extensions -pedantic)# -Werror)
