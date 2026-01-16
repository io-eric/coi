#!/bin/bash
# Unified Benchmark Runner for Coi, React, and Vue
# Measures bundle size (counter apps) and DOM performance (rows apps)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "========================================"
echo "     Coi vs React vs Vue Benchmark"
echo "========================================"

# Check dependencies
if ! command -v coi &> /dev/null; then
    echo "Error: 'coi' command not found. Build the project first."
    exit 1
fi

if ! command -v python3 &> /dev/null; then
    echo "Error: 'python3' not found."
    exit 1
fi

# Run the unified benchmark
python3 benchmark.py "$@"

echo ""
echo "Results saved to:"
echo "  - benchmark_results.svg"
echo "  - benchmark_results.json"
