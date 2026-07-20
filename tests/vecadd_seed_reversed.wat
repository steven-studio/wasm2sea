(module
  (memory 1)
  (func (export "test") (param $a i32) (param $b i32) (result i32)
    (local $sum0 i32) (local $sum1 i32)

    ;; 故意讓 offset=4 的 Load 先出現，offset=0 的後出現
    ;; 這樣 j（第二個 consumer）對到的 lhs/rhs Load
    ;; offset 會比 i（第一個 consumer）小，觸發 diff == -elemSize 分支
    (local.set $sum1
      (i32.add
        (i32.load offset=4 (local.get $a))
        (i32.load offset=4 (local.get $b))))

    (local.set $sum0
      (i32.add
        (i32.load offset=0 (local.get $a))
        (i32.load offset=0 (local.get $b))))

    (i32.add (local.get $sum1) (local.get $sum0))
  )
)
