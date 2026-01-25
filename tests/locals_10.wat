(module
  ;; Test 10: Complex - combination of all operations
  ;; Uses LocalSet, LocalTee, arithmetic, and parameter
  ;; a = 5, b = a + x, result = b
  (func (export "main") (param $x i32) (result i32)
    (local $a i32)
    (local $b i32)
    (local.set $a (i32.const 5))
    (local.tee $b 
      (i32.add (local.get $a) (local.get $x))
    )
  )
)
