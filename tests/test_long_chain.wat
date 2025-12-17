(module
  (func $test (export "test") (param i32 i32 i32 i32 i32 i32) (result i32)
    ;; 計算: ((((p0 + p1) * p2) - p3) + p4) * p5
    local.get 0    ;; p0
    local.get 1    ;; p1
    i32.add        ;; p0 + p1
    
    local.get 2    ;; p2
    i32.mul        ;; (p0 + p1) * p2
    
    local.get 3    ;; p3
    i32.sub        ;; ((p0 + p1) * p2) - p3
    
    local.get 4    ;; p4
    i32.add        ;; (((p0 + p1) * p2) - p3) + p4
    
    local.get 5    ;; p5
    i32.mul        ;; ((((p0 + p1) * p2) - p3) + p4) * p5
  )
)
