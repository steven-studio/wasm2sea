(module
  (func $test (export "test") (param i32 i32 i32) (result i32)
    ;; 实现: condition ? true_val : false_val
    local.get 2    ;; false_val
    local.get 1    ;; true_val
    local.get 0    ;; condition
    select
  )
)