# HTTP Throughput Comparison: Nova vs Bun vs Deno vs Node.js
# Simple Hello World benchmark

Write-Host "=== HTTP Throughput Comparison ===" -ForegroundColor Cyan
Write-Host "Test: 500 HTTP requests (sequential)" -ForegroundColor Yellow
Write-Host ""

$results = @()

# Function to benchmark a runtime
function Benchmark-Runtime {
    param(
        [string]$Name,
        [string]$Command,
        [string[]]$Arguments,
        [string]$Color = "White"
    )

    Write-Host "Testing $Name..." -ForegroundColor $Color

    # Start server
    $tempOut = [System.IO.Path]::GetTempFileName()
    $tempErr = [System.IO.Path]::GetTempFileName()
    $process = Start-Process -FilePath $Command -ArgumentList $Arguments -NoNewWindow -PassThru -RedirectStandardError $tempErr -RedirectStandardOutput $tempOut
    Start-Sleep -Seconds 15

    # Test if server is responding
    try {
        $null = Invoke-WebRequest -Uri "http://localhost:3000/" -TimeoutSec 2 -UseBasicParsing -ErrorAction Stop
    } catch {
        Write-Host "  ERROR: Server not responding" -ForegroundColor Red
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        Remove-Item $tempOut -ErrorAction SilentlyContinue
        Remove-Item $tempErr -ErrorAction SilentlyContinue
        return $null
    }

    # Run benchmark
    $start = Get-Date
    $successCount = 0

    for ($i = 1; $i -le 500; $i++) {
        try {
            $null = Invoke-WebRequest -Uri "http://localhost:3000/" -UseBasicParsing -TimeoutSec 5 -ErrorAction Stop
            $successCount++
        } catch {
            # Continue on error
        }
    }

    $duration = (Get-Date) - $start
    $rps = [math]::Round($successCount / $duration.TotalSeconds, 2)

    # Stop server
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2

    # Clean up temp files
    Remove-Item $tempOut -ErrorAction SilentlyContinue
    Remove-Item $tempErr -ErrorAction SilentlyContinue

    Write-Host "  Completed: $successCount requests" -ForegroundColor Gray
    Write-Host "  Duration: $([math]::Round($duration.TotalSeconds, 2))s" -ForegroundColor Gray
    Write-Host "  Throughput: $rps req/sec" -ForegroundColor Green
    Write-Host ""

    return @{
        Name = $Name
        RequestsPerSecond = $rps
        TotalRequests = $successCount
        Duration = $duration.TotalSeconds
    }
}

# Test Nova
$novaResult = Benchmark-Runtime -Name "Nova" -Command ".\build\Release\nova.exe" -Arguments @("benchmarks\http_hello_nova.ts") -Color "Magenta"
if ($novaResult) { $results += $novaResult }

# Test Node.js
$nodeResult = Benchmark-Runtime -Name "Node.js" -Command "node" -Arguments @("benchmarks\http_hello_node.js") -Color "Green"
if ($nodeResult) { $results += $nodeResult }

# Test Bun
$bunResult = Benchmark-Runtime -Name "Bun" -Command "bun" -Arguments @("benchmarks\http_hello_bun.ts") -Color "Yellow"
if ($bunResult) { $results += $bunResult }

# Test Deno
$denoResult = Benchmark-Runtime -Name "Deno" -Command "deno" -Arguments @("run", "--allow-net", "benchmarks\http_hello_deno.ts") -Color "Cyan"
if ($denoResult) { $results += $denoResult }

# Print comparison table
Write-Host "=== RESULTS ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Runtime        Req/sec    Relative" -ForegroundColor Yellow
Write-Host "-------        -------    --------" -ForegroundColor Yellow

$sorted = $results | Sort-Object -Property RequestsPerSecond -Descending
$fastest = $sorted[0].RequestsPerSecond

foreach ($result in $sorted) {
    $name = $result.Name.PadRight(14)
    $rps = $result.RequestsPerSecond.ToString().PadLeft(7)
    $relative = [math]::Round(($result.RequestsPerSecond / $fastest) * 100, 1)
    $relativeStr = "${relative}%".PadLeft(8)

    if ($result.Name -eq "Nova") {
        Write-Host "$name $rps    $relativeStr" -ForegroundColor Magenta
    } else {
        Write-Host "$name $rps    $relativeStr" -ForegroundColor White
    }
}

Write-Host ""
Write-Host "Winner: $($sorted[0].Name) with $($sorted[0].RequestsPerSecond) req/sec" -ForegroundColor Green
