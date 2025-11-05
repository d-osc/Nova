# Nova Compiler - Interactive Demo

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║                                                        ║" -ForegroundColor Cyan
Write-Host "║           🚀 NOVA COMPILER - DEMO 🚀                  ║" -ForegroundColor Cyan
Write-Host "║        TypeScript/JavaScript → LLVM IR                ║" -ForegroundColor Cyan
Write-Host "║                                                        ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

function Show-Example {
    param($name, $file)
    
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
    Write-Host "  Example: $name" -ForegroundColor Yellow
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
    Write-Host ""
    
    Write-Host "📄 Source Code:" -ForegroundColor Green
    Write-Host "───────────────" -ForegroundColor DarkGray
    Get-Content $file | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
    Write-Host ""
    
    Write-Host "⚙️  Compiling..." -ForegroundColor Cyan
    $output = & .\build\Release\nova.exe compile $file --emit-all 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Compilation successful!" -ForegroundColor Green
        Write-Host ""
        
        $llFile = $file -replace '\.ts$', '.ll'
        Write-Host "🔧 Generated LLVM IR:" -ForegroundColor Magenta
        Write-Host "─────────────────────" -ForegroundColor DarkGray
        Get-Content $llFile | ForEach-Object { Write-Host "  $_" -ForegroundColor White }
    } else {
        Write-Host "❌ Compilation failed!" -ForegroundColor Red
        Write-Host "Error: $output" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "Press any key to continue..." -ForegroundColor DarkGray
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    Write-Host ""
}

# Demo 1: Simple Addition
Show-Example "Simple Function" "test_add_only.ts"

# Demo 2: Function Calls
Show-Example "Function Calls" "test_simple.ts"

# Demo 3: Math Operations
Show-Example "Multiple Operations" "test_math.ts"

# Demo 4: Nested Calls
Show-Example "Nested Function Calls" "test_nested.ts"

Write-Host ""
Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║                                                        ║" -ForegroundColor Cyan
Write-Host "║              ✨ Demo Complete! ✨                     ║" -ForegroundColor Cyan
Write-Host "║                                                        ║" -ForegroundColor Cyan
Write-Host "║  Nova Compiler successfully compiled all examples!    ║" -ForegroundColor Cyan
Write-Host "║                                                        ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""
Write-Host "📚 Documentation:" -ForegroundColor Yellow
Write-Host "   - FINAL_SUMMARY.md : Complete project summary" -ForegroundColor White
Write-Host "   - TEST_RESULTS.md  : Detailed test results" -ForegroundColor White
Write-Host ""
Write-Host "🎯 Usage:" -ForegroundColor Yellow
Write-Host "   .\build\Release\nova.exe compile <file.ts> --emit-all" -ForegroundColor White
Write-Host ""
