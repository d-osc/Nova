Write-Host "========================================"
Write-Host "5. STARTUP TIME (5 runs average)"
Write-Host "========================================"
Write-Host ""

# Node.js startup
Write-Host "--- Node.js ---"
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    $start = Get-Date
    node benchmarks/bench_startup.js 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$nodeAvg = ($times | Measure-Object -Average).Average
Write-Host ("  Average: {0:N2}ms" -f $nodeAvg)

# Bun startup
Write-Host "--- Bun ---"
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    $start = Get-Date
    bun benchmarks/bench_startup.js 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$bunAvg = ($times | Measure-Object -Average).Average
Write-Host ("  Average: {0:N2}ms" -f $bunAvg)
