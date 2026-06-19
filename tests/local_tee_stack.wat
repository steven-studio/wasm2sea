(module
  (func $test (param i32) (result i32)
    (local i32)
    i32.const 55
    local.tee 1
  )
  (export "test" (func $test))
)
