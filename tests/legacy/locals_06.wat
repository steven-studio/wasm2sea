(module
  ;; Test 6: Sparse locals (declare many, use few)
  ;; Declares 10 locals but only uses index 5
  ;; Tests lazy initialization
  (func (export "main") (param $x i32) (result i32)
    (local i32) (local i32) (local i32) (local i32) (local i32)
    (local i32) (local i32) (local i32) (local i32) (local i32)
    ;; Only set and get local at index 5 (6th local, after parameter)
    (local.set 6 (i32.const 99))
    (local.get 6)
  )
)
