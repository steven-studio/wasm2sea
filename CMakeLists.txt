cmake_minimum_required(VERSION 3.15)
project(wasm2sea CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(third_party/binaryen EXCLUDE_FROM_ALL)

# ===== dstogov/ir 路徑 =====
set(IR_DIR ${CMAKE_SOURCE_DIR}/third_party/dstogov-ir)
set(IR_LIB ${IR_DIR}/libir.a)  # 若實際產出路徑不同，再改這一行即可

# ===== Binaryen 路徑 =====
set(BINARYEN_DIR ${CMAKE_SOURCE_DIR}/third_party/binaryen)

# ===== 專案原始碼 =====
set(SOURCES
    src/main.cpp
    src/wasm_lower.cpp
    src/value_ir_dump.cpp
    src/value_ir_eval.cpp
    src/ir_bridge.cpp
    src/wasm_reader.cpp
)

add_executable(wasm2sea ${SOURCES})

# ===== Include 目錄（關鍵！）=====
target_include_directories(wasm2sea PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${IR_DIR}
    ${BINARYEN_DIR}/src
)

# ===== 連結 libir.a =====
target_link_libraries(wasm2sea PRIVATE 
    ${IR_DIR}/libir.a
    binaryen
    pthread
    m                        # 數學函式庫
    dl                       # 動態連結函式庫
)

# ===== 編譯選項 =====
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(wasm2sea PRIVATE -Wall -Wextra)
endif()

# ===== 自動編譯 ir（可選）=====
add_custom_target(build_ir
    COMMAND make
    WORKING_DIRECTORY ${IR_DIR}
    COMMENT "Building dstogov/ir library"
)
add_dependencies(wasm2sea build_ir)

# ===== 測試 =====
enable_testing()
add_test(NAME basic_add COMMAND wasm2sea)
