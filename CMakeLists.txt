cmake_minimum_required(VERSION 4.3)
project(MyVM)

set(CMAKE_CXX_STANDARD 20)

add_executable(MyVM main.cpp
        share/utility/base_types.h
        share/utility/value.h
        share/utility/obj.h
        share/utility/hash_util.h
        share/utility/chunk.h
        share/gc/GC.h
        share/vm/VM.h
        share/vm/call_frame.h
        share/vm/op_code.h
        share/vm/vm.cpp
        share/interpreter/interpreter.h
        share/interpreter/interpreter.cpp)
include_directories(${CMAKE_SOURCE_DIR})