Write-Host "========================================"
Write-Host "9. BUNDLER BENCHMARK"
Write-Host "========================================"
Write-Host ""

# Clean up
Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-nova" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-bun" -ErrorAction SilentlyContinue

# Nova Bundler (5 runs)
Write-Host "--- Nova Bundler ---"
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-nova" -ErrorAction SilentlyContinue
    $start = Get-Date
    & "./build/Release/nova.exe" -b "benchmarks/bundler_test/src/index.js" --outDir "benchmarks/bundler_test/dist-nova" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$novaAvg = ($times | Measure-Object -Average).Average
Write-Host ("  Average: {0:N2}ms" -f $novaAvg)

# Bun Bundler (5 runs)
Write-Host "--- Bun Bundler ---"
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-bun" -ErrorAction SilentlyContinue
    $start = Get-Date
    bun build "benchmarks/bundler_test/src/index.js" --outdir "benchmarks/bundler_test/dist-bun" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$bunAvg = ($times | Measure-Object -Average).Average
Write-Host ("  Average: {0:N2}ms" -f $bunAvg)

# Output sizes
Write-Host ""
Write-Host "--- Output Size ---"
if (Test-Path "benchmarks/bundler_test/dist-nova") {
    $novaSize = (Get-ChildItem "benchmarks/bundler_test/dist-nova" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("  Nova: {0:N2} KB" -f ($novaSize / 1024))
}
if (Test-Path "benchmarks/bundler_test/dist-bun") {
    $bunSize = (Get-ChildItem "benchmarks/bundler_test/dist-bun" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("  Bun: {0:N2} KB" -f ($bunSize / 1024))
}
