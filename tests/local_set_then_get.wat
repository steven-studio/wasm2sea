(module
  (func $test (param i32) (result i32)
    (local i32)
    i32.const 99
    local.set 1
    local.get 1
  )
  (export "test" (func $test))
)
