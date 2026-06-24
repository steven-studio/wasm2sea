(module
  (memory (export "memory") 1)

  ;; A = [[1, 2], [3, 4]]
  (data (i32.const 0)  "\01\00\00\00\02\00\00\00\03\00\00\00\04\00\00\00")
  ;; B = [[5, 6], [7, 8]]
  (data (i32.const 16) "\05\00\00\00\06\00\00\00\07\00\00\00\08\00\00\00")

  ;; matmul: C = A * B, 2x2, returns checksum of C
  (func (export "matmul") (result i32)
    (local $sum i32)

    ;; C[0][0] = A[0][0]*B[0][0] + A[0][1]*B[1][0] = 1*5 + 2*7 = 19
    (i32.store (i32.const 32)
      (i32.add
        (i32.mul (i32.load (i32.const 0))  (i32.load (i32.const 16)))
        (i32.mul (i32.load (i32.const 4))  (i32.load (i32.const 24)))
      )
    )

    ;; C[0][1] = A[0][0]*B[0][1] + A[0][1]*B[1][1] = 1*6 + 2*8 = 22
    (i32.store (i32.const 36)
      (i32.add
        (i32.mul (i32.load (i32.const 0))  (i32.load (i32.const 20)))
        (i32.mul (i32.load (i32.const 4))  (i32.load (i32.const 28)))
      )
    )

    ;; C[1][0] = A[1][0]*B[0][0] + A[1][1]*B[1][0] = 3*5 + 4*7 = 43
    (i32.store (i32.const 40)
      (i32.add
        (i32.mul (i32.load (i32.const 8))  (i32.load (i32.const 16)))
        (i32.mul (i32.load (i32.const 12)) (i32.load (i32.const 24)))
      )
    )

    ;; C[1][1] = A[1][0]*B[0][1] + A[1][1]*B[1][1] = 3*6 + 4*8 = 50
    (i32.store (i32.const 44)
      (i32.add
        (i32.mul (i32.load (i32.const 8))  (i32.load (i32.const 20)))
        (i32.mul (i32.load (i32.const 12)) (i32.load (i32.const 28)))
      )
    )

    ;; checksum = 19 + 22 + 43 + 50 = 134
    (local.set $sum
      (i32.add
        (i32.add
          (i32.load (i32.const 32))
          (i32.load (i32.const 36))
        )
        (i32.add
          (i32.load (i32.const 40))
          (i32.load (i32.const 44))
        )
      )
    )

    (local.get $sum)
  )
)
