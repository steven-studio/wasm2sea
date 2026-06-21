(module
  (func $test (export "test") (param $n i32) (result i32)
    (local $sum i32)
    (local $i i32)
    i32.const 0
    local.set $sum
    i32.const 1
    local.set $i
    (loop $loop
      local.get $sum
      local.get $i
      i32.add
      local.set $sum
      local.get $i
      i32.const 1
      i32.add
      local.set $i
      local.get $i
      local.get $n
      i32.le_s
      br_if $loop
    )
    local.get $sum
  )
)
