(module
  (func (export "main") (param $a i32) (param $b i32) (result i32)
    (local $temp1 i32)
    (local $temp2 i32)
    
    ;; temp1 = a + b
    (local.set $temp1 (i32.add (local.get $a) (local.get $b)))
    
    ;; temp2 = a * b
    (local.set $temp2 (i32.mul (local.get $a) (local.get $b)))
    
    ;; return temp1 + temp2
    (i32.add (local.get $temp1) (local.get $temp2))
  )
)