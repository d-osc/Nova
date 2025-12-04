# Quick HTTP throughput test
Write-Host "Testing Nova HTTP throughput..." -ForegroundColor Cyan

$start = Get-Date
1..100 | ForEach-Object -Parallel {
    try {
        Invoke-WebRequest -Uri 'http://localhost:3000/' -UseBasicParsing -ErrorAction Stop | Out-Null
    } catch {
        # Ignore errors
    }
} -ThrottleLimit 10

$duration = (Get-Date) - $start
$rps = [math]::Round(100 / $duration.TotalSeconds, 2)

Write-Host "100 requests completed in $($duration.TotalSeconds) seconds" -ForegroundColor White
Write-Host "Throughput: $rps requests/sec" -ForegroundColor Green
