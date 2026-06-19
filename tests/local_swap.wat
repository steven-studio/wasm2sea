(module
  (func $test (param i32 i32) (result i32)
    (local i32)
    local.get 0
    local.set 2
    local.get 1
    local.set 0
    local.get 2
    local.set 1
    local.get 0
  )
  (export "test" (func $test))
)
