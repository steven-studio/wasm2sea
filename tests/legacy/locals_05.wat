(module
  ;; Test 5: Multiple local variables
  ;; Uses two locals, sets them to 10 and 20, returns sum
  (func (export "main") (param $x i32) (result i32)
    (local $a i32)
    (local $b i32)
    (local.set $a (i32.const 10))
    (local.set $b (i32.const 20))
    (i32.add (local.get $a) (local.get $b))
  )
)