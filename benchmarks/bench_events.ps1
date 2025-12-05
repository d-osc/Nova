# EventEmitter Benchmark Script
# Compares Nova, Node.js, and Bun EventEmitter performance

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  EventEmitter Benchmark" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultFile = "events_bench_results_$timestamp.txt"

# Test Node.js
Write-Host "[1/3] Testing Node.js EventEmitter..." -ForegroundColor Yellow
Write-Host ""
$nodeOutput = node benchmarks/events_bench_node.js 2>&1
$nodeOutput | Write-Host
Write-Host ""

# Test Bun
Write-Host "[2/3] Testing Bun EventEmitter..." -ForegroundColor Yellow
Write-Host ""
$bunOutput = bun benchmarks/events_bench_bun.ts 2>&1
$bunOutput | Write-Host
Write-Host ""

# Test Nova
Write-Host "[3/3] Testing Nova EventEmitter..." -ForegroundColor Yellow
Write-Host ""
$novaOutput = build/Release/nova.exe benchmarks/events_bench_nova.ts 2>&1
$novaOutput | Write-Host
Write-Host ""

# Save results
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Results Summary" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

@"
EventEmitter Benchmark Results
Generated: $(Get-Date)
=====================================

=== Node.js ===
$nodeOutput

=== Bun ===
$bunOutput

=== Nova ===
$novaOutput
"@ | Out-File $resultFile

Write-Host "Full results saved to: $resultFile" -ForegroundColor Green
Write-Host ""
