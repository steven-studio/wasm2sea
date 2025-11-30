cmake_minimum_required(VERSION 3.15)
project(wasm2sea LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(wasm2sea
    src/main.cpp
    src/ir_bridge.cpp
    src/wasm_instr.hpp
    src/value_ir.hpp
    src/wasm_lower.hpp
    src/wasm_lower.cpp
    src/value_ir_dump.hpp
    src/value_ir_dump.cpp
)
