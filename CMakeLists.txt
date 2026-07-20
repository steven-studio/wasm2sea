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
    src/wasm_reader.cpp
    src/wasm_dump.cpp
    src/wasm_lower.cpp
    src/value_ir_dump.cpp
    src/value_ir_eval.cpp
    src/ir_bridge.cpp
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

# ===== SIMD / 向量化支援開關 =====
# 平常關閉（OFF）：third_party/dstogov-ir 保持在 master 分支即可編譯。
# 開啟（ON）：需要先把 third_party/dstogov-ir 切到 simd 分支，
#   否則 dstogov/ir 端會缺少對應符號，連結失敗。
# 用法：cmake .. -DWASM2SEA_ENABLE_SIMD=ON
option(WASM2SEA_ENABLE_SIMD "Enable SIMD/vectorization support (requires dstogov/ir simd branch)" OFF)

if(WASM2SEA_ENABLE_SIMD)
    target_compile_definitions(wasm2sea PRIVATE WASM2SEA_ENABLE_SIMD)
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
