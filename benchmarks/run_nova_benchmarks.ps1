# Nova Compiler Benchmark Suite
# Tests: Bundler, Transpiler, Startup

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "     NOVA COMPILER BENCHMARK SUITE" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$novaPath = "$PSScriptRoot/../build/Release/nova.exe"

# ============================================
# 1. STARTUP TIME
# ============================================
Write-Host "1. STARTUP TIME (--version)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

$times = @()
for ($i = 0; $i -lt 10; $i++) {
    $start = Get-Date
    & $novaPath --version 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$avgStartup = ($times | Measure-Object -Average).Average
Write-Host ("   Nova Startup: {0:N2}ms" -f $avgStartup) -ForegroundColor Green
Write-Host ""

# ============================================
# 2. BUNDLER PERFORMANCE
# ============================================
Write-Host "2. BUNDLER PERFORMANCE" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

# Create test project if not exists
$testDir = "benchmarks/bundler_test/src"
if (!(Test-Path $testDir)) {
    Write-Host "   Creating test project..."
}

# Small project (5 files)
Write-Host "   Small project (5 files):"
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-small" -ErrorAction SilentlyContinue
    $start = Get-Date
    & $novaPath -b "benchmarks/bundler_test/src/index.js" --outDir "benchmarks/bundler_test/dist-small" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$smallAvg = ($times | Measure-Object -Average).Average
Write-Host ("     Nova: {0:N2}ms" -f $smallAvg) -ForegroundColor Green

# Measure Bun for comparison
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-bun-small" -ErrorAction SilentlyContinue
    $start = Get-Date
    bun build "benchmarks/bundler_test/src/index.js" --outdir "benchmarks/bundler_test/dist-bun-small" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$bunSmallAvg = ($times | Measure-Object -Average).Average
Write-Host ("     Bun:  {0:N2}ms" -f $bunSmallAvg) -ForegroundColor Gray

$speedup = $bunSmallAvg / $smallAvg
Write-Host ("     Nova is {0:N2}x faster" -f $speedup) -ForegroundColor Cyan
Write-Host ""

# ============================================
# 3. TRANSPILER (TypeScript -> JavaScript)
# ============================================
Write-Host "3. TRANSPILER (TS -> JS)" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

# Create a TypeScript file for transpilation
$tsContent = @"
interface User {
    name: string;
    age: number;
}

function greet(user: User): string {
    return \`Hello, \${user.name}!\`;
}

const users: User[] = [
    { name: 'Alice', age: 30 },
    { name: 'Bob', age: 25 }
];

users.forEach(u => console.log(greet(u)));
"@
Set-Content -Path "benchmarks/test_transpile.ts" -Value $tsContent

$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Force "benchmarks/test_transpile_out.js" -ErrorAction SilentlyContinue
    $start = Get-Date
    & $novaPath -b "benchmarks/test_transpile.ts" --outDir "benchmarks/transpile_out" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$transpileAvg = ($times | Measure-Object -Average).Average
Write-Host ("   Nova Transpile: {0:N2}ms" -f $transpileAvg) -ForegroundColor Green
Write-Host ""

# ============================================
# 4. LARGE PROJECT BUNDLING
# ============================================
Write-Host "4. LARGE PROJECT BUNDLING" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

# Create a larger test project
$largeDir = "benchmarks/large_project/src"
New-Item -ItemType Directory -Force -Path $largeDir | Out-Null

# Generate 20 modules
for ($i = 1; $i -le 20; $i++) {
    $moduleContent = @"
export function module${i}Func1() { return 'module$i func1'; }
export function module${i}Func2() { return 'module$i func2'; }
export function module${i}Func3() { return 'module$i func3'; }
export const module${i}Data = [1, 2, 3, 4, 5];
"@
    Set-Content -Path "$largeDir/module$i.js" -Value $moduleContent
}

# Create main index
$indexContent = ""
for ($i = 1; $i -le 20; $i++) {
    $indexContent += "import { module${i}Func1 } from './module$i.js';`n"
}
$indexContent += "`nconsole.log('Large project loaded');"
for ($i = 1; $i -le 20; $i++) {
    $indexContent += "`nconsole.log(module${i}Func1());"
}
Set-Content -Path "$largeDir/index.js" -Value $indexContent

# Benchmark Nova on large project
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/large_project/dist-nova" -ErrorAction SilentlyContinue
    $start = Get-Date
    & $novaPath -b "$largeDir/index.js" --outDir "benchmarks/large_project/dist-nova" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$largeNovaAvg = ($times | Measure-Object -Average).Average
Write-Host ("   Nova (20 modules): {0:N2}ms" -f $largeNovaAvg) -ForegroundColor Green

# Benchmark Bun on large project
$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/large_project/dist-bun" -ErrorAction SilentlyContinue
    $start = Get-Date
    bun build "$largeDir/index.js" --outdir "benchmarks/large_project/dist-bun" 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$largeBunAvg = ($times | Measure-Object -Average).Average
Write-Host ("   Bun (20 modules):  {0:N2}ms" -f $largeBunAvg) -ForegroundColor Gray

$speedup = $largeBunAvg / $largeNovaAvg
Write-Host ("   Nova is {0:N2}x faster" -f $speedup) -ForegroundColor Cyan
Write-Host ""

# ============================================
# 5. OUTPUT SIZE COMPARISON
# ============================================
Write-Host "5. OUTPUT SIZE COMPARISON" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

if (Test-Path "benchmarks/bundler_test/dist-small") {
    $novaSize = (Get-ChildItem "benchmarks/bundler_test/dist-small" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("   Small Project - Nova: {0:N2} KB" -f ($novaSize / 1024)) -ForegroundColor Green
}
if (Test-Path "benchmarks/bundler_test/dist-bun-small") {
    $bunSize = (Get-ChildItem "benchmarks/bundler_test/dist-bun-small" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("   Small Project - Bun:  {0:N2} KB" -f ($bunSize / 1024)) -ForegroundColor Gray
}

if (Test-Path "benchmarks/large_project/dist-nova") {
    $novaLargeSize = (Get-ChildItem "benchmarks/large_project/dist-nova" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("   Large Project - Nova: {0:N2} KB" -f ($novaLargeSize / 1024)) -ForegroundColor Green
}
if (Test-Path "benchmarks/large_project/dist-bun") {
    $bunLargeSize = (Get-ChildItem "benchmarks/large_project/dist-bun" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("   Large Project - Bun:  {0:N2} KB" -f ($bunLargeSize / 1024)) -ForegroundColor Gray
}
Write-Host ""

# ============================================
# 6. MINIFICATION
# ============================================
Write-Host "6. MINIFICATION BENCHMARK" -ForegroundColor Yellow
Write-Host "   ----------------------------------------"

$times = @()
for ($i = 0; $i -lt 5; $i++) {
    Remove-Item -Recurse -Force "benchmarks/bundler_test/dist-minified" -ErrorAction SilentlyContinue
    $start = Get-Date
    & $novaPath -b "benchmarks/bundler_test/src/index.js" --outDir "benchmarks/bundler_test/dist-minified" --minify 2>&1 | Out-Null
    $elapsed = ((Get-Date) - $start).TotalMilliseconds
    $times += $elapsed
}
$minifyAvg = ($times | Measure-Object -Average).Average
Write-Host ("   Nova with minify: {0:N2}ms" -f $minifyAvg) -ForegroundColor Green

if (Test-Path "benchmarks/bundler_test/dist-minified") {
    $minifiedSize = (Get-ChildItem "benchmarks/bundler_test/dist-minified" -Recurse | Measure-Object -Property Length -Sum).Sum
    Write-Host ("   Minified size: {0:N2} KB" -f ($minifiedSize / 1024)) -ForegroundColor Green
}
Write-Host ""

# ============================================
# SUMMARY
# ============================================
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "              SUMMARY" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Nova Compiler Performance:" -ForegroundColor Yellow
Write-Host ("  Startup Time:        {0:N2}ms" -f $avgStartup)
Write-Host ("  Small Bundle:        {0:N2}ms" -f $smallAvg)
Write-Host ("  Large Bundle (20):   {0:N2}ms" -f $largeNovaAvg)
Write-Host ("  Transpile:           {0:N2}ms" -f $transpileAvg)
Write-Host ("  Minify:              {0:N2}ms" -f $minifyAvg)
Write-Host ""
Write-Host "vs Bun:" -ForegroundColor Yellow
Write-Host ("  Small Bundle: Nova {0:N2}x faster" -f ($bunSmallAvg / $smallAvg))
Write-Host ("  Large Bundle: Nova {0:N2}x faster" -f ($largeBunAvg / $largeNovaAvg))
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
