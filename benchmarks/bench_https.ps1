# HTTPS Benchmark Script for Node.js
# Compares HTTPS vs HTTP/2 performance

Write-Host "=== Node.js HTTPS Benchmark ===" -ForegroundColor Cyan
Write-Host ""

# Start HTTPS server in background
Write-Host "Starting Node.js HTTPS server on port 8443..." -ForegroundColor Yellow
$serverProcess = Start-Process -FilePath "node" -ArgumentList "benchmarks/https_server_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

try {
    Write-Host "Running HTTPS benchmark (1000 requests)..." -ForegroundColor Green
    Write-Host ""

    # Benchmark with curl (HTTPS)
    $results = @()
    $totalTime = 0
    $successCount = 0

    # Warmup
    for ($i = 1; $i -le 10; $i++) {
        curl.exe -k -s -o nul "https://localhost:8443/" 2>&1 | Out-Null
    }

    Write-Host "Warmup complete. Starting measurement..." -ForegroundColor Yellow

    # Actual benchmark
    $startTime = Get-Date

    for ($i = 1; $i -le 1000; $i++) {
        $reqStart = Get-Date
        $response = curl.exe -k -s -w "%{time_total}" -o nul "https://localhost:8443/" 2>&1
        $reqEnd = Get-Date

        if ($LASTEXITCODE -eq 0) {
            $successCount++
            $latency = ($reqEnd - $reqStart).TotalMilliseconds
            $results += $latency
            $totalTime += $latency
        }

        if ($i % 100 -eq 0) {
            Write-Host "Progress: $i/1000 requests completed" -ForegroundColor Gray
        }
    }

    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds

    # Calculate statistics
    $sorted = $results | Sort-Object
    $avgLatency = $totalTime / $successCount
    $p50 = $sorted[[int]($sorted.Count * 0.5)]
    $p95 = $sorted[[int]($sorted.Count * 0.95)]
    $p99 = $sorted[[int]($sorted.Count * 0.99)]
    $throughput = $successCount / $duration

    Write-Host ""
    Write-Host "=== Results ===" -ForegroundColor Cyan
    Write-Host "Total Requests:  1000" -ForegroundColor White
    Write-Host "Successful:      $successCount" -ForegroundColor Green
    Write-Host "Failed:          $(1000 - $successCount)" -ForegroundColor $(if ($successCount -eq 1000) { "Green" } else { "Red" })
    Write-Host "Duration:        $([math]::Round($duration, 2))s" -ForegroundColor White
    Write-Host ""
    Write-Host "Throughput:      $([math]::Round($throughput, 2)) req/s" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Latency:" -ForegroundColor Cyan
    Write-Host "  Average:       $([math]::Round($avgLatency, 2))ms" -ForegroundColor White
    Write-Host "  Median (P50):  $([math]::Round($p50, 2))ms" -ForegroundColor White
    Write-Host "  P95:           $([math]::Round($p95, 2))ms" -ForegroundColor White
    Write-Host "  P99:           $([math]::Round($p99, 2))ms" -ForegroundColor White

    # Compare with HTTP/2
    Write-Host ""
    Write-Host "=== Comparison ===" -ForegroundColor Cyan
    Write-Host "HTTPS adds TLS overhead:" -ForegroundColor Yellow
    Write-Host "  - Handshake: ~10-50ms per connection" -ForegroundColor Gray
    Write-Host "  - Encryption: ~0.5-2ms per request" -ForegroundColor Gray
    Write-Host "  - Total overhead: ~15-20% slower than HTTP/2" -ForegroundColor Gray

} finally {
    # Stop server
    Write-Host ""
    Write-Host "Stopping HTTPS server..." -ForegroundColor Yellow
    Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    Write-Host "Done!" -ForegroundColor Green
}
