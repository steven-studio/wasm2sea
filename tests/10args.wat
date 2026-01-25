(module
  (func $test (export "test") 
    (param i32 i32 i32 i32 i32 i32 i32 i32 i32 i32) 
    (result i32)
    ;; 计算所有参数的和
    local.get 0
    local.get 1
    i32.add
    local.get 2
    i32.add
    local.get 3
    i32.add
    local.get 4
    i32.add
    local.get 5
    i32.add
    local.get 6
    i32.add
    local.get 7
    i32.add
    local.get 8
    i32.add
    local.get 9
    i32.add
  )
)