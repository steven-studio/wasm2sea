(module
  ;; Test 7: Local variable with arithmetic operations
  ;; result = (x * 2) + 5
  (func (export "main") (param $x i32) (result i32)
    (local $result i32)
    (local.set $result (i32.mul (local.get $x) (i32.const 2)))
    (local.set $result (i32.add (local.get $result) (i32.const 5)))
    (local.get $result)
  )
)
