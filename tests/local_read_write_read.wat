(module
  (func $test (param i32) (result i32)
    (local i32)
    local.get 0
    local.set 1
    local.get 1
  )
  (export "test" (func $test))
)
