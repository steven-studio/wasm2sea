(module
  ;; Signed division (main test function)
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (i32.div_s (local.get $a) (local.get $b))
  )
  
  ;; Additional test: unsigned division
  (func (export "test_div_u") (param $a i32) (param $b i32) (result i32)
    (i32.div_u (local.get $a) (local.get $b))
  )
  
  ;; Additional test: signed division with constant
  (func (export "test_div_const") (param $a i32) (result i32)
    (i32.div_s (local.get $a) (i32.const 2))
  )
  
  ;; Additional test: division with zero check (safe division)
  (func (export "test_div_safe") (param $a i32) (param $b i32) (result i32)
    (if (result i32)
      (i32.eqz (local.get $b))
      (then (i32.const 0))  ;; Return 0 if divisor is zero
      (else (i32.div_s (local.get $a) (local.get $b)))
    )
  )
)