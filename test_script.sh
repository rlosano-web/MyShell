#!/bin/bash

echo -e "\n=== Test 1: Basic Functionality ==="
echo "Running test commands..."
./bin/myshell < tests/test_input.txt

echo -e "\n=== Test 3: Log File ==="
if [ -f myshell.log ]; then
    echo "Log file contents (last 5 lines):"
    tail -5 myshell.log
else
    echo "Log file not found"
fi

echo -e "\n=== Test 4: Interactive Test ==="
echo "Type 'help' to see available commands, 'exit' to quit"
echo "Testing background jobs:"
echo "Starting sleep 5 in background..."
timeout 1 ./bin/myshell <<< "sleep 5 &"
echo "Background job started"

echo -e "\n=== Cleaning up ==="
rm -f test.txt
echo "Done!"