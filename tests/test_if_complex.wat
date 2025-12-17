(module
  (func $test (export "test") (param i32 i32) (result i32)
    ;; if (p0 > 5) return p0 + p1; else return p0 * p1;
    local.get 0
    i32.const 5
    i32.gt_s
    if (result i32)
      local.get 0
      local.get 1
      i32.add
    else
      local.get 0
      local.get 1
      i32.mul
    end
  )
)