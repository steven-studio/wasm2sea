(module
  ;; Test 2: Bitwise OR operation
  ;; Returns: a | b
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.or (local.get $a) (local.get $b))
  )
)
