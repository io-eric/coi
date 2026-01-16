# Coi vs React vs Vue Benchmark

This directory contains benchmarks comparing **Coi**, **React**, and **Vue**.

## What's Measured

| Benchmark | Apps Used | Measures |
|-----------|-----------|----------|
| **Bundle Size** | Counter apps | Total production build size (HTML + JS + CSS + WASM) |
| **DOM Performance** | Rows apps | Create, update, swap, and clear 1,000 rows |

## Prerequisites

- **Python 3**: For the runner script
- **Node.js & npm**: Required to build React and Vue
- **Coi**: The `coi` command must be in PATH
- **Playwright** (optional): For DOM benchmarks (`pip install playwright && playwright install chromium`)

## Running the Benchmark

```bash
./run.sh
```

### Options

```bash
./run.sh --no-build    # Skip building, use existing dist folders
./run.sh --size-only   # Only measure bundle sizes
./run.sh --dom-only    # Only run DOM performance benchmarks
```

## Results

The benchmark generates:
- `benchmark_results.json` — Raw data
- `benchmark_results.svg` — Visual comparison chart

![Benchmark Results](benchmark_results.svg)

## Project Structure

```
benchmark/
├── coi-counter/     # Coi counter app (bundle size)
├── react-counter/   # React counter app (bundle size)
├── vue-counter/     # Vue counter app (bundle size)
├── coi-rows/        # Coi rows app (DOM performance)
├── react-rows/      # React rows app (DOM performance)
├── vue-rows/        # Vue rows app (DOM performance)
├── benchmark.py     # Main benchmark runner
└── run.sh           # Entry point script
```

