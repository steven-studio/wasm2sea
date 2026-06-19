(module
  (func $test (export "test") (param i32) (result i32)
    local.get 0
    if
      i32.const 42
      return
    end
    i32.const 0
  )
)
