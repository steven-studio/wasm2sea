(module
  (func $test (export "test") (param i32 i32) (result i32)
    local.get 0    ;; 12 (二進制: 1100)
    local.get 1    ;; 10 (二進制: 1010)
    i32.and        ;; 結果: 8 (二進制: 1000)
  )
)