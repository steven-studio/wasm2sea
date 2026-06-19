(module
  (func $test (param i32) (result i32)
    (local i32)
    i32.const 1
    local.set 1
    i32.const 2
    local.set 1
    i32.const 3
    local.set 1
    local.get 1
  )
  (export "test" (func $test))
)
