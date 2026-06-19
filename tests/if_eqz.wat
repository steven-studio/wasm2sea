(module
  (func $test (export "test") (param i32) (result i32)
    local.get 0
    i32.eqz
    if (result i32)
      i32.const 1
    else
      i32.const 0
    end
  )
)
