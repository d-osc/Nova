# Nova Compiler - Performance Benchmark# Nova Compiler - Performance Benchmark# Nova Compiler - Performance Benchmark# Nova Compiler - Performance Benchmark# Nova Compiler - Performance Benchmark



Write-Host "========================================"

Write-Host "  Nova Compiler - Performance Benchmark"

Write-Host "========================================"Write-Host ""

Write-Host ""

Write-Host "========================================" -ForegroundColor Cyan

$testFiles = @("test_add_only.ts", "test_simple.ts", "test_math.ts", "test_complex.ts", "test_nested.ts", "test_advanced.ts", "showcase.ts")

Write-Host "  Nova Compiler - Performance Benchmark" -ForegroundColor CyanWrite-Host ""

Write-Host "Running compilation benchmarks..."

Write-Host ""Write-Host "========================================" -ForegroundColor Cyan



$results = @()Write-Host ""Write-Host "========================================" -ForegroundColor Cyan



foreach ($file in $testFiles) {

    Write-Host "  Benchmarking: $file"

    $testFiles = @(Write-Host "  Nova Compiler - Performance Benchmark" -ForegroundColor CyanWrite-Host ""Write-Host ""

    # Warmup

    $null = & .\build\Release\nova.exe compile $file --emit-all 2>&1    @{ Name = "test_add_only.ts"; Desc = "Simple Addition" },

    

    # Measure 5 runs    @{ Name = "test_simple.ts"; Desc = "Function Calls" },Write-Host "========================================" -ForegroundColor Cyan

    $times = @()

    for ($i = 1; $i -le 5; $i++) {    @{ Name = "test_math.ts"; Desc = "All Arithmetic" },

        $sw = [System.Diagnostics.Stopwatch]::StartNew()

        $null = & .\build\Release\nova.exe compile $file --emit-all 2>&1    @{ Name = "test_complex.ts"; Desc = "Chained Calls" },Write-Host ""Write-Host "========================================" -ForegroundColor CyanWrite-Host "╔═══════════════════════════════════════════════╗" -ForegroundColor Cyan

        $sw.Stop()

        $times += $sw.Elapsed.TotalMilliseconds    @{ Name = "test_nested.ts"; Desc = "Nested Calls" },

    }

        @{ Name = "test_advanced.ts"; Desc = "Fibonacci and Factorial" },

    $avg = ($times | Measure-Object -Average).Average

        @{ Name = "showcase.ts"; Desc = "Comprehensive Examples" }

    Write-Host "    Average: $([math]::Round($avg, 2)) ms"

    Write-Host "")$testFiles = @(Write-Host "  Nova Compiler - Performance Benchmark" -ForegroundColor CyanWrite-Host "║   Nova Compiler - Performance Benchmark      ║" -ForegroundColor Cyan

    

    $results += @{

        File = $file

        Avg = $avgWrite-Host "Running compilation benchmarks..." -ForegroundColor Yellow    @{ Name = "test_add_only.ts"; Desc = "Simple Addition" },

    }

}Write-Host ""



Write-Host "========================================"    @{ Name = "test_simple.ts"; Desc = "Function Calls" },Write-Host "========================================" -ForegroundColor CyanWrite-Host "╚═══════════════════════════════════════════════╝" -ForegroundColor Cyan

Write-Host "  Summary"

Write-Host "========================================"$results = @()

Write-Host ""

    @{ Name = "test_math.ts"; Desc = "All Arithmetic" },

foreach ($result in $results) {

    $fileName = $result.File.PadRight(25)foreach ($test in $testFiles) {

    $avgTime = [math]::Round($result.Avg, 2)

    Write-Host "  $fileName : $avgTime ms"    $file = $test.Name    @{ Name = "test_complex.ts"; Desc = "Chained Calls" },Write-Host ""Write-Host ""

}

    $desc = $test.Desc

Write-Host ""

        @{ Name = "test_nested.ts"; Desc = "Nested Calls" },

$totalAvg = ($results | ForEach-Object { $_.Avg } | Measure-Object -Average).Average

Write-Host "  Overall Average: $([math]::Round($totalAvg, 2)) ms"    Write-Host "  Benchmarking: $desc" -ForegroundColor White

Write-Host ""

    Write-Host "    File: $file" -ForegroundColor DarkGray    @{ Name = "test_advanced.ts"; Desc = "Fibonacci & Factorial" },

if ($totalAvg -lt 50) {

    Write-Host "  Performance Grade: EXCELLENT"    

} elseif ($totalAvg -lt 100) {

    Write-Host "  Performance Grade: GOOD"    # Warmup run    @{ Name = "showcase.ts"; Desc = "Comprehensive Examples" }

} else {

    Write-Host "  Performance Grade: ACCEPTABLE"    $null = & .\build\Release\nova.exe compile $file --emit-all 2>&1

}

    )$tests = @($tests = @(

Write-Host ""

    # Measure 5 runs

    $times = @()

    for ($i = 1; $i -le 5; $i++) {

        $sw = [System.Diagnostics.Stopwatch]::StartNew()Write-Host "Running compilation benchmarks..." -ForegroundColor Yellow    @{Name="Simple Addition"; File="test_add_only.ts"},    @{Name="Simple Addition"; File="test_add_only.ts"},

        $null = & .\build\Release\nova.exe compile $file --emit-all 2>&1

        $sw.Stop()Write-Host ""

        $times += $sw.Elapsed.TotalMilliseconds

    }    @{Name="Function Calls"; File="test_simple.ts"},    @{Name="Function Calls"; File="test_simple.ts"},

    

    $avg = ($times | Measure-Object -Average).Average$results = @()

    $min = ($times | Measure-Object -Minimum).Minimum

    $max = ($times | Measure-Object -Maximum).Maximum    @{Name="Math Operations"; File="test_math.ts"},    @{Name="Math Operations"; File="test_math.ts"},

    

    Write-Host "    Average: $([math]::Round($avg, 2)) ms" -ForegroundColor Greenforeach ($test in $testFiles) {

    Write-Host "    Min: $([math]::Round($min, 2)) ms, Max: $([math]::Round($max, 2)) ms" -ForegroundColor DarkGray

    Write-Host ""    $file = $test.Name    @{Name="Chained Calls"; File="test_complex.ts"},    @{Name="Chained Calls"; File="test_complex.ts"},

    

    $results += @{    $desc = $test.Desc

        File = $file

        Desc = $desc        @{Name="Nested Calls"; File="test_nested.ts"}    @{Name="Nested Calls"; File="test_nested.ts"}

        Avg = $avg

        Min = $min    Write-Host "  Benchmarking: $desc" -ForegroundColor White

        Max = $max

    }    Write-Host "    File: $file" -ForegroundColor DarkGray))

}

    

Write-Host "========================================" -ForegroundColor Cyan

Write-Host "  Summary" -ForegroundColor Cyan    # Warmup run

Write-Host "========================================" -ForegroundColor Cyan

Write-Host ""    $null = & .\build\Release\nova.exe compile $file --emit-all 2>&1



foreach ($result in $results) {    $results = @()$results = @()

    $fileName = $result.File.PadRight(25)

    $avgTime = [math]::Round($result.Avg, 2)    # Measure 5 runs

    

    Write-Host "  $fileName : $avgTime ms" -ForegroundColor Yellow    $times = @()

}

    for ($i = 1; $i -le 5; $i++) {

Write-Host ""

        $sw = [System.Diagnostics.Stopwatch]::StartNew()foreach ($test in $tests) {foreach ($test in $tests) {

$totalAvg = ($results | ForEach-Object { $_.Avg } | Measure-Object -Average).Average

Write-Host "  Overall Average: $([math]::Round($totalAvg, 2)) ms" -ForegroundColor Green        $null = & .\build\Release\nova.exe compile $file --emit-all 2>&1

Write-Host ""

        $sw.Stop()    Write-Host "Benchmarking: $($test.Name)..." -ForegroundColor Yellow    Write-Host "Benchmarking: $($test.Name)..." -ForegroundColor Yellow

# Performance grade

if ($totalAvg -lt 50) {        $times += $sw.Elapsed.TotalMilliseconds

    Write-Host "  Performance Grade: EXCELLENT" -ForegroundColor Green

} elseif ($totalAvg -lt 100) {    }        

    Write-Host "  Performance Grade: GOOD" -ForegroundColor Yellow

} else {    

    Write-Host "  Performance Grade: ACCEPTABLE" -ForegroundColor White

}    $avg = ($times | Measure-Object -Average).Average    # Warm-up run    # Warm-up run



Write-Host ""    $min = ($times | Measure-Object -Minimum).Minimum


    $max = ($times | Measure-Object -Maximum).Maximum    $null = & .\build\Release\nova.exe compile $test.File --emit-all 2>&1    $null = & .\build\Release\nova.exe compile $test.File --emit-all 2>&1

    

    Write-Host "    Average: $([math]::Round($avg, 2)) ms" -ForegroundColor Green        

    Write-Host "    Min: $([math]::Round($min, 2)) ms, Max: $([math]::Round($max, 2)) ms" -ForegroundColor DarkGray

    Write-Host ""    # Measure 10 runs    # Measure 10 runs

    

    $results += @{    $times = @()    $times = @()

        File = $file

        Desc = $desc    for ($i = 0; $i -lt 10; $i++) {    for ($i = 0; $i -lt 10; $i++) {

        Avg = $avg

        Min = $min        $sw = [System.Diagnostics.Stopwatch]::StartNew()        $sw = [System.Diagnostics.Stopwatch]::StartNew()

        Max = $max

    }        $null = & .\build\Release\nova.exe compile $test.File --emit-all 2>&1        $null = & .\build\Release\nova.exe compile $test.File --emit-all 2>&1

}

        $sw.Stop()        $sw.Stop()

Write-Host "========================================" -ForegroundColor Cyan

Write-Host "  Summary" -ForegroundColor Cyan        $times += $sw.Elapsed.TotalMilliseconds        $times += $sw.Elapsed.TotalMilliseconds

Write-Host "========================================" -ForegroundColor Cyan

Write-Host ""    }    }



Write-Host "  File                     | Avg Time  | Min Time  | Max Time" -ForegroundColor White        

Write-Host "  --------------------------------------------------------" -ForegroundColor DarkGray

    $avg = ($times | Measure-Object -Average).Average    $avg = ($times | Measure-Object -Average).Average

foreach ($result in $results) {

    $fileName = $result.File.PadRight(24)    $min = ($times | Measure-Object -Minimum).Minimum    $min = ($times | Measure-Object -Minimum).Minimum

    $avgTime = ([math]::Round($result.Avg, 2)).ToString().PadLeft(7)

    $minTime = ([math]::Round($result.Min, 2)).ToString().PadLeft(7)    $max = ($times | Measure-Object -Maximum).Maximum    $max = ($times | Measure-Object -Maximum).Maximum

    $maxTime = ([math]::Round($result.Max, 2)).ToString().PadLeft(7)

            

    Write-Host "  $fileName | $avgTime ms | $minTime ms | $maxTime ms" -ForegroundColor Yellow

}    $results += @{    $results += @{



Write-Host ""        Name = $test.Name        Name = $test.Name



$totalAvg = ($results | ForEach-Object { $_.Avg } | Measure-Object -Average).Average        Average = $avg        Average = $avg

Write-Host "  Overall Average: $([math]::Round($totalAvg, 2)) ms" -ForegroundColor Green

Write-Host ""        Min = $min        Min = $min



# Performance grade        Max = $max        Max = $max

if ($totalAvg -lt 50) {

    Write-Host "  Performance Grade: EXCELLENT ⚡" -ForegroundColor Green    }    }

} elseif ($totalAvg -lt 100) {

    Write-Host "  Performance Grade: GOOD 👍" -ForegroundColor Yellow        

} else {

    Write-Host "  Performance Grade: ACCEPTABLE ✓" -ForegroundColor White    Write-Host "  Avg: $([math]::Round($avg, 2))ms, Min: $([math]::Round($min, 2))ms, Max: $([math]::Round($max, 2))ms" -ForegroundColor Green    Write-Host "  Avg: $([math]::Round($avg, 2))ms, Min: $([math]::Round($min, 2))ms, Max: $([math]::Round($max, 2))ms" -ForegroundColor Green

}

}}

