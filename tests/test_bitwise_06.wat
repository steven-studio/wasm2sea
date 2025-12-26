(module
  ;; Test 6: Logical shift right (zero-extending)
  ;; Returns: a >>> b (fills with zeros)
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.shr_u (local.get $a) (local.get $b))
  )
)
