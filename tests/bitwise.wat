(module
  (func (export "test_and") (param $a i32) (param $b i32) (result i32)
    (i32.and (local.get $a) (local.get $b))
  )
  
  (func (export "test_or") (param $a i32) (param $b i32) (result i32)
    (i32.or (local.get $a) (local.get $b))
  )
  
  (func (export "test_xor") (param $a i32) (param $b i32) (result i32)
    (i32.xor (local.get $a) (local.get $b))
  )
  
  (func (export "test_shl") (param $a i32) (param $b i32) (result i32)
    (i32.shl (local.get $a) (local.get $b))
  )
  
  (func (export "test_shr") (param $a i32) (param $b i32) (result i32)
    (i32.shr_s (local.get $a) (local.get $b))
  )
  
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    ;; Test combination: (a & b) | (a ^ b)
    (i32.or
      (i32.and (local.get $a) (local.get $b))
      (i32.xor (local.get $a) (local.get $b))
    )
  )
)