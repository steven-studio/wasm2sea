(module
  ;; Test 1: Basic LocalSet + LocalGet
  ;; Sets a local variable to 10 and returns it
  ;; Input parameter is ignored
  (func (export "main") (param $x i32) (result i32)
    (local $temp i32)
    (local.set $temp (i32.const 10))
    (local.get $temp)
  )
)