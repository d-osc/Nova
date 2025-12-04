# Network Benchmark Script
# Comprehensive network performance testing for Nova vs Node.js vs Bun

param(
    [int]$Duration = 10,
    [int]$Connections = 100,
    [int]$Requests = 10000
)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "   Nova Network Benchmark Suite" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Configuration:" -ForegroundColor Yellow
Write-Host "  Duration: $Duration seconds"
Write-Host "  Connections: $Connections concurrent"
Write-Host "  Requests: $Requests total"
Write-Host ""

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultFile = "net_bench_results_$timestamp.txt"

# Function to run benchmark
function Run-Benchmark {
    param(
        [string]$Name,
        [string]$Command,
        [string]$Args,
        [int]$Port,
        [string]$Endpoint = "/"
    )

    Write-Host "==================================================" -ForegroundColor Green
    Write-Host "  Testing: $Name" -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green

    # Start server
    Write-Host "Starting $Name server..." -ForegroundColor Yellow
    $server = Start-Process -FilePath $Command -ArgumentList $Args -PassThru -NoNewWindow -RedirectStandardOutput "bench_${Name}_out.txt" -RedirectStandardError "bench_${Name}_err.txt"

    # Wait for server to start
    Start-Sleep -Seconds 2

    # Check if server is running
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:$Port$Endpoint" -UseBasicParsing -TimeoutSec 5 -ErrorAction Stop
        Write-Host "Server is responding!" -ForegroundColor Green
    } catch {
        Write-Host "ERROR: Server failed to start!" -ForegroundColor Red
        Write-Host $_.Exception.Message -ForegroundColor Red
        Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
        return
    }

    # Run Apache Bench (if available) or use PowerShell
    Write-Host "Running benchmark..." -ForegroundColor Yellow

    # Try using curl for benchmarking (measure requests per second)
    $results = @{
        TotalRequests = 0
        SuccessfulRequests = 0
        FailedRequests = 0
        TotalTime = 0
        RequestsPerSecond = 0
        AverageLatency = 0
        MinLatency = [double]::MaxValue
        MaxLatency = 0
    }

    $startTime = Get-Date
    $endTime = $startTime.AddSeconds($Duration)
    $latencies = @()

    Write-Host "Sending requests for $Duration seconds..." -ForegroundColor Cyan

    while ((Get-Date) -lt $endTime) {
        $reqStart = Get-Date
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:$Port$Endpoint" -UseBasicParsing -TimeoutSec 1 -ErrorAction Stop
            $reqEnd = Get-Date
            $latency = ($reqEnd - $reqStart).TotalMilliseconds

            $results.TotalRequests++
            $results.SuccessfulRequests++
            $latencies += $latency

            if ($latency -lt $results.MinLatency) { $results.MinLatency = $latency }
            if ($latency -gt $results.MaxLatency) { $results.MaxLatency = $latency }
        } catch {
            $results.TotalRequests++
            $results.FailedRequests++
        }
    }

    $actualTime = ((Get-Date) - $startTime).TotalSeconds
    $results.TotalTime = $actualTime
    $results.RequestsPerSecond = [math]::Round($results.SuccessfulRequests / $actualTime, 2)

    if ($latencies.Count -gt 0) {
        $results.AverageLatency = [math]::Round(($latencies | Measure-Object -Average).Average, 2)
    }

    # Stop server
    Write-Host "Stopping server..." -ForegroundColor Yellow
    Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    # Display results
    Write-Host ""
    Write-Host "Results for $Name" -ForegroundColor Cyan
    Write-Host "-----------------------------------" -ForegroundColor Gray
    Write-Host "Total Requests:       $($results.TotalRequests)"
    Write-Host "Successful:           $($results.SuccessfulRequests)" -ForegroundColor Green
    Write-Host "Failed:               $($results.FailedRequests)" -ForegroundColor $(if ($results.FailedRequests -gt 0) { "Red" } else { "Green" })
    Write-Host "Total Time:           $([math]::Round($results.TotalTime, 2)) seconds"
    Write-Host "Requests/sec:         $($results.RequestsPerSecond)" -ForegroundColor Yellow
    Write-Host "Average Latency:      $($results.AverageLatency) ms"
    Write-Host "Min Latency:          $([math]::Round($results.MinLatency, 2)) ms"
    Write-Host "Max Latency:          $([math]::Round($results.MaxLatency, 2)) ms"
    Write-Host ""

    # Save to file
    Add-Content -Path $resultFile -Value "==================================================="
    Add-Content -Path $resultFile -Value "$Name Results"
    Add-Content -Path $resultFile -Value "==================================================="
    Add-Content -Path $resultFile -Value "Total Requests:       $($results.TotalRequests)"
    Add-Content -Path $resultFile -Value "Successful:           $($results.SuccessfulRequests)"
    Add-Content -Path $resultFile -Value "Failed:               $($results.FailedRequests)"
    Add-Content -Path $resultFile -Value "Total Time:           $([math]::Round($results.TotalTime, 2)) seconds"
    Add-Content -Path $resultFile -Value "Requests/sec:         $($results.RequestsPerSecond)"
    Add-Content -Path $resultFile -Value "Average Latency:      $($results.AverageLatency) ms"
    Add-Content -Path $resultFile -Value "Min Latency:          $([math]::Round($results.MinLatency, 2)) ms"
    Add-Content -Path $resultFile -Value "Max Latency:          $([math]::Round($results.MaxLatency, 2)) ms"
    Add-Content -Path $resultFile -Value ""

    return $results
}

# Clear previous results
if (Test-Path $resultFile) {
    Remove-Item $resultFile
}

