(module
  (func $test (export "test") (param i32 i32) (result i32)
    local.get 0    ;; 20
    local.get 1    ;; 4
    i32.div_s      ;; 20 / 4 = 5
  )
)