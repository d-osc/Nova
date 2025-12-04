# Quick Performance Benchmark
# Runs working benchmarks: Startup time, Compute, and JSON performance

param(
    [int]$Iterations = 10
)

$ErrorActionPreference = "Stop"

Write-Host "`n==================================================" -ForegroundColor Cyan
Write-Host "Nova Performance Quick Benchmark" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Iterations per test: $Iterations`n" -ForegroundColor Cyan

# Check if nova.exe exists
if (-not (Test-Path "./build/Release/nova.exe")) {
    Write-Host "Error: nova.exe not found at ./build/Release/nova.exe" -ForegroundColor Red
    Write-Host "Please build Nova first with: cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

# Check if benchmark files exist
$requiredFiles = @(
    "benchmarks/compute_bench.ts",
    "benchmarks/compute_bench.js",
    "benchmarks/json_perf_final.ts"
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path $file)) {
        Write-Host "Warning: $file not found" -ForegroundColor Yellow
    }
}

# ============================================================================
# 1. Startup Time Benchmark
# ============================================================================

Write-Host "`n[1/3] Startup Time Benchmark" -ForegroundColor Green
Write-Host "$('-' * 50)"

# Create a minimal test file
$testFile = "benchmarks/hello.ts"
"console.log(42);" | Out-File -FilePath $testFile -Encoding ASCII -NoNewline

# Nova startup
Write-Host "Measuring Nova startup time..." -ForegroundColor Cyan
$novaTimes = @()
for ($i = 1; $i -le $Iterations; $i++) {
    $time = Measure-Command { ./build/Release/nova.exe $testFile 2>&1 | Out-Null }
    $novaTimes += $time.TotalMilliseconds
    Write-Host "  Run ${i}: $($time.TotalMilliseconds.ToString('F2')) ms"
}

# Node startup
Write-Host "`nMeasuring Node startup time..." -ForegroundColor Cyan
$nodeTimes = @()
for ($i = 1; $i -le $Iterations; $i++) {
    $time = Measure-Command { node $testFile 2>&1 | Out-Null }
    $nodeTimes += $time.TotalMilliseconds
    Write-Host "  Run ${i}: $($time.TotalMilliseconds.ToString('F2')) ms"
}

# Bun startup (if available)
$bunAvailable = $false
try {
    $null = Get-Command bun -ErrorAction Stop
    $bunAvailable = $true
} catch { }

$bunTimes = @()
if ($bunAvailable) {
    Write-Host "`nMeasuring Bun startup time..." -ForegroundColor Cyan
    for ($i = 1; $i -le $Iterations; $i++) {
        $time = Measure-Command { bun $testFile 2>&1 | Out-Null }
        $bunTimes += $time.TotalMilliseconds
        Write-Host "  Run ${i}: $($time.TotalMilliseconds.ToString('F2')) ms"
    }
}

# Calculate averages
$novaAvg = ($novaTimes | Measure-Object -Average).Average
$nodeAvg = ($nodeTimes | Measure-Object -Average).Average

Write-Host "`n--- Startup Time Results ---" -ForegroundColor Yellow
Write-Host "Nova: $($novaAvg.ToString('F2')) ms (avg)"
Write-Host "Node: $($nodeAvg.ToString('F2')) ms (avg)"

if ($bunAvailable) {
    $bunAvg = ($bunTimes | Measure-Object -Average).Average
    Write-Host "Bun:  $($bunAvg.ToString('F2')) ms (avg)"

    $fastest = [Math]::Min([Math]::Min($novaAvg, $nodeAvg), $bunAvg)
    if ($fastest -eq $novaAvg) { Write-Host "Winner: Nova" -ForegroundColor Green }
    elseif ($fastest -eq $nodeAvg) { Write-Host "Winner: Node" -ForegroundColor Green }
    else { Write-Host "Winner: Bun" -ForegroundColor Green }
} else {
    $fastest = [Math]::Min($novaAvg, $nodeAvg)
    if ($fastest -eq $novaAvg) { Write-Host "Winner: Nova" -ForegroundColor Green }
    else { Write-Host "Winner: Node" -ForegroundColor Green }
}

# ============================================================================
# 2. Compute Performance Benchmark
# ============================================================================

Write-Host "`n`n[2/3] Compute Performance Benchmark" -ForegroundColor Green
Write-Host "$('-' * 50)"

if (Test-Path "benchmarks/compute_bench.ts") {
    # Nova compute
    Write-Host "Measuring Nova compute performance..." -ForegroundColor Cyan
    $novaComputeTimes = @()
    for ($i = 1; $i -le $Iterations; $i++) {
        $time = Measure-Command { ./build/Release/nova.exe benchmarks/compute_bench.ts 2>&1 | Out-Null }
        $novaComputeTimes += $time.TotalMilliseconds
        Write-Host "  Run ${i}: $($time.TotalMilliseconds.ToString('F2')) ms"
    }

    # Node compute
    Write-Host "`nMeasuring Node compute performance..." -ForegroundColor Cyan
    $nodeComputeTimes = @()
    for ($i = 1; $i -le $Iterations; $i++) {
        $time = Measure-Command { node benchmarks/compute_bench.js 2>&1 | Out-Null }
        $nodeComputeTimes += $time.TotalMilliseconds
        Write-Host "  Run ${i}: $($time.TotalMilliseconds.ToString('F2')) ms"
    }

    # Bun compute
    $bunComputeTimes = @()
    if ($bunAvailable) {
        Write-Host "`nMeasuring Bun compute performance..." -ForegroundColor Cyan
        for ($i = 1; $i -le $Iterations; $i++) {
            $time = Measure-Command { bun benchmarks/compute_bench.ts 2>&1 | Out-Null }
            $bunComputeTimes += $time.TotalMilliseconds
            Write-Host "  Run ${i}: $($time.TotalMilliseconds.ToString('F2')) ms"
        }
    }

    # Calculate averages
    $novaComputeAvg = ($novaComputeTimes | Measure-Object -Average).Average
    $nodeComputeAvg = ($nodeComputeTimes | Measure-Object -Average).Average

    Write-Host "`n--- Compute Performance Results ---" -ForegroundColor Yellow
    Write-Host "Nova: $($novaComputeAvg.ToString('F2')) ms (avg)"
    Write-Host "Node: $($nodeComputeAvg.ToString('F2')) ms (avg)"

    if ($bunAvailable) {
        $bunComputeAvg = ($bunComputeTimes | Measure-Object -Average).Average
        Write-Host "Bun:  $($bunComputeAvg.ToString('F2')) ms (avg)"

        $fastest = [Math]::Min([Math]::Min($novaComputeAvg, $nodeComputeAvg), $bunComputeAvg)
        if ($fastest -eq $novaComputeAvg) { Write-Host "Winner: Nova" -ForegroundColor Green }
        elseif ($fastest -eq $nodeComputeAvg) { Write-Host "Winner: Node" -ForegroundColor Green }
        else { Write-Host "Winner: Bun" -ForegroundColor Green }
    } else {
        $fastest = [Math]::Min($novaComputeAvg, $nodeComputeAvg)
        if ($fastest -eq $novaComputeAvg) { Write-Host "Winner: Nova" -ForegroundColor Green }
        else { Write-Host "Winner: Node" -ForegroundColor Green }
    }
} else {
    Write-Host "Skipping: compute_bench.ts not found" -ForegroundColor Yellow
}

# ============================================================================
# 3. JSON Performance Benchmark
# ============================================================================

Write-Host "`n`n[3/3] JSON Performance Benchmark" -ForegroundColor Green
Write-Host "$('-' * 50)"

if (Test-Path "benchmarks/json_perf_final.ts") {
    Write-Host "Running Nova JSON benchmark..." -ForegroundColor Cyan
    ./build/Release/nova.exe benchmarks/json_perf_final.ts

    Write-Host "`nRunning Node JSON benchmark..." -ForegroundColor Cyan
    node benchmarks/json_perf_final.ts

    if ($bunAvailable) {
        Write-Host "`nRunning Bun JSON benchmark..." -ForegroundColor Cyan
        bun benchmarks/json_perf_final.ts
    }
} else {
    Write-Host "Skipping: json_perf_final.ts not found" -ForegroundColor Yellow
}

# ============================================================================
# Summary
# ============================================================================

Write-Host "`n`n==================================================" -ForegroundColor Cyan
Write-Host "Benchmark Summary" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

Write-Host "`nStartup Time:"
Write-Host "  Nova: $($novaAvg.ToString('F2')) ms"
Write-Host "  Node: $($nodeAvg.ToString('F2')) ms"
if ($bunAvailable) {
    Write-Host "  Bun:  $($bunAvg.ToString('F2')) ms"
}

if (Test-Path "benchmarks/compute_bench.ts") {
    Write-Host "`nCompute Performance:"
    Write-Host "  Nova: $($novaComputeAvg.ToString('F2')) ms"
    Write-Host "  Node: $($nodeComputeAvg.ToString('F2')) ms"
    if ($bunAvailable) {
        Write-Host "  Bun:  $($bunComputeAvg.ToString('F2')) ms"
    }
}

Write-Host "`nFor detailed HTTP benchmarks (when available), run:"
Write-Host "  powershell -ExecutionPolicy Bypass -File benchmarks/bench_http_comprehensive.ps1" -ForegroundColor Yellow

Write-Host "`nBenchmark completed!`n" -ForegroundColor Green
