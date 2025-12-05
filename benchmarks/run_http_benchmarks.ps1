#################################################
# HTTP Performance Benchmark Suite (Windows)
# Compares Nova vs Node.js vs Bun vs Deno
#################################################

$DURATION = 30
$THREADS = 4
$CONNECTIONS = 100

Write-Host "============================================"
Write-Host "HTTP Performance Benchmark Suite"
Write-Host "============================================"
Write-Host ""
Write-Host "Configuration:"
Write-Host "  Duration: ${DURATION}s"
Write-Host "  Threads: ${THREADS}"
Write-Host "  Connections: ${CONNECTIONS}"
Write-Host ""

# Check if bombardier is installed (Windows alternative to wrk)
if (!(Get-Command bombardier -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: bombardier is not installed"
    Write-Host "Install: go install github.com/codesenberg/bombardier@latest"
    Write-Host "Or download from: https://github.com/codesenberg/bombardier/releases"
    exit 1
}

function Run-Benchmark {
    param(
        [string]$Runtime,
        [string]$Command,
        [int]$Port,
        [string]$TestName
    )

    Write-Host "================================================"
    Write-Host "Testing: $TestName - $Runtime"
    Write-Host "================================================"

    # Start server in background
    $process = Start-Process -FilePath $Command -ArgumentList "benchmarks\$TestName.ts" -PassThru -WindowStyle Hidden

    # Wait for server to start
    Start-Sleep -Seconds 2

    # Check if server started
    if ($process.HasExited) {
        Write-Host "ERROR: Failed to start $Runtime server"
        return
    }

    # Run benchmark
    Write-Host "Running bombardier benchmark..."
    $url = "http://127.0.0.1:$Port/"
    bombardier -c $CONNECTIONS -t ${DURATION}s -l $url 2>&1 | Tee-Object -FilePath "$env:TEMP\wrk_${Runtime}_${TestName}.txt"

    Write-Host ""

    # Stop server
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
}

# Test 1: Hello World
Write-Host ""
Write-Host "###############################################"
Write-Host "# Test 1: Hello World (Minimum Overhead)"
Write-Host "###############################################"
Write-Host ""

if (Get-Command ".\build\Release\nova.exe" -ErrorAction SilentlyContinue) {
    Run-Benchmark -Runtime "Nova" -Command ".\build\Release\nova.exe" -Port 3000 -TestName "http_hello_world"
}

if (Get-Command node -ErrorAction SilentlyContinue) {
    Run-Benchmark -Runtime "Node.js" -Command "node" -Port 3000 -TestName "http_hello_world"
}

if (Get-Command bun -ErrorAction SilentlyContinue) {
    Run-Benchmark -Runtime "Bun" -Command "bun" -Port 3000 -TestName "http_hello_world"
}

if (Get-Command deno -ErrorAction SilentlyContinue) {
    Run-Benchmark -Runtime "Deno" -Command "deno" -Port 3000 -TestName "http_hello_world"
}

# Test 2: JSON Response
Write-Host ""
Write-Host "###############################################"
Write-Host "# Test 2: JSON Response"
Write-Host "###############################################"
Write-Host ""

if (Get-Command ".\build\Release\nova.exe" -ErrorAction SilentlyContinue) {
    Run-Benchmark -Runtime "Nova" -Command ".\build\Release\nova.exe" -Port 3001 -TestName "http_json_response"
}

if (Get-Command node -ErrorAction SilentlyContinue) {
    Run-Benchmark -Runtime "Node.js" -Command "node" -Port 3001 -TestName "http_json_response"
}

Write-Host ""
Write-Host "============================================"
Write-Host "Benchmark Complete!"
Write-Host "============================================"
Write-Host ""
Write-Host "Results saved to $env:TEMP\wrk_*.txt"
Write-Host ""
