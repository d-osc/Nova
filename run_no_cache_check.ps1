$tests = Get-ChildItem -Path "tests\conformance\*.ts"
$passed = 0; $failed = 0; $failedTests = @()
foreach ($test in $tests) {
    $proc = Start-Process -FilePath ".\build\Release\nova.exe" -ArgumentList @($test.FullName) -Wait -PassThru -RedirectStandardOutput "$env:TEMP\n.txt" -RedirectStandardError "$env:TEMP\e.txt" -NoNewWindow -WorkingDirectory (Get-Location).Path
    $code = $proc.ExitCode
    if ($code -eq 0) { $passed++ } else { $failed++; $failedTests += "$($test.BaseName) (exit $code)" }
}
Write-Host "Results: $passed / $($tests.Count) passed, $failed failed"
if ($failedTests.Count -gt 0) { Write-Host "Failed:"; foreach ($t in $failedTests) { Write-Host "  - $t" } }
