#!/bin/bash

TESTS=(
  "local_get_param 42"
  "local_get_uninitialized 0"
  "local_set_then_get 0"
  "local_set_overwrite_param 99"
  "local_set_multiple 0"
  "local_tee_stack 0"
  "local_tee_then_get 0"
  "local_read_write_read 7"
  "local_tee_overwrite 0"
  "local_swap 3 5"
)

PASS=0
FAIL=0

for entry in "${TESTS[@]}"; do
  TEST=$(echo $entry | cut -d' ' -f1)
  ARGS=$(echo $entry | cut -d' ' -f2-)

  # wasmtime expected
  EXPECTED=$(wasmtime ~/wasm2sea/tests/${TEST}.wasm --invoke test $ARGS 2>/dev/null)

  # your pipeline actual
  bash ~/wasm2sea/tests/run_test.sh $TEST $ARGS > /tmp/actual_out.txt 2>&1
  ACTUAL=$(grep "Result:" /tmp/actual_out.txt | tail -1 | grep -o '[0-9-]*$')

  if [ "$EXPECTED" = "$ACTUAL" ]; then
    echo "PASS: $TEST ($ARGS) = $EXPECTED"
    PASS=$((PASS+1))
  else
    echo "FAIL: $TEST ($ARGS) expected=$EXPECTED actual=$ACTUAL"
    FAIL=$((FAIL+1))
  fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed"