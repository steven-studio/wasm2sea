(module
  (func $test (export "test") (param i32) (result i32)
    (local $result i32)
    i32.const 0
    local.set $result
    local.get 0
    if
      i32.const 42
      local.set $result
    end
    local.get $result
  )
)