Write-Host "Results will be saved to: $resultFile" -ForegroundColor Cyan
Write-Host ""

# Test 1: HTTP Plain Text
Write-Host ""
Write-Host "######################################################" -ForegroundColor Magenta
Write-Host "#  Test 1: HTTP Plain Text Response" -ForegroundColor Magenta
Write-Host "######################################################" -ForegroundColor Magenta
Write-Host ""

$novaHTTP = Run-Benchmark -Name "Nova_HTTP" -Command "build\Release\nova.exe" -Args "benchmarks\net_bench_nova.ts" -Port 3000
$nodeHTTP = Run-Benchmark -Name "Node_HTTP" -Command "node" -Args "benchmarks\net_bench_node.js" -Port 3000
$bunHTTP = Run-Benchmark -Name "Bun_HTTP" -Command "bun" -Args "benchmarks\net_bench_bun.ts" -Port 3000

# Test 2: HTTP JSON Response
Write-Host ""
Write-Host "######################################################" -ForegroundColor Magenta
Write-Host "#  Test 2: HTTP JSON Response" -ForegroundColor Magenta
Write-Host "######################################################" -ForegroundColor Magenta
Write-Host ""

$novaJSON = Run-Benchmark -Name "Nova_JSON" -Command "build\Release\nova.exe" -Args "benchmarks\net_bench_json_nova.ts" -Port 3002
$nodeJSON = Run-Benchmark -Name "Node_JSON" -Command "node" -Args "benchmarks\net_bench_json_node.js" -Port 3002
$bunJSON = Run-Benchmark -Name "Bun_JSON" -Command "bun" -Args "benchmarks\net_bench_json_bun.ts" -Port 3002

# Test 3: HTTP2
Write-Host ""
Write-Host "######################################################" -ForegroundColor Magenta
Write-Host "#  Test 3: HTTP2 Plain Text Response" -ForegroundColor Magenta
Write-Host "######################################################" -ForegroundColor Magenta
Write-Host ""

$novaHTTP2 = Run-Benchmark -Name "Nova_HTTP2" -Command "build\Release\nova.exe" -Args "benchmarks\net_bench_http2_nova.ts" -Port 3001
$nodeHTTP2 = Run-Benchmark -Name "Node_HTTP2" -Command "node" -Args "benchmarks\net_bench_http2_node.js" -Port 3001

# Summary
Write-Host ""
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "   SUMMARY - Requests per Second" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "HTTP Plain Text:" -ForegroundColor Yellow
Write-Host "  Nova:     $($novaHTTP.RequestsPerSecond) req/sec"
Write-Host "  Node.js:  $($nodeHTTP.RequestsPerSecond) req/sec"
Write-Host "  Bun:      $($bunHTTP.RequestsPerSecond) req/sec"
Write-Host ""

Write-Host "HTTP JSON:" -ForegroundColor Yellow
Write-Host "  Nova:     $($novaJSON.RequestsPerSecond) req/sec"
Write-Host "  Node.js:  $($nodeJSON.RequestsPerSecond) req/sec"
Write-Host "  Bun:      $($bunJSON.RequestsPerSecond) req/sec"
Write-Host ""

Write-Host "HTTP2 Plain Text:" -ForegroundColor Yellow
Write-Host "  Nova:     $($novaHTTP2.RequestsPerSecond) req/sec"
Write-Host "  Node.js:  $($nodeHTTP2.RequestsPerSecond) req/sec"
Write-Host ""

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "   SUMMARY - Average Latency (ms)" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "HTTP Plain Text:" -ForegroundColor Yellow
Write-Host "  Nova:     $($novaHTTP.AverageLatency) ms"
Write-Host "  Node.js:  $($nodeHTTP.AverageLatency) ms"
Write-Host "  Bun:      $($bunHTTP.AverageLatency) ms"
Write-Host ""

Write-Host "HTTP JSON:" -ForegroundColor Yellow
Write-Host "  Nova:     $($novaJSON.AverageLatency) ms"
Write-Host "  Node.js:  $($nodeJSON.AverageLatency) ms"
Write-Host "  Bun:      $($bunJSON.AverageLatency) ms"
Write-Host ""

Write-Host "HTTP2 Plain Text:" -ForegroundColor Yellow
Write-Host "  Nova:     $($novaHTTP2.AverageLatency) ms"
Write-Host "  Node.js:  $($nodeHTTP2.AverageLatency) ms"
Write-Host ""

# Save summary
Add-Content -Path $resultFile -Value "==================================================="
Add-Content -Path $resultFile -Value "SUMMARY - Requests per Second"
Add-Content -Path $resultFile -Value "==================================================="
Add-Content -Path $resultFile -Value ""
Add-Content -Path $resultFile -Value "HTTP Plain Text:"
Add-Content -Path $resultFile -Value "  Nova:     $($novaHTTP.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value "  Node.js:  $($nodeHTTP.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value "  Bun:      $($bunHTTP.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value ""
Add-Content -Path $resultFile -Value "HTTP JSON:"
Add-Content -Path $resultFile -Value "  Nova:     $($novaJSON.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value "  Node.js:  $nodeJSON.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value "  Bun:      $($bunJSON.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value ""
Add-Content -Path $resultFile -Value "HTTP2 Plain Text:"
Add-Content -Path $resultFile -Value "  Nova:     $($novaHTTP2.RequestsPerSecond) req/sec"
Add-Content -Path $resultFile -Value "  Node.js:  $($nodeHTTP2.RequestsPerSecond) req/sec"

Write-Host "Complete! Results saved to: $resultFile" -ForegroundColor Green
Write-Host ""
