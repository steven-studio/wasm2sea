(module
  (func $double (param i32) (result i32)
    local.get 0
    i32.const 2
    i32.mul
  )
  (func $add_one (param i32) (result i32)
    local.get 0
    call $double
    i32.const 1
    i32.add
  )
  (func (export "test") (param i32) (result i32)
    local.get 0
    call $add_one
  )
)
