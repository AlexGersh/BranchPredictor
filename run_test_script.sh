#!/bin/bash

# Colors for better readability
GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m" # No Color

# Loop through 1 to 99
for i in $(seq 1 99); do
    trace="./tests_itai_idan_CA/example${i}.trc"
    expected="./tests_itai_idan_CA/example${i}.out"
    output="log${i}.txt"

    echo -n "Running test ${i}... "

    # Run the simulator and capture the output
    ./bp_main "$trace" > "$output"

    # Compare output with expected
    if diff "$output" "$expected" > /dev/null; then
        echo -e "${GREEN}PASSED${NC}"
    else
        echo -e "${RED}FAILED${NC}"
    fi
done

