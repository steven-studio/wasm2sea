(module
  (func $test (param i32) (result i32)
    (local i32)
    i32.const 77
    local.tee 1
    drop
    local.get 1
  )
  (export "test" (func $test))
)
