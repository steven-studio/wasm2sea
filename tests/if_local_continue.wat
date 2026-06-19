(module
  (func $test (export "test") (param i32 i32) (result i32)
    (local i32)
    i32.const 10
    local.set 2
    local.get 0
    if
      i32.const 99
      local.set 2
    end
    local.get 2
    local.get 1
    i32.add
  )
)
