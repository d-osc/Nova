# Nova Benchmark Runner
# Compares Nova, Node.js, Bun, and Deno

$ErrorActionPreference = "SilentlyContinue"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "     NOVA COMPILER BENCHMARK SUITE" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Check available runtimes
$hasNode = Get-Command node -ErrorAction SilentlyContinue
$hasBun = Get-Command bun -ErrorAction SilentlyContinue
$hasDeno = Get-Command deno -ErrorAction SilentlyContinue
$novaPath = "..\build\Release\nova.exe"

Write-Host "Available Runtimes:" -ForegroundColor Yellow
if ($hasNode) { Write-Host "  [OK] Node.js: $(node --version)" -ForegroundColor Green }
if ($hasBun) { Write-Host "  [OK] Bun: $(bun --version)" -ForegroundColor Green }
if ($hasDeno) { Write-Host "  [OK] Deno: $(deno --version | Select-Object -First 1)" -ForegroundColor Green }
if (Test-Path $novaPath) { Write-Host "  [OK] Nova Compiler" -ForegroundColor Green }
Write-Host ""

# Results storage
$results = @{}

function Run-Benchmark {
    param($name, $runtime, $cmd, $file)

    Write-Host "  Running $name with $runtime..." -ForegroundColor Gray
    $startTime = Get-Date
    $memBefore = (Get-Process -Id $PID).WorkingSet64 / 1MB

    try {
        $output = & $cmd $file 2>&1
        $elapsed = ((Get-Date) - $startTime).TotalMilliseconds

        # Extract time from output if available
        $timeMatch = $output | Select-String -Pattern "Total: (\d+)ms"
        if ($timeMatch) {
            $elapsed = [int]$timeMatch.Matches[0].Groups[1].Value
        }

        return @{
            Time = [math]::Round($elapsed, 2)
            Output = $output -join "`n"
            Success = $true
        }
    } catch {
        return @{
            Time = 0
            Output = $_.Exception.Message
            Success = $false
        }
    }
}

# ============================================
# COMPUTATION BENCHMARK
# ============================================
Write-Host "1. COMPUTATION BENCHMARK (Fibonacci + Primes)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

if (Test-Path $novaPath) {
    $nova = Run-Benchmark "compute" "Nova" $novaPath "run benchmarks/bench_compute.ts"
    Write-Host "   Nova:    $($nova.Time)ms" -ForegroundColor $(if($nova.Success){"Green"}else{"Red"})
}
if ($hasNode) {
    $node = Run-Benchmark "compute" "Node" "node" "benchmarks/bench_compute.ts"
    Write-Host "   Node.js: $($node.Time)ms" -ForegroundColor Green
}
if ($hasBun) {
    $bun = Run-Benchmark "compute" "Bun" "bun" "benchmarks/bench_compute.ts"
    Write-Host "   Bun:     $($bun.Time)ms" -ForegroundColor Green
}
if ($hasDeno) {
    $deno = Run-Benchmark "compute" "Deno" "deno" "run benchmarks/bench_compute.ts"
    Write-Host "   Deno:    $($deno.Time)ms" -ForegroundColor Green
}
Write-Host ""

# ============================================
# JSON BENCHMARK
# ============================================
Write-Host "2. JSON BENCHMARK (Parse + Stringify)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

if (Test-Path $novaPath) {
    $nova = Run-Benchmark "json" "Nova" $novaPath "run benchmarks/bench_json.ts"
    Write-Host "   Nova:    $($nova.Time)ms" -ForegroundColor $(if($nova.Success){"Green"}else{"Red"})
}
if ($hasNode) {
    $node = Run-Benchmark "json" "Node" "node" "benchmarks/bench_json.ts"
    Write-Host "   Node.js: $($node.Time)ms" -ForegroundColor Green
}
if ($hasBun) {
    $bun = Run-Benchmark "json" "Bun" "bun" "benchmarks/bench_json.ts"
    Write-Host "   Bun:     $($bun.Time)ms" -ForegroundColor Green
}
if ($hasDeno) {
    $deno = Run-Benchmark "json" "Deno" "deno" "run benchmarks/bench_json.ts"
    Write-Host "   Deno:    $($deno.Time)ms" -ForegroundColor Green
}
Write-Host ""

