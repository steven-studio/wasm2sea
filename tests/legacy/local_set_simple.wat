(module
  (func $test (export "test") (param i32) (result i32)
    (local $temp i32)
    
    local.get 0
    i32.const 2
    i32.mul
    local.set $temp    ;; 只测试 set，不用 tee
    
    local.get $temp
    i32.const 1
    i32.add
  )
)
