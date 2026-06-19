(module
  (func $test (param i32) (result i32)
    (local i32)
    i32.const 10
    local.tee 1
    drop
    i32.const 20
    local.set 1
    local.get 1
  )
  (export "test" (func $test))
)
