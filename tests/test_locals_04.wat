(module
  ;; Test 4: Modifying parameter (parameters are locals too)
  ;; Adds 100 to the input parameter
  (func (export "main") (param $x i32) (result i32)
    (local.set $x (i32.add (local.get $x) (i32.const 100)))
    (local.get $x)
  )
)