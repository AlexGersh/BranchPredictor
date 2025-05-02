#!/bin/bash

# Colors for better readability
GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m" # No Color

# Loop through 1 to 99
for i in $(seq 1 99); do
    trace="./tests/example${i}.trc"
    expected="./tests/example${i}.out"
    output="./outlogs/log${i}.txt"

    echo -n "Running test ${i}... "

    # Run the simulator and capture the output
    ./bp_main "$trace" > "$output"

    # Compare output with expected

    if diff "$output" "$expected" > /dev/null; then
        echo -e "${GREEN}PASSED${NC}"
        rm -f ./difflogs/difflog${i}.txt
    else
        echo -e "${RED}FAILED${NC}"
        diff "$output" "$expected" > ./difflogs/difflog${i}.txt
    fi
done

