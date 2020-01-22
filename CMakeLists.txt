cmake_minimum_required(VERSION 3.13)
project(cppSVF)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(DEBUG)

#include_directories(
#    /Library/Frameworks/Python.framework/Versions/3.7/include/python3.7m/
#)

link_directories(
    /Library/Frameworks/Python.framework/Versions/3.8/lib
)

add_executable(cppSVF main.cpp src/cpp/svf.h src/cpp/svf.cpp src/cpp/tests/test_svf.h src/cpp/tests/test_svf.cpp)

#target_include_directories(
#        cppSVF /Library/Frameworks/Python.framework/Versions/3.7/include/python3.7m/
#)

FIND_PACKAGE(PythonLibs 3.8 REQUIRED)
IF(PYTHONLIBS_FOUND)
    message(status " Python: ${PYTHON_INCLUDE_DIRS}")
    INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_DIRS}")
ELSE()
    MESSAGE(FATAL_ERROR "Unable to find Python libraries.")
ENDIF()
#message(status " Inc: ${INCLUDE_DIRECTORIES}")


target_link_libraries(cppSVF python3.8)
