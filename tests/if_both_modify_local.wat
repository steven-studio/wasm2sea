(module
  (func $test (export "test") (param i32) (result i32)
    (local i32)
    local.get 0
    if
      i32.const 10
      local.set 1
    else
      i32.const 20
      local.set 1
    end
    local.get 1
  )
)
