(module
  (func $test (param i32 i32) (result i32)
    local.get 0  ;; 第一個參數
    local.get 1  ;; 第二個參數
    i32.sub      ;; 應該是 p0 - p1
  )
)