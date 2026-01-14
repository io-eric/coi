#!/usr/bin/env python3
"""
DOM Benchmark Runner for Coi, React, and Vue.
Measures performance of common DOM operations using Playwright.
"""

import os
import json
import subprocess
import sys
import asyncio
import time
import statistics

# Check for playwright
try:
    from playwright.async_api import async_playwright
except ImportError:
    print("Playwright not found. Installing...")
    subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'playwright'])
    subprocess.check_call([sys.executable, '-m', 'playwright', 'install', 'chromium'])
    from playwright.async_api import async_playwright

FRAMEWORKS = ['coi', 'react', 'vue']
BENCHMARK_DIR = os.path.dirname(os.path.abspath(__file__))

# Benchmark operations to run
BENCHMARKS = [
    ('create1000', 'Create 1,000 rows'),
    ('clear', 'Clear rows'),
    ('create1000', 'Create 1,000 rows (2nd)'),
    ('update', 'Update 1,000 rows'),
    ('swap', 'Swap rows'),
    ('clear', 'Clear rows (2nd)'),
]

# Number of warmup runs and actual runs
WARMUP_RUNS = 2
BENCHMARK_RUNS = 5


def build_projects():
    """Build all framework projects."""
    print("\n=== Building Projects ===\n")
    
    # Build Coi
    print("Building Coi...")
    coi_dir = os.path.join(BENCHMARK_DIR, 'coi-rows')
    if os.path.exists(coi_dir):
        try:
            subprocess.check_call(['coi', 'build'], cwd=coi_dir)
            print("  ✓ Coi built successfully")
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            print(f"  ✗ Failed to build Coi: {e}")
    else:
        print(f"  ✗ Coi directory not found: {coi_dir}")
    
    # Build React and Vue
    for fw in ['react', 'vue']:
        print(f"Building {fw.capitalize()}...")
        fw_dir = os.path.join(BENCHMARK_DIR, f'{fw}-rows')
        if os.path.exists(fw_dir):
            try:
                subprocess.check_call(['npm', 'install'], cwd=fw_dir, 
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                subprocess.check_call(['npm', 'run', 'build'], cwd=fw_dir,
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                print(f"  ✓ {fw.capitalize()} built successfully")
            except subprocess.CalledProcessError as e:
                print(f"  ✗ Failed to build {fw}: {e}")
        else:
            print(f"  ✗ {fw.capitalize()} directory not found: {fw_dir}")


async def run_benchmark(page, button_id: str, timeout: int = 30000) -> float:
    """
    Click a benchmark button and measure time until DOM settles.
    Uses performance.now() around the click and waits for paint.
    Returns the measured duration in milliseconds.
    """
    # Measure from JS side - this works for all frameworks
    result = await page.evaluate('''(buttonId) => {
        return new Promise((resolve) => {
            const button = document.getElementById(buttonId);
            if (!button) {
                resolve(-1);
                return;
            }
            
            const start = performance.now();
            button.click();
            
            // Wait for next frame (React/Vue batch updates)
            // Then wait for paint to complete
            requestAnimationFrame(() => {
                requestAnimationFrame(() => {
                    const end = performance.now();
                    resolve(end - start);
                });
            });
        });
    }''', button_id)
    
    # Small delay to ensure stability
    await asyncio.sleep(0.05)
    
    return result


async def benchmark_framework(browser, framework: str, port: int) -> dict:
    """Run all benchmarks for a single framework."""
    results = {}
    
    page = await browser.new_page()
    await page.goto(f'http://localhost:{port}')
    
    # Wait for page to be ready
    await page.wait_for_selector('.controls')
    await asyncio.sleep(0.5)
    
    # Warmup runs
    print(f"    Warming up ({WARMUP_RUNS} runs)...")
    for _ in range(WARMUP_RUNS):
        await run_benchmark(page, 'create1000')
        await run_benchmark(page, 'clear')
    
    # Run each benchmark multiple times
    for button_id, label in BENCHMARKS:
        durations = []
        
        for run in range(BENCHMARK_RUNS):
            duration = await run_benchmark(page, button_id)
            if duration > 0:
                durations.append(duration)
        
        if durations:
            results[label] = {
                'mean': statistics.mean(durations),
                'median': statistics.median(durations),
                'min': min(durations),
                'max': max(durations),
                'stddev': statistics.stdev(durations) if len(durations) > 1 else 0,
                'runs': durations
            }
        else:
            results[label] = {'mean': -1, 'error': 'No valid results'}
    
    await page.close()
    return results


async def run_all_benchmarks():
    """Run benchmarks for all frameworks."""
    all_results = {}
    servers = {}
    
    # Start servers for each framework
    print("\n=== Starting Servers ===\n")
    
    base_port = 5173
    for i, fw in enumerate(FRAMEWORKS):
        port = base_port + i
        fw_dir = os.path.join(BENCHMARK_DIR, f'{fw}-rows', 'dist')
        
        if not os.path.exists(fw_dir):
            print(f"  ✗ {fw.capitalize()} dist not found, skipping")
            continue
        
        # Start a simple HTTP server
        process = subprocess.Popen(
            [sys.executable, '-m', 'http.server', str(port)],
            cwd=fw_dir,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
        servers[fw] = {'process': process, 'port': port}
        print(f"  ✓ {fw.capitalize()} server started on port {port}")
    
    # Wait for servers to start
    await asyncio.sleep(1)
    
    # Run benchmarks
    print("\n=== Running Benchmarks ===\n")
    
    async with async_playwright() as p:
        browser = await p.chromium.launch(headless=True)
        
        for fw in FRAMEWORKS:
            if fw not in servers:
                continue
                
            print(f"  {fw.capitalize()}:")
            try:
                results = await benchmark_framework(browser, fw, servers[fw]['port'])
                all_results[fw] = results
                
                # Print summary for this framework
                for label, data in results.items():
                    if 'mean' in data and data['mean'] > 0:
                        print(f"    {label}: {data['mean']:.2f}ms (±{data.get('stddev', 0):.2f})")
                    
            except Exception as e:
                print(f"    ✗ Error: {e}")
                all_results[fw] = {'error': str(e)}
        
        await browser.close()
    
    # Stop servers
    print("\n=== Stopping Servers ===\n")
    for fw, server in servers.items():
        server['process'].terminate()
        print(f"  ✓ {fw.capitalize()} server stopped")
    
    return all_results


def get_bundle_sizes() -> dict:
    """Get bundle sizes for all frameworks."""
    sizes = {}
    for fw in FRAMEWORKS:
        dist_path = os.path.join(BENCHMARK_DIR, f'{fw}-rows', 'dist')
        total_size = 0
        if os.path.exists(dist_path):
            for dirpath, dirnames, filenames in os.walk(dist_path):
                for f in filenames:
                    fp = os.path.join(dirpath, f)
                    if not os.path.islink(fp):
                        total_size += os.path.getsize(fp)
        sizes[fw] = total_size
    return sizes


def generate_svg_report(perf_results: dict, bundle_sizes: dict):
    """Generate an SVG visualization of the benchmark results."""
    width, height = 900, 700
    bg_color, text_main, text_sub = "#f8f9fa", "#212529", "#6c757d"
    colors = {"coi": "#9477ff", "react": "#00d8ff", "vue": "#42b883"}
    
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" style="font-family: -apple-system, BlinkMacSystemFont, \'Segoe UI\', Roboto, sans-serif; background: {bg_color};">']
    svg.append(f'<rect width="100%" height="100%" fill="{bg_color}"/>')
    
    # Header
    svg.append(f'<text x="{width/2}" y="40" text-anchor="middle" fill="{text_main}" font-size="24" font-weight="bold">DOM Performance Benchmark</text>')
    svg.append(f'<text x="{width/2}" y="65" text-anchor="middle" fill="{text_sub}" font-size="14">Lower is better (milliseconds)</text>')
    
    # Legend
    lx = width/2 - 150
    for fw in FRAMEWORKS:
        svg.append(f'<rect x="{lx}" y="85" width="12" height="12" fill="{colors[fw]}" rx="2"/>')
        svg.append(f'<text x="{lx + 18}" y="96" fill="{text_main}" font-size="13" font-weight="600">{fw.capitalize()}</text>')
        lx += 100
    
    # Chart area
    chart_x, chart_y = 200, 130
    bar_group_height = 50
    bar_height = 12
    max_width = 600
    
    # Get unique benchmark names
    benchmarks = ['Create 1,000 rows', 'Update 1,000 rows', 'Swap rows', 'Clear rows']
    
    # Find max value for scaling
    max_val = 1
    for fw in FRAMEWORKS:
        if fw in perf_results:
            for bench in benchmarks:
                if bench in perf_results[fw]:
                    val = perf_results[fw][bench].get('mean', 0)
                    if val > max_val:
                        max_val = val
    
    # Draw benchmarks
    y = chart_y
    for bench_name in benchmarks:
        # Label
        svg.append(f'<text x="{chart_x - 10}" y="{y + 22}" text-anchor="end" fill="{text_main}" font-size="12">{bench_name}</text>')
        
        # Bars for each framework
        bar_y = y
        for fw in FRAMEWORKS:
            if fw in perf_results and bench_name in perf_results[fw]:
                val = perf_results[fw][bench_name].get('mean', 0)
                if val > 0:
                    bar_w = (val / max_val) * max_width
                    svg.append(f'<rect x="{chart_x}" y="{bar_y}" width="{bar_w}" height="{bar_height}" fill="{colors[fw]}" rx="2"/>')
                    svg.append(f'<text x="{chart_x + bar_w + 5}" y="{bar_y + 10}" fill="{text_main}" font-size="10">{val:.1f}ms</text>')
            bar_y += bar_height + 2
        
        y += bar_group_height
    
    # Bundle sizes section
    y += 30
    svg.append(f'<text x="{width/2}" y="{y}" text-anchor="middle" fill="{text_main}" font-size="16" font-weight="bold">Bundle Size Comparison</text>')
    y += 10
    svg.append(f'<text x="{width/2}" y="{y}" text-anchor="middle" fill="{text_sub}" font-size="12">Smaller is better (KB)</text>')
    
    y += 30
    max_size = max(bundle_sizes.values()) if bundle_sizes.values() else 1
    
    for fw in FRAMEWORKS:
        size_kb = bundle_sizes.get(fw, 0) / 1024
        bar_w = (bundle_sizes.get(fw, 0) / max_size) * max_width
        
        svg.append(f'<text x="{chart_x - 10}" y="{y + 10}" text-anchor="end" fill="{text_main}" font-size="12" font-weight="bold">{fw.capitalize()}</text>')
        svg.append(f'<rect x="{chart_x}" y="{y}" width="{bar_w}" height="16" fill="{colors[fw]}" rx="2"/>')
        svg.append(f'<text x="{chart_x + bar_w + 5}" y="{y + 12}" fill="{text_main}" font-size="11">{size_kb:.1f} KB</text>')
        y += 26
    
    svg.append('</svg>')
    
    output_path = os.path.join(BENCHMARK_DIR, 'dom_benchmark_results.svg')
    with open(output_path, 'w') as f:
        f.write('\n'.join(svg))
    print(f"\n  ✓ SVG report saved to {output_path}")


def print_results_table(perf_results: dict, bundle_sizes: dict):
    """Print a formatted results table."""
    print("\n" + "=" * 70)
    print(" DOM BENCHMARK RESULTS")
    print("=" * 70)
    
    benchmarks = ['Create 1,000 rows', 'Update 1,000 rows', 'Swap rows', 'Clear rows']
    
    # Header
    header = f"{'Benchmark':<25}"
    for fw in FRAMEWORKS:
        header += f" | {fw.capitalize():>12}"
    print(header)
    print("-" * 70)
    
    # Results
    for bench in benchmarks:
        row = f"{bench:<25}"
        vals = []
        for fw in FRAMEWORKS:
            if fw in perf_results and bench in perf_results[fw]:
                val = perf_results[fw][bench].get('mean', -1)
                vals.append(val)
                row += f" | {val:>10.2f}ms" if val > 0 else " |        N/A"
            else:
                vals.append(float('inf'))
                row += " |        N/A"
        
        # Highlight winner
        if vals:
            min_val = min(v for v in vals if v > 0)
            # Add star to winner (reprint row with highlighting)
        
        print(row)
    
    print("-" * 70)
    
    # Bundle sizes
    print(f"\n{'Bundle Size':<25}", end="")
    for fw in FRAMEWORKS:
        size_kb = bundle_sizes.get(fw, 0) / 1024
        print(f" | {size_kb:>10.1f}KB", end="")
    print("\n" + "=" * 70)


def main():
    """Main entry point."""
    skip_build = "--no-build" in sys.argv
    
    if not skip_build:
        build_projects()
    
    # Run benchmarks
    perf_results = asyncio.run(run_all_benchmarks())
    
    # Get bundle sizes
    bundle_sizes = get_bundle_sizes()
    
    # Generate reports
    print_results_table(perf_results, bundle_sizes)
    generate_svg_report(perf_results, bundle_sizes)
    
    # Save JSON results
    output = {
        'performance': perf_results,
        'bundle_sizes': {fw: size for fw, size in bundle_sizes.items()}
    }
    
    json_path = os.path.join(BENCHMARK_DIR, 'dom_benchmark_results.json')
    with open(json_path, 'w') as f:
        json.dump(output, f, indent=2)
    print(f"  ✓ JSON results saved to {json_path}")


if __name__ == "__main__":
    main()
