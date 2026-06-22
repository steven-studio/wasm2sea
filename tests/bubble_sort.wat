(module
  (memory 1)
  (func (export "test") (param i32) (result i32)
    (local i32 i32 i32 i32 i32)
    ;; local 1: i (outer loop)
    ;; local 2: j (inner loop)
    ;; local 3: temp
    ;; local 4: a[j]
    ;; local 5: a[j+1]

    ;; outer loop: i from 0 to n-1
    (local.set 1 (i32.const 0))
    (block $break_outer
      (loop $outer
        (br_if $break_outer (i32.ge_s (local.get 1) (local.get 0)))

        ;; inner loop: j from 0 to n-i-2
        (local.set 2 (i32.const 0))
        (block $break_inner
          (loop $inner
            (br_if $break_inner
              (i32.ge_s (local.get 2)
                (i32.sub (i32.sub (local.get 0) (local.get 1)) (i32.const 1))))

            ;; load a[j] and a[j+1]
            (local.set 4 (i32.load (i32.mul (local.get 2) (i32.const 4))))
            (local.set 5 (i32.load (i32.mul (i32.add (local.get 2) (i32.const 1)) (i32.const 4))))

            ;; if a[j] > a[j+1], swap
            (if (i32.gt_s (local.get 4) (local.get 5))
              (then
                (i32.store (i32.mul (local.get 2) (i32.const 4)) (local.get 5))
                (i32.store (i32.mul (i32.add (local.get 2) (i32.const 1)) (i32.const 4)) (local.get 4))
              )
            )

            (local.set 2 (i32.add (local.get 2) (i32.const 1)))
            (br $inner)
          )
        )

        (local.set 1 (i32.add (local.get 1) (i32.const 1)))
        (br $outer)
      )
    )

    ;; compute checksum: sum of sorted array
    (local.set 2 (i32.const 0))
    (local.set 3 (i32.const 0))
    (block $break_sum
      (loop $sum
        (br_if $break_sum (i32.ge_s (local.get 2) (local.get 0)))
        (local.set 3
          (i32.add (local.get 3)
            (i32.load (i32.mul (local.get 2) (i32.const 4)))))
        (local.set 2 (i32.add (local.get 2) (i32.const 1)))
        (br $sum)
      )
    )
    (local.get 3)
  )
)
