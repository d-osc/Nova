# Simple Node.js HTTP/2 benchmark

Write-Host "=== Node.js HTTP/2 Benchmark ===" -ForegroundColor Cyan
Write-Host ""

$port = 3000

# Start Node.js HTTP/2 server
Write-Host "Starting Node.js HTTP/2 server..." -ForegroundColor Green
$process = Start-Process -FilePath "node" -ArgumentList "benchmarks\http2_server_node.js" -PassThru -WindowStyle Hidden -RedirectStandardOutput "http2_node_out.txt" -RedirectStandardError "http2_node_err.txt"

Start-Sleep -Seconds 3

# Test connection
Write-Host "Testing HTTP/2 connection..." -ForegroundColor Gray
$testResult = curl.exe --http2-prior-knowledge -s "http://127.0.0.1:$port/" 2>&1
Write-Host "Response: $testResult"
Write-Host ""

# Performance test
Write-Host "Running performance test - 100 requests..." -ForegroundColor Yellow

$times = @()
for ($i = 1; $i -le 100; $i++) {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $null = curl.exe --http2-prior-knowledge -s "http://127.0.0.1:$port/" 2>&1
    $sw.Stop()
    $times += $sw.Elapsed.TotalMilliseconds

    if ($i % 20 -eq 0) {
        Write-Host "  Progress: $i/100"
    }
}

Write-Host ""
Write-Host "Results:" -ForegroundColor Cyan
$avg = ($times | Measure-Object -Average).Average
$min = ($times | Measure-Object -Minimum).Minimum
$max = ($times | Measure-Object -Maximum).Maximum
$median = ($times | Sort-Object)[[math]::Floor($times.Count / 2)]

Write-Host "  Average latency: $([math]::Round($avg, 2))ms" -ForegroundColor White
Write-Host "  Median latency:  $([math]::Round($median, 2))ms" -ForegroundColor White
Write-Host "  Min latency:     $([math]::Round($min, 2))ms" -ForegroundColor White
Write-Host "  Max latency:     $([math]::Round($max, 2))ms" -ForegroundColor White
Write-Host "  Requests/sec:    $([math]::Round(1000 / $avg, 2))" -ForegroundColor White

# Stop server
Write-Host ""
Write-Host "Stopping server..." -ForegroundColor Gray
Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== Benchmark Complete ===" -ForegroundColor Cyan
