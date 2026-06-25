(module
  (func (export "test") (param i32) (result i32)
    local.get 0
    i32.const 1
    i32.le_s
    if (result i32)
      i32.const 1
    else
      local.get 0
      local.get 0
      i32.const 1
      i32.sub
      call 0
      i32.mul
    end
  )
)
