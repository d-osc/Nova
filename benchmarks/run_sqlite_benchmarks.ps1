# Run SQLite Benchmarks - Windows PowerShell Version
# Compare Nova Standard vs Ultra vs Node.js

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "SQLite Performance Comparison Benchmark" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if nova compiler exists
$novaPath = $null
if (Test-Path "..\build\Release\nova.exe") {
    $novaPath = "..\build\Release\nova.exe"
} elseif (Test-Path "..\build\windows-native\nova.exe") {
    $novaPath = "..\build\windows-native\nova.exe"
} elseif (Test-Path "..\dist\nova-windows-x64.exe") {
    $novaPath = "..\dist\nova-windows-x64.exe"
}

if (-not $novaPath) {
    Write-Host "Error: Nova compiler not found!" -ForegroundColor Red
    Write-Host "Please build Nova first with: cmake --build build --config Release"
    exit 1
}

Write-Host "Using Nova compiler: $novaPath" -ForegroundColor Green
Write-Host ""

# Run Node.js benchmark (if available)
if (Get-Command node -ErrorAction SilentlyContinue) {
    Write-Host "=========================================" -ForegroundColor Yellow
    Write-Host "Node.js SQLite Benchmark" -ForegroundColor Yellow
    Write-Host "=========================================" -ForegroundColor Yellow
    node sqlite_benchmark.ts
    Write-Host ""
} else {
    Write-Host "Node.js not found - skipping Node.js benchmark" -ForegroundColor Gray
    Write-Host ""
}

# Run Bun benchmark (if available)
if (Get-Command bun -ErrorAction SilentlyContinue) {
    Write-Host "=========================================" -ForegroundColor Yellow
    Write-Host "Bun SQLite Benchmark" -ForegroundColor Yellow
    Write-Host "=========================================" -ForegroundColor Yellow
    bun sqlite_benchmark.ts
    Write-Host ""
}

# Run Nova standard benchmark
Write-Host "=========================================" -ForegroundColor Magenta
Write-Host "Nova Standard SQLite Benchmark" -ForegroundColor Magenta
Write-Host "=========================================" -ForegroundColor Magenta
& $novaPath sqlite_benchmark.ts
Write-Host ""

# Run Nova ultra benchmark
Write-Host "=========================================" -ForegroundColor Green
Write-Host "Nova ULTRA SQLite Benchmark" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
& $novaPath sqlite_ultra_benchmark.ts
Write-Host ""

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Benchmark Complete!" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Expected Results:" -ForegroundColor White
Write-Host "  Nova Ultra vs Node.js:" -ForegroundColor Yellow
Write-Host "    • Batch Insert: 5-10x faster" -ForegroundColor Green
Write-Host "    • Repeated Queries: 5-7x faster" -ForegroundColor Green
Write-Host "    • Large Results: 4-5x faster" -ForegroundColor Green
Write-Host "    • Memory Usage: 50-70% reduction" -ForegroundColor Green
Write-Host ""
Write-Host "  Nova Ultra vs Nova Standard:" -ForegroundColor Yellow
Write-Host "    • Statement Caching: 3-5x faster" -ForegroundColor Green
Write-Host "    • Connection Pooling: 2-3x faster" -ForegroundColor Green
Write-Host "    • Zero-Copy Strings: 2-4x faster" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Cyan