Write-Host ""



Write-Host ""Write-Host ""

Write-Host "========================================" -ForegroundColor CyanWrite-Host "╔═══════════════════════════════════════════════╗" -ForegroundColor Cyan

Write-Host "  Benchmark Results" -ForegroundColor CyanWrite-Host "║            Benchmark Results                  ║" -ForegroundColor Cyan

Write-Host "========================================" -ForegroundColor CyanWrite-Host "╚═══════════════════════════════════════════════╝" -ForegroundColor Cyan

Write-Host ""Write-Host ""



Write-Host ("{0,-25} {1,10} {2,10} {3,10}" -f "Test", "Avg (ms)", "Min (ms)", "Max (ms)") -ForegroundColor WhiteWrite-Host ("{0,-25} {1,10} {2,10} {3,10}" -f "Test", "Avg (ms)", "Min (ms)", "Max (ms)") -ForegroundColor White

Write-Host ("{0,-25} {1,10} {2,10} {3,10}" -f "----", "--------", "--------", "--------") -ForegroundColor DarkGrayWrite-Host ("{0,-25} {1,10} {2,10} {3,10}" -f "----", "--------", "--------", "--------") -ForegroundColor DarkGray



foreach ($result in $results) {foreach ($result in $results) {

    $avg = [math]::Round($result.Average, 2)    $avg = [math]::Round($result.Average, 2)

    $min = [math]::Round($result.Min, 2)    $min = [math]::Round($result.Min, 2)

    $max = [math]::Round($result.Max, 2)    $max = [math]::Round($result.Max, 2)

    Write-Host ("{0,-25} {1,10} {2,10} {3,10}" -f $result.Name, $avg, $min, $max) -ForegroundColor White    Write-Host ("{0,-25} {1,10} {2,10} {3,10}" -f $result.Name, $avg, $min, $max) -ForegroundColor White

}}



