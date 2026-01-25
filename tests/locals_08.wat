(module
  ;; Test 8: Local in nested operations
  ;; temp = 3, result = (temp * x) + temp
  (func (export "main") (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (i32.const 3))
    (i32.add 
      (i32.mul (local.get $temp) (local.get $x))
      (local.get $temp)
    )
  )
)
