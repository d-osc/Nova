$tests = Get-ChildItem -Path "tests\conformance\*.ts"
$passed = 0
$failed = 0
$failedTests = @()
$total = $tests.Count

Write-Host "Running $total conformance tests..."
Write-Host "============================================"

foreach ($test in $tests) {
    $name = $test.BaseName

    $proc = Start-Process -FilePath ".\build\Release\nova.exe" -ArgumentList @("--no-cache", $test.FullName) -Wait -PassThru -RedirectStandardOutput "$env:TEMP\nova_out.txt" -RedirectStandardError "$env:TEMP\nova_err.txt" -NoNewWindow -WorkingDirectory (Get-Location).Path
    $code = $proc.ExitCode

    if ($code -eq 0) {
        $passed++
        Write-Host "[PASS] $name (exit $code)" -ForegroundColor Green
    } else {
        $failed++
        $failedTests += "$name (exit $code)"
        Write-Host "[FAIL] $name (exit $code)" -ForegroundColor Red
    }
}

Write-Host "============================================"
Write-Host "Results: $passed / $total passed, $failed failed"
if ($failedTests.Count -gt 0) {
    Write-Host ""
    Write-Host "Failed tests:"
    foreach ($t in $failedTests) { Write-Host "  - $t" }
}
