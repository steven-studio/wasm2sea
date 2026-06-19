(module
  ;; Test 2: Local variable reassignment
  ;; Sets local to 10, then adds input parameter to it
  (func (export "main") (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (i32.const 10))
    (local.set $temp (i32.add (local.get $temp) (local.get $x)))
    (local.get $temp)
  )
)