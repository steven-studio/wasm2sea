(module
  (func $test (param i32) (result i32)
    i32.const 42
    local.set 0
    local.get 0
  )
  (export "test" (func $test))
)
