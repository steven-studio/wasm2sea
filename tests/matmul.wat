(module
  (memory 1)
  (func (export "matmul") (result i32)
    (local i32 i32 i32 i32 i32 i32)
    ;; A = [[1,2,3],[4,5,6],[7,8,9]] at addr 0
    ;; B = [[9,8,7],[6,5,4],[3,2,1]] at addr 36
    ;; C = result at addr 72
    ;; store A
    i32.const 0  i32.const 1  i32.store
    i32.const 4  i32.const 2  i32.store
    i32.const 8  i32.const 3  i32.store
    i32.const 12 i32.const 4  i32.store
    i32.const 16 i32.const 5  i32.store
    i32.const 20 i32.const 6  i32.store
    i32.const 24 i32.const 7  i32.store
    i32.const 28 i32.const 8  i32.store
    i32.const 32 i32.const 9  i32.store
    ;; store B
    i32.const 36 i32.const 9  i32.store
    i32.const 40 i32.const 8  i32.store
    i32.const 44 i32.const 7  i32.store
    i32.const 48 i32.const 6  i32.store
    i32.const 52 i32.const 5  i32.store
    i32.const 56 i32.const 4  i32.store
    i32.const 60 i32.const 3  i32.store
    i32.const 64 i32.const 2  i32.store
    i32.const 68 i32.const 1  i32.store
    ;; i = 0
    i32.const 0 local.set 0
    block
      loop
        local.get 0
        i32.const 3
        i32.ge_s
        br_if 1
        ;; j = 0
        i32.const 0 local.set 1
        block
          loop
            local.get 1
            i32.const 3
            i32.ge_s
            br_if 1
            ;; k = 0
            i32.const 0 local.set 2
            block
              loop
                local.get 2
                i32.const 3
                i32.ge_s
                br_if 1
                ;; C[i][j] += A[i][k] * B[k][j]
                ;; addr of C[i][j] = 72 + (i*3+j)*4
                i32.const 72
                local.get 0
                i32.const 3
                i32.mul
                local.get 1
                i32.add
                i32.const 4
                i32.mul
                i32.add
                ;; load C[i][j]
                i32.const 72
                local.get 0
                i32.const 3
                i32.mul
                local.get 1
                i32.add
                i32.const 4
                i32.mul
                i32.add
                i32.load
                ;; A[i][k] = mem[(i*3+k)*4]
                local.get 0
                i32.const 3
                i32.mul
                local.get 2
                i32.add
                i32.const 4
                i32.mul
                i32.load
                ;; B[k][j] = mem[36 + (k*3+j)*4]
                i32.const 36
                local.get 2
                i32.const 3
                i32.mul
                local.get 1
                i32.add
                i32.const 4
                i32.mul
                i32.add
                i32.load
                i32.mul
                i32.add
                i32.store
                ;; k++
                local.get 2
                i32.const 1
                i32.add
                local.set 2
                br 0
              end
            end
            ;; j++
            local.get 1
            i32.const 1
            i32.add
            local.set 1
            br 0
          end
        end
        ;; i++
        local.get 0
        i32.const 1
        i32.add
        local.set 0
        br 0
      end
    end
    ;; return C[0][0]
    i32.const 72
    i32.load
  )
)