# ============================================
# ARRAY BENCHMARK
# ============================================
Write-Host "3. ARRAY BENCHMARK (1M elements: map/filter/reduce/sort)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

if (Test-Path $novaPath) {
    $nova = Run-Benchmark "array" "Nova" $novaPath "run benchmarks/bench_array.ts"
    Write-Host "   Nova:    $($nova.Time)ms" -ForegroundColor $(if($nova.Success){"Green"}else{"Red"})
}
if ($hasNode) {
    $node = Run-Benchmark "array" "Node" "node" "benchmarks/bench_array.ts"
    Write-Host "   Node.js: $($node.Time)ms" -ForegroundColor Green
}
if ($hasBun) {
    $bun = Run-Benchmark "array" "Bun" "bun" "benchmarks/bench_array.ts"
    Write-Host "   Bun:     $($bun.Time)ms" -ForegroundColor Green
}
if ($hasDeno) {
    $deno = Run-Benchmark "array" "Deno" "deno" "run benchmarks/bench_array.ts"
    Write-Host "   Deno:    $($deno.Time)ms" -ForegroundColor Green
}
Write-Host ""

# ============================================
# STRING BENCHMARK
# ============================================
Write-Host "4. STRING BENCHMARK (concat/template/methods)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

if (Test-Path $novaPath) {
    $nova = Run-Benchmark "string" "Nova" $novaPath "run benchmarks/bench_string.ts"
    Write-Host "   Nova:    $($nova.Time)ms" -ForegroundColor $(if($nova.Success){"Green"}else{"Red"})
}
if ($hasNode) {
    $node = Run-Benchmark "string" "Node" "node" "benchmarks/bench_string.ts"
    Write-Host "   Node.js: $($node.Time)ms" -ForegroundColor Green
}
if ($hasBun) {
    $bun = Run-Benchmark "string" "Bun" "bun" "benchmarks/bench_string.ts"
    Write-Host "   Bun:     $($bun.Time)ms" -ForegroundColor Green
}
if ($hasDeno) {
    $deno = Run-Benchmark "string" "Deno" "deno" "run benchmarks/bench_string.ts"
    Write-Host "   Deno:    $($deno.Time)ms" -ForegroundColor Green
}
Write-Host ""

# ============================================
# STARTUP TIME
# ============================================
Write-Host "5. STARTUP TIME (Hello World)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

if (Test-Path $novaPath) {
    $start = Get-Date
    & $novaPath run benchmarks/bench_startup.ts 2>&1 | Out-Null
    $novaStartup = ((Get-Date) - $start).TotalMilliseconds
    Write-Host "   Nova:    $([math]::Round($novaStartup, 2))ms" -ForegroundColor Green
}
if ($hasNode) {
    $start = Get-Date
    node benchmarks/bench_startup.ts 2>&1 | Out-Null
    $nodeStartup = ((Get-Date) - $start).TotalMilliseconds
    Write-Host "   Node.js: $([math]::Round($nodeStartup, 2))ms" -ForegroundColor Green
}
if ($hasBun) {
    $start = Get-Date
    bun benchmarks/bench_startup.ts 2>&1 | Out-Null
    $bunStartup = ((Get-Date) - $start).TotalMilliseconds
    Write-Host "   Bun:     $([math]::Round($bunStartup, 2))ms" -ForegroundColor Green
}
if ($hasDeno) {
    $start = Get-Date
    deno run benchmarks/bench_startup.ts 2>&1 | Out-Null
    $denoStartup = ((Get-Date) - $start).TotalMilliseconds
    Write-Host "   Deno:    $([math]::Round($denoStartup, 2))ms" -ForegroundColor Green
}
Write-Host ""

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "          BENCHMARK COMPLETE" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
