# Simple HTTP/2 Benchmark (works without HTTP/2-specific curl flags)

Write-Host "=== Node.js HTTP/2 Performance Benchmark ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: Testing with standard HTTP connections" -ForegroundColor Yellow
Write-Host "(curl on this system doesn't support HTTP/2 flags)" -ForegroundColor Gray
Write-Host ""

$port = 3000

function Run-Benchmark {
    param(
        [string]$name,
        [int]$count
    )

    Write-Host "Test: $name ($count requests)" -ForegroundColor Yellow

    $times = @()
    $progress = 0

    for ($i = 1; $i -le $count; $i++) {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $null = curl -s "http://127.0.0.1:$port/" 2>&1
        $sw.Stop()
        $times += $sw.Elapsed.TotalMilliseconds

        if ($i % 100 -eq 0) {
            Write-Host "  $i/$count..." -ForegroundColor Gray
        }
    }

    $avg = ($times | Measure-Object -Average).Average
    $min = ($times | Measure-Object -Minimum).Minimum
    $max = ($times | Measure-Object -Maximum).Maximum
    $median = ($times | Sort-Object)[[math]::Floor($times.Count / 2)]
    $p95 = ($times | Sort-Object)[[math]::Floor($times.Count * 0.95)]
    $p99 = ($times | Sort-Object)[[math]::Floor($times.Count * 0.99)]

    Write-Host "  Results:" -ForegroundColor Green
    Write-Host "    Average:  $([math]::Round($avg, 2))ms" -ForegroundColor White
    Write-Host "    Median:   $([math]::Round($median, 2))ms" -ForegroundColor White
    Write-Host "    Min:      $([math]::Round($min, 2))ms" -ForegroundColor White
    Write-Host "    Max:      $([math]::Round($max, 2))ms" -ForegroundColor White
    Write-Host "    P95:      $([math]::Round($p95, 2))ms" -ForegroundColor White
    Write-Host "    P99:      $([math]::Round($p99, 2))ms" -ForegroundColor White
    Write-Host "    Req/sec:  $([math]::Round(1000 / $avg, 2))" -ForegroundColor Cyan
    Write-Host ""

    return @{
        Name = $name
        AvgMs = [math]::Round($avg, 2)
        MedianMs = [math]::Round($median, 2)
        MinMs = [math]::Round($min, 2)
        MaxMs = [math]::Round($max, 2)
        P95Ms = [math]::Round($p95, 2)
        P99Ms = [math]::Round($p99, 2)
        ReqPerSec = [math]::Round(1000 / $avg, 2)
    }
}

# Warmup
Write-Host "Warming up..." -ForegroundColor Gray
for ($i = 1; $i -le 5; $i++) {
    $null = curl -s "http://127.0.0.1:$port/" 2>&1
}
Write-Host ""

# Run tests
$results = @()
$results += Run-Benchmark -name "Light Load" -count 100
$results += Run-Benchmark -name "Medium Load" -count 500
$results += Run-Benchmark -name "Heavy Load" -count 1000

# Summary
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Node.js HTTP/2 Server Performance:" -ForegroundColor White
Write-Host ""

foreach ($result in $results) {
    Write-Host "$($result.Name):" -ForegroundColor Yellow
    Write-Host "  Throughput: $($result.ReqPerSec) req/s" -ForegroundColor Cyan
    Write-Host "  Latency (avg): $($result.AvgMs)ms" -ForegroundColor White
    Write-Host "  Latency (P95): $($result.P95Ms)ms" -ForegroundColor White
    Write-Host ""
}

Write-Host "=== Benchmark Complete ===" -ForegroundColor Green
