# Nova Compiler Test Suite Runner

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Nova Compiler - Test Suite" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$tests = @(
    @{Name="Simple Addition"; File="test_add_only.ts"},
    @{Name="Function Calls"; File="test_simple.ts"},
    @{Name="Math Operations"; File="test_math.ts"},
    @{Name="Chained Calls"; File="test_complex.ts"},
    @{Name="Nested Calls"; File="test_nested.ts"}
)

$passed = 0
$failed = 0

foreach ($test in $tests) {
    Write-Host "Testing: $($test.Name) " -NoNewline -ForegroundColor Yellow
    
    $output = & .\build\Release\nova.exe compile $test.File --emit-all 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASSED" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "FAILED" -ForegroundColor Red
        Write-Host "  Error: $output" -ForegroundColor Red
        $failed++
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Results: $passed passed, $failed failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Yellow" })
Write-Host "========================================" -ForegroundColor Cyan

if ($failed -eq 0) {
    Write-Host ""
    Write-Host "All tests passed! Nova Compiler is working!" -ForegroundColor Green
    Write-Host ""
    exit 0
} else {
    exit 1
}
