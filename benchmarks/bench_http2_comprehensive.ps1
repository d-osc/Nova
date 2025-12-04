# Comprehensive HTTP/2 Benchmark - Node.js
# This benchmark tests Node.js HTTP/2 performance with various request patterns

Write-Host "=== Comprehensive HTTP/2 Benchmark - Node.js ===" -ForegroundColor Cyan
Write-Host ""

$port = 3000
$results = @{}

function Measure-HTTPPerformance {
    param(
        [string]$testName,
        [int]$requestCount
    )

    Write-Host "Running: $testName..." -ForegroundColor Yellow

    $times = @()
    $failures = 0

    for ($i = 1; $i -le $requestCount; $i++) {
        try {
            $sw = [System.Diagnostics.Stopwatch]::StartNew()
            $response = curl.exe -s --http2 "http://127.0.0.1:$port/" 2>&1
            $sw.Stop()

            if ($response -match "Hello HTTP/2") {
                $times += $sw.Elapsed.TotalMilliseconds
            } else {
                $failures++
            }
        } catch {
            $failures++
        }

        if ($i % 50 -eq 0) {
            Write-Host "  Progress: $i/$requestCount" -ForegroundColor Gray
        }
    }

    if ($times.Count -gt 0) {
        $avg = ($times | Measure-Object -Average).Average
        $min = ($times | Measure-Object -Minimum).Minimum
        $max = ($times | Measure-Object -Maximum).Maximum
        $median = ($times | Sort-Object)[[math]::Floor($times.Count / 2)]
        $p95 = ($times | Sort-Object)[[math]::Floor($times.Count * 0.95)]

        Write-Host "  Results:" -ForegroundColor Green
        Write-Host "    Successful: $($times.Count)/$requestCount" -ForegroundColor White
        Write-Host "    Average: $([math]::Round($avg, 2))ms" -ForegroundColor White
        Write-Host "    Median:  $([math]::Round($median, 2))ms" -ForegroundColor White
        Write-Host "    Min:     $([math]::Round($min, 2))ms" -ForegroundColor White
        Write-Host "    Max:     $([math]::Round($max, 2))ms" -ForegroundColor White
        Write-Host "    P95:     $([math]::Round($p95, 2))ms" -ForegroundColor White
        Write-Host "    Req/sec: $([math]::Round(1000 / $avg, 2))" -ForegroundColor White

        return @{
            TestName = $testName
            TotalRequests = $requestCount
            Successful = $times.Count
            Failed = $failures
            AvgMs = [math]::Round($avg, 2)
            MedianMs = [math]::Round($median, 2)
            MinMs = [math]::Round($min, 2)
            MaxMs = [math]::Round($max, 2)
            P95Ms = [math]::Round($p95, 2)
            ReqPerSec = [math]::Round(1000 / $avg, 2)
        }
    } else {
        Write-Host "  ERROR: All requests failed" -ForegroundColor Red
        return $null
    }

    Write-Host ""
}

# Start Node.js HTTP/2 server
Write-Host "Starting Node.js HTTP/2 server..." -ForegroundColor Green
$server = Start-Process -FilePath "node" -ArgumentList "benchmarks\http2_server_node.js" -PassThru -WindowStyle Hidden -RedirectStandardOutput "http2_bench_out.txt" -RedirectStandardError "http2_bench_err.txt"

Start-Sleep -Seconds 3

# Verify server is running
$test = curl.exe -s --http2 "http://127.0.0.1:$port/" 2>&1
if ($test -notmatch "Hello HTTP/2") {
    Write-Host "ERROR: Server failed to start properly" -ForegroundColor Red
    Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
    exit 1
}

Write-Host "Server started successfully" -ForegroundColor Green
Write-Host ""

# Run benchmarks
Write-Host "=== Test Suite ===" -ForegroundColor Cyan
Write-Host ""

$results["Warmup"] = Measure-HTTPPerformance -testName "Warmup (10 requests)" -requestCount 10
Start-Sleep -Seconds 1

$results["Light"] = Measure-HTTPPerformance -testName "Light Load (100 requests)" -requestCount 100
Start-Sleep -Seconds 1

$results["Medium"] = Measure-HTTPPerformance -testName "Medium Load (500 requests)" -requestCount 500
Start-Sleep -Seconds 2

$results["Heavy"] = Measure-HTTPPerformance -testName "Heavy Load (1000 requests)" -requestCount 1000

# Stop server
Write-Host ""
Write-Host "Stopping server..." -ForegroundColor Gray
Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue

# Generate summary
Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host ""

foreach ($key in @("Light", "Medium", "Heavy")) {
    $result = $results[$key]
    if ($result) {
        Write-Host "$($result.TestName):" -ForegroundColor Yellow
        Write-Host "  Avg Latency: $($result.AvgMs)ms" -ForegroundColor White
        Write-Host "  Throughput:  $($result.ReqPerSec) req/s" -ForegroundColor White
        Write-Host "  P95:         $($result.P95Ms)ms" -ForegroundColor White
        Write-Host ""
    }
}

# Save results
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultsFile = "http2_bench_results_$timestamp.json"
$results | ConvertTo-Json -Depth 10 | Out-File $resultsFile
Write-Host "Results saved to: $resultsFile" -ForegroundColor Green

Write-Host ""
Write-Host "=== Benchmark Complete ===" -ForegroundColor Cyan
