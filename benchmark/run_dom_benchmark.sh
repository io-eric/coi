#!/bin/bash
# Run DOM benchmark for Coi, React, and Vue

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "  DOM Benchmark: Coi vs React vs Vue"
echo "=========================================="

# Check dependencies
if ! command -v coi &> /dev/null; then
    echo "Error: 'coi' command not found. Build the project first."
    exit 1
fi

if ! command -v npm &> /dev/null; then
    echo "Error: 'npm' command not found. Install Node.js first."
    exit 1
fi

# Run the benchmark
python3 dom_benchmark.py "$@"
