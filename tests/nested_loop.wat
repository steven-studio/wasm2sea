(module
  (func (export "nested_loop") (result i32)
    (local $i i32)
    (local $j i32)
    (local $sum i32)

    ;; i = 0
    (local.set $i (i32.const 0))

    (block $outer_break
      (loop $outer
        ;; if i >= 3 break
        (br_if $outer_break
          (i32.ge_s (local.get $i) (i32.const 3)))

        ;; j = 0
        (local.set $j (i32.const 0))

        (block $inner_break
          (loop $inner
            ;; if j >= 3 break
            (br_if $inner_break
              (i32.ge_s (local.get $j) (i32.const 3)))

            ;; sum += i * 3 + j
            (local.set $sum
              (i32.add (local.get $sum)
                (i32.add
                  (i32.mul (local.get $i) (i32.const 3))
                  (local.get $j))))

            ;; j++
            (local.set $j (i32.add (local.get $j) (i32.const 1)))
            (br $inner)
          )
        )

        ;; i++
        (local.set $i (i32.add (local.get $i) (i32.const 1)))
        (br $outer)
      )
    )
    (local.get $sum)
  )
)
