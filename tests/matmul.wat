(module
  (memory 1)
  (func (export "test") (param i32) (result i32)
    (local i32 i32 i32 i32)
    ;; store A at addr 0
    i32.const 0  i32.const 1  i32.store
    i32.const 4  i32.const 2  i32.store
    i32.const 8  i32.const 3  i32.store
    i32.const 12 i32.const 4  i32.store
    i32.const 16 i32.const 5  i32.store
    i32.const 20 i32.const 6  i32.store
    i32.const 24 i32.const 7  i32.store
    i32.const 28 i32.const 8  i32.store
    i32.const 32 i32.const 9  i32.store
    ;; store B at addr 36
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
    i32.const 0 local.set 1
    block
      loop
        local.get 1 i32.const 3 i32.ge_s br_if 1
        ;; j = 0
        i32.const 0 local.set 2
        block
          loop
            local.get 2 i32.const 3 i32.ge_s br_if 1
            ;; k = 0
            i32.const 0 local.set 3
            block
              loop
                local.get 3 i32.const 3 i32.ge_s br_if 1
                ;; C[i][j] += A[i][k] * B[k][j]
                i32.const 72
                local.get 1 i32.const 3 i32.mul
                local.get 2 i32.add
                i32.const 4 i32.mul i32.add
                i32.const 72
                local.get 1 i32.const 3 i32.mul
                local.get 2 i32.add
                i32.const 4 i32.mul i32.add
                i32.load
                local.get 1 i32.const 3 i32.mul
                local.get 3 i32.add
                i32.const 4 i32.mul
                i32.load
                i32.const 36
                local.get 3 i32.const 3 i32.mul
                local.get 2 i32.add
                i32.const 4 i32.mul i32.add
                i32.load
                i32.mul i32.add
                i32.store
                local.get 3 i32.const 1 i32.add local.set 3
                br 0
              end
            end
            local.get 2 i32.const 1 i32.add local.set 2
            br 0
          end
        end
        local.get 1 i32.const 1 i32.add local.set 1
        br 0
      end
    end
    ;; return C[param/3][param%3] = mem[72 + param*4]
    i32.const 72
    local.get 0
    i32.const 4 i32.mul
    i32.add
    i32.load
  )
)