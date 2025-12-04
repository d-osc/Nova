# HTTP/2 Benchmark Script for Windows
# Compares Nova, Node.js, and Bun HTTP/2 servers

Write-Host "=== HTTP/2 Performance Benchmark ===" -ForegroundColor Cyan
Write-Host ""

# Configuration
$port = 3000
$requests = 10000
$concurrency = 100

# Check if h2load is available
$h2loadAvailable = Get-Command h2load -ErrorAction SilentlyContinue
if (-not $h2loadAvailable) {
    Write-Host "WARNING: h2load not found. Using curl for basic testing." -ForegroundColor Yellow
    Write-Host "For proper HTTP/2 benchmarking, install nghttp2-tools" -ForegroundColor Yellow
    Write-Host ""
}

function Test-ServerResponse {
    param($runtime, $url)

    try {
        $response = curl.exe -s --http2 "$url" 2>&1
        if ($response -match "Hello HTTP/2") {
            return $true
        }
    } catch {}
    return $false
}

function Benchmark-Runtime {
    param($name, $command, $color)

    Write-Host "Testing $name..." -ForegroundColor $color

    # Start server
    $process = Start-Process -FilePath "cmd.exe" -ArgumentList "/c $command" -PassThru -NoNewWindow -RedirectStandardOutput "http2_${name}_out.txt" -RedirectStandardError "http2_${name}_err.txt"

    # Wait for server to start
    Start-Sleep -Seconds 2

    # Verify server is running
    $serverOk = Test-ServerResponse $name "http://127.0.0.1:$port"

    if (-not $serverOk) {
        Write-Host "  Failed to start $name server" -ForegroundColor Red
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        return
    }

    Write-Host "  Server started successfully" -ForegroundColor Green

    # Run benchmark
    if ($h2loadAvailable) {
        Write-Host "  Running h2load benchmark..." -ForegroundColor Gray
        $result = h2load -n $requests -c $concurrency "http://127.0.0.1:$port/" 2>&1 | Out-String

        # Parse results
        if ($result -match "requests: (\d+) total") {
            $totalReqs = $matches[1]
            Write-Host "    Total requests: $totalReqs" -ForegroundColor White
        }

        if ($result -match "finished in ([\d.]+)s") {
            $time = $matches[1]
            Write-Host "    Time: ${time}s" -ForegroundColor White
        }

        if ($result -match "(\d+\.\d+) req/s") {
            $rps = $matches[1]
            Write-Host "    Requests/sec: $rps" -ForegroundColor White
        }

        if ($result -match "time for request: +([\d.]+)ms") {
            $latency = $matches[1]
            Write-Host "    Avg latency: ${latency}ms" -ForegroundColor White
        }
    } else {
        # Simple test with curl
        Write-Host "  Running simple curl test (10 requests)..." -ForegroundColor Gray
        $times = @()
        for ($i = 1; $i -le 10; $i++) {
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            $null = curl.exe -s --http2 "http://127.0.0.1:$port/" 2>&1
            $sw.Stop()
            $times += $sw.Elapsed.TotalMilliseconds
        }

        $avg = ($times | Measure-Object -Average).Average
        $min = ($times | Measure-Object -Minimum).Minimum
        $max = ($times | Measure-Object -Maximum).Maximum

        Write-Host "    Avg latency: $([math]::Round($avg, 2))ms" -ForegroundColor White
        Write-Host "    Min latency: $([math]::Round($min, 2))ms" -ForegroundColor White
        Write-Host "    Max latency: $([math]::Round($max, 2))ms" -ForegroundColor White
    }

    # Stop server
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    Write-Host ""
}

# Run benchmarks
Write-Host "Benchmarking HTTP/2 servers with $requests requests at concurrency $concurrency" -ForegroundColor Cyan
Write-Host ""

# Node.js
if (Get-Command node -ErrorAction SilentlyContinue) {
    Benchmark-Runtime "Node" "node benchmarks\http2_server_node.js" "Green"
} else {
    Write-Host "Node.js not found, skipping" -ForegroundColor Yellow
}

# Bun (Note: Bun doesn't have native HTTP/2 yet)
if (Get-Command bun -ErrorAction SilentlyContinue) {
    Write-Host "Bun..." -ForegroundColor Magenta
    Write-Host "  Bun does not have native HTTP/2 support yet" -ForegroundColor Yellow
    Write-Host ""
}

# Nova
if (Test-Path "build\Release\nova.exe") {
    Benchmark-Runtime "Nova" "build\Release\nova.exe benchmarks\http2_server_nova.ts" "Cyan"
} else {
    Write-Host "Nova not found at build\Release\nova.exe, skipping" -ForegroundColor Yellow
}

Write-Host "=== Benchmark Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: For accurate HTTP/2 benchmarking, install h2load:" -ForegroundColor Yellow
Write-Host "  - Windows: Download nghttp2 from https://github.com/nghttp2/nghttp2/releases" -ForegroundColor Gray
Write-Host "  - Or use WSL: sudo apt-get install nghttp2-client" -ForegroundColor Gray
