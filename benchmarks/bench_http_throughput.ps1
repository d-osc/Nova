# HTTP Throughput Benchmark Script
# Measures requests/second for Nova, Node.js, and Bun

$Port = 3000
$Duration = 10  # seconds
$Concurrent = 10  # concurrent requests

Write-Host "=== HTTP Throughput Benchmark ===" -ForegroundColor Cyan
Write-Host "Duration: $Duration seconds" -ForegroundColor Yellow
Write-Host "Concurrent connections: $Concurrent" -ForegroundColor Yellow
Write-Host ""

# Function to run benchmark
function Run-Benchmark {
    param (
        [string]$Name,
        [string]$Command,
        [string]$TestFile
    )

    Write-Host "Testing $Name..." -ForegroundColor Green

    # Start server
    $process = Start-Process -FilePath $Command -ArgumentList $TestFile -NoNewWindow -PassThru -RedirectStandardError "bench_${Name}_err.log" -RedirectStandardOutput "bench_${Name}_out.log"

    # Wait for server to start
    Start-Sleep -Seconds 3

    # Test if server is responding
    try {
        $response = Invoke-WebRequest -Uri "http://localhost:$Port/" -TimeoutSec 2 -ErrorAction Stop
        Write-Host "  Server started successfully" -ForegroundColor Gray
    } catch {
        Write-Host "  ERROR: Server failed to start" -ForegroundColor Red
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        return $null
    }

    # Run load test
    $totalRequests = 0
    $successfulRequests = 0
    $startTime = Get-Date
    $endTime = $startTime.AddSeconds($Duration)

    Write-Host "  Running load test..." -ForegroundColor Gray

    # Create concurrent jobs
    $jobs = @()
    for ($i = 0; $i -lt $Concurrent; $i++) {
        $jobs += Start-Job -ScriptBlock {
            param($port, $endTime)
            $count = 0
            while ((Get-Date) -lt $endTime) {
                try {
                    $null = Invoke-WebRequest -Uri "http://localhost:$port/" -TimeoutSec 1 -ErrorAction Stop
                    $count++
                } catch {
                    # Continue on error
                }
            }
            return $count
        } -ArgumentList $Port, $endTime
    }

    # Wait for all jobs to complete
    $jobResults = $jobs | Wait-Job | Receive-Job
    $jobs | Remove-Job

    $totalRequests = ($jobResults | Measure-Object -Sum).Sum
    $actualDuration = ((Get-Date) - $startTime).TotalSeconds
    $requestsPerSecond = [math]::Round($totalRequests / $actualDuration, 2)

    # Stop server
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    Write-Host "  Total Requests: $totalRequests" -ForegroundColor White
    Write-Host "  Duration: $([math]::Round($actualDuration, 2))s" -ForegroundColor White
    Write-Host "  Requests/sec: $requestsPerSecond" -ForegroundColor Cyan
    Write-Host ""

    return @{
        Name = $Name
        TotalRequests = $totalRequests
        Duration = $actualDuration
        RequestsPerSecond = $requestsPerSecond
    }
}

# Run benchmarks
$results = @()

# Nova
$novaResult = Run-Benchmark -Name "Nova" -Command ".\build\Release\nova.exe" -TestFile "benchmarks\http_hello_nova.ts"
if ($novaResult) { $results += $novaResult }

# Node.js
$nodeResult = Run-Benchmark -Name "Node.js" -Command "node" -TestFile "benchmarks\http_hello_node.js"
if ($nodeResult) { $results += $nodeResult }

# Bun
$bunResult = Run-Benchmark -Name "Bun" -Command "bun" -TestFile "benchmarks\http_hello_bun.ts"
if ($bunResult) { $results += $bunResult }

# Print summary
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Runtime        Requests/sec" -ForegroundColor Yellow
Write-Host "-------        ------------" -ForegroundColor Yellow

foreach ($result in $results | Sort-Object -Property RequestsPerSecond -Descending) {
    $name = $result.Name.PadRight(14)
    Write-Host "$name $($result.RequestsPerSecond)" -ForegroundColor White
}

# Calculate relative performance
if ($results.Count -gt 1) {
    Write-Host ""
    Write-Host "Relative Performance (vs fastest):" -ForegroundColor Yellow
    $fastest = ($results | Sort-Object -Property RequestsPerSecond -Descending)[0]
    foreach ($result in $results) {
        $name = $result.Name.PadRight(14)
        $percent = [math]::Round(($result.RequestsPerSecond / $fastest.RequestsPerSecond) * 100, 1)
        Write-Host "$name ${percent}%" -ForegroundColor White
    }
}
