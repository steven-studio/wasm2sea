(module
  (func $test (export "test") (param i32 i32) (result i32)
    ;; 只测试 i32.gt_s，返回 1 或 0
    local.get 0
    local.get 1
    i32.gt_s
  )
)