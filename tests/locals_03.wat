(module
  ;; Test 3: LocalTee instruction
  ;; LocalTee sets a local and leaves value on stack
  ;; Returns: 5 + 5 = 10
  (func (export "main") (param $x i32) (result i32)
    (local $temp i32)
    (i32.add 
      (local.tee $temp (i32.const 5))
      (local.get $temp)
    )
  )
)