$totalAvg = ($results | ForEach-Object { $_.Average } | Measure-Object -Average).Average$totalAvg = ($results | ForEach-Object { $_.Average } | Measure-Object -Average).Average

Write-Host ""Write-Host ""

Write-Host "Overall Average: $([math]::Round($totalAvg, 2))ms" -ForegroundColor GreenWrite-Host "Overall Average: " -NoNewline -ForegroundColor Yellow

Write-Host ""Write-Host "$([math]::Round($totalAvg, 2))ms" -ForegroundColor Green

Write-Host ""

if ($totalAvg -lt 500) {

    Write-Host "Performance: EXCELLENT" -ForegroundColor Greenif ($totalAvg -lt 500) {

} elseif ($totalAvg -lt 1000) {    Write-Host "✓ Performance: EXCELLENT (<500ms)" -ForegroundColor Green

    Write-Host "Performance: GOOD" -ForegroundColor Yellow} elseif ($totalAvg -lt 1000) {

} else {    Write-Host "✓ Performance: GOOD (<1s)" -ForegroundColor Yellow

    Write-Host "Performance: NEEDS OPTIMIZATION" -ForegroundColor Red} else {

}    Write-Host "! Performance: NEEDS OPTIMIZATION (>1s)" -ForegroundColor Red

}

Write-Host ""

Write-Host ""
