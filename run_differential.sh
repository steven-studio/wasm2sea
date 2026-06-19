#!/bin/bash

TESTS=(
  "local_get_param 42"
  "local_get_uninitialized 0"
  "local_set_then_get 2485"
  "local_set_overwrite_param 99"
  "local_set_multiple 0"
  "local_tee_stack 0"
  "local_tee_then_get 0"
  "local_read_write_read 7"
  "local_tee_overwrite 0"
  "local_swap 3 5"
  "if_else 5 3"
  "if_else 3 5"
  "if_no_else 1"
  "if_no_else 0"
  "if_complex 7 3"
  "if_complex 3 7"
  "if_nested 3 4"
  "if_nested 3 -1"
  "if_nested -1 4"
)

PASS=0
FAIL=0

for entry in "${TESTS[@]}"; do
  TEST=$(echo $entry | cut -d' ' -f1)
  ARGS=$(echo $entry | cut -d' ' -f2-)

  # wasmtime expected
  EXPECTED=$(wasmtime run --invoke test ~/wasm2sea/tests/${TEST}.wasm $ARGS 2>/dev/null)

  # your pipeline actual
  bash ~/wasm2sea/tests/run_test.sh $TEST $ARGS > /tmp/actual_out.txt 2>&1
  ACTUAL=$(bash ~/wasm2sea/tests/run_test.sh $TEST $ARGS 2>/dev/null | tail -1)

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