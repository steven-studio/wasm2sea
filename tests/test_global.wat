(module
  (global $g (mut i32) (i32.const 0))
  
  (func (export "test") (param i32) (result i32)
    global.get $g
    local.get 0
    i32.add
    global.set $g
    global.get $g
  )
)
