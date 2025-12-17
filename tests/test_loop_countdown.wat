(module
  (func $test (export "test") (param i32) (result i32)
    (local $i i32)
    local.get 0
    local.set $i
    
    loop $repeat
      local.get $i
      i32.const 1
      i32.sub
      local.tee $i
      
      i32.const 0
      i32.gt_s
      br_if $repeat
    end
    
    local.get $i
  )
)
