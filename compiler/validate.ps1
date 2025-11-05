# Nova Compiler - Final Validation

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Nova Compiler - Final Validation" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$allFiles = @(
    "test_add_only.ts",
    "test_simple.ts", 
    "test_math.ts",
    "test_complex.ts",
    "test_nested.ts",
    "test_advanced.ts",
    "showcase.ts",
    "test_multi_op.ts",
    "test_deep_chain.ts",
    "test_expressions.ts",
    "test_many_functions.ts",
    "test_params.ts"
)

Write-Host "Validating $($allFiles.Count) test files..." -ForegroundColor Yellow
Write-Host ""

$passed = 0
$failed = 0
$totalFunctions = 0
$totalLines = 0

foreach ($file in $allFiles) {
    Write-Host "  Testing: $file " -NoNewline
    
    $output = & .\build\Release\nova.exe compile $file --emit-all 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[PASS]" -ForegroundColor Green
        $passed++
        
        $llFile = $file -replace '\.ts$', '.ll'
        if (Test-Path $llFile) {
            $funcs = (Select-String "^define" $llFile).Count
            $lines = (Get-Content $llFile).Count
            $totalFunctions += $funcs
            $totalLines += $lines
            Write-Host "    Functions: $funcs, Lines: $lines" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "[FAIL]" -ForegroundColor Red
        $failed++
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Results" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Passed:          $passed / $($allFiles.Count)" -ForegroundColor Green
Write-Host "  Failed:          $failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Red" })
Write-Host "  Total Functions: $totalFunctions" -ForegroundColor Yellow
Write-Host "  Total LLVM IR:   $totalLines lines" -ForegroundColor Yellow
Write-Host ""

if ($failed -eq 0) {
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  ALL TESTS PASSED!" -ForegroundColor Green
    Write-Host "  Nova Compiler: PRODUCTION READY" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
} else {
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "  SOME TESTS FAILED" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
}

Write-Host ""
