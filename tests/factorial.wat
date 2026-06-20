(module
  (func $test (export "test") (param $n i32) (result i32)
    (local $result i32)
    i32.const 1
    local.set $result
    (loop $loop
      local.get $result
      local.get $n
      i32.mul
      local.set $result
      local.get $n
      i32.const 1
      i32.sub
      local.set $n
      local.get $n
      i32.const 1
      i32.gt_s
      br_if $loop
    )
    local.get $result
  )
)
