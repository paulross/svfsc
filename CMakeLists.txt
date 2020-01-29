cmake_minimum_required(VERSION 3.13)
project(cppSVF)

set(CMAKE_CXX_STANDARD 17)

IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message("debug build")
    add_compile_definitions(DEBUG)
ELSE()
    message("release build")
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)


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
        src/cpp/tests/test.h
        src/cpp/tests/test.cpp
        src/cpp/util/SaveStreamState.h
)

include_directories(
        src/cpp
        src/cpp/tests
        src/cpp/util
)

FIND_PACKAGE(PythonLibs 3.8 REQUIRED)
IF(PYTHONLIBS_FOUND)
    message(status " Python: ${PYTHON_INCLUDE_DIRS}")
    INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_DIRS}")
ELSE()
    MESSAGE(FATAL_ERROR "Unable to find Python libraries.")
ENDIF()


target_link_libraries(cppSVF python3.8)
