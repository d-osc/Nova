# HTTP Server Benchmarking Script for Windows
# Measures throughput, latency, CPU, and memory usage

param(
    [string]$ServerType = "node",  # node, bun, or nova
    [int]$Port = 3000,
    [int]$Duration = 10,           # seconds
    [int]$Concurrency = 50,        # parallel connections
    [int]$Warmup = 2               # warmup seconds
)

$ErrorActionPreference = "Stop"

# Function to start server process
function Start-Server {
    param([string]$Type, [int]$Port)

    Write-Host "Starting $Type server on port $Port..." -ForegroundColor Cyan

    switch ($Type) {
        "node" {
            $process = Start-Process -FilePath "node" -ArgumentList "benchmarks/http_node.js" `
                -PassThru -NoNewWindow -RedirectStandardOutput "bench_node.log" `
                -RedirectStandardError "bench_node_err.log"
            $env:PORT = $Port
        }
        "bun" {
            $process = Start-Process -FilePath "bun" -ArgumentList "benchmarks/http_bun.ts" `
                -PassThru -NoNewWindow -RedirectStandardOutput "bench_bun.log" `
                -RedirectStandardError "bench_bun_err.log"
            $env:PORT = $Port
        }
        default {
            throw "Unknown server type: $Type"
        }
    }

    # Wait for server to start
    Start-Sleep -Seconds 2

    # Test if server is responding
    $maxRetries = 10
    $retry = 0
    while ($retry -lt $maxRetries) {
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:$Port" -TimeoutSec 1 -UseBasicParsing
            Write-Host "Server is ready!" -ForegroundColor Green
            return $process
        } catch {
            $retry++
            Start-Sleep -Milliseconds 500
        }
    }

    throw "Server failed to start or respond"
}

# Function to run benchmark
function Run-Benchmark {
    param([int]$Port, [int]$Duration, [int]$Concurrency, [int]$Warmup)

    Write-Host "`nRunning warmup for $Warmup seconds..." -ForegroundColor Yellow
    $warmupEnd = (Get-Date).AddSeconds($Warmup)
    while ((Get-Date) -lt $warmupEnd) {
        try {
            Invoke-WebRequest -Uri "http://localhost:$Port" -UseBasicParsing -TimeoutSec 1 | Out-Null
        } catch {}
    }

    Write-Host "Running benchmark for $Duration seconds with $Concurrency concurrent connections..." -ForegroundColor Yellow

    $url = "http://localhost:$Port"
    $client = New-Object System.Net.Http.HttpClient
    $client.Timeout = [TimeSpan]::FromSeconds(5)

    $startTime = Get-Date
    $endTime = $startTime.AddSeconds($Duration)
    $requests = 0
    $errors = 0
    $latencies = @()

    $jobs = @()

    # Create worker function
    $workerScript = {
        param($Url, $EndTime)

        $client = New-Object System.Net.Http.HttpClient
        $client.Timeout = [TimeSpan]::FromSeconds(5)
        $localRequests = 0
        $localErrors = 0
        $localLatencies = @()

        while ((Get-Date) -lt $EndTime) {
            $reqStart = Get-Date
            try {
                $response = $client.GetAsync($Url).Result
                $reqEnd = Get-Date
                $latencyMs = ($reqEnd - $reqStart).TotalMilliseconds
                $localLatencies += $latencyMs
                $localRequests++
                $response.Dispose()
            } catch {
                $localErrors++
            }
        }

        $client.Dispose()
        return @{
            Requests = $localRequests
            Errors = $localErrors
            Latencies = $localLatencies
        }
    }

    # Start concurrent workers
    for ($i = 0; $i -lt $Concurrency; $i++) {
        $job = Start-Job -ScriptBlock $workerScript -ArgumentList $url, $endTime
        $jobs += $job
    }

    # Wait for all jobs to complete
    Write-Host "Benchmark running..." -ForegroundColor Cyan
    $jobs | Wait-Job | Out-Null

    # Collect results
    foreach ($job in $jobs) {
        $result = Receive-Job -Job $job
        $requests += $result.Requests
        $errors += $result.Errors
        $latencies += $result.Latencies
        Remove-Job -Job $job
    }

    $actualDuration = ((Get-Date) - $startTime).TotalSeconds
    $client.Dispose()

    # Calculate statistics
    $throughput = [math]::Round($requests / $actualDuration, 2)
    $sortedLatencies = $latencies | Sort-Object
    $count = $sortedLatencies.Count

    $p50 = if ($count -gt 0) { $sortedLatencies[[math]::Floor($count * 0.50)] } else { 0 }
    $p90 = if ($count -gt 0) { $sortedLatencies[[math]::Floor($count * 0.90)] } else { 0 }
    $p99 = if ($count -gt 0) { $sortedLatencies[[math]::Floor($count * 0.99)] } else { 0 }
    $avg = if ($count -gt 0) { ($sortedLatencies | Measure-Object -Average).Average } else { 0 }
    $min = if ($count -gt 0) { ($sortedLatencies | Measure-Object -Minimum).Minimum } else { 0 }
    $max = if ($count -gt 0) { ($sortedLatencies | Measure-Object -Maximum).Maximum } else { 0 }

    return @{
        TotalRequests = $requests
        Errors = $errors
        Duration = [math]::Round($actualDuration, 2)
        Throughput = $throughput
        LatencyAvg = [math]::Round($avg, 2)
        LatencyMin = [math]::Round($min, 2)
        LatencyMax = [math]::Round($max, 2)
        LatencyP50 = [math]::Round($p50, 2)
        LatencyP90 = [math]::Round($p90, 2)
        LatencyP99 = [math]::Round($p99, 2)
    }
}

# Main execution
try {
    Write-Host "`n========================================" -ForegroundColor Green
    Write-Host "HTTP Server Benchmark - $ServerType" -ForegroundColor Green
    Write-Host "========================================`n" -ForegroundColor Green

    # Start server
    $serverProcess = Start-Server -Type $ServerType -Port $Port

    # Get initial resource usage
    $initialCpu = (Get-Counter "\Processor(_Total)\% Processor Time").CounterSamples[0].CookedValue
    $initialMem = (Get-Process -Id $serverProcess.Id).WorkingSet64 / 1MB

    Write-Host "Initial Memory: $([math]::Round($initialMem, 2)) MB" -ForegroundColor Cyan

    # Run benchmark
    $results = Run-Benchmark -Port $Port -Duration $Duration -Concurrency $Concurrency -Warmup $Warmup

    # Get final resource usage
    $finalCpu = (Get-Counter "\Processor(_Total)\% Processor Time").CounterSamples[0].CookedValue
    $finalMem = (Get-Process -Id $serverProcess.Id).WorkingSet64 / 1MB

    # Print results
    Write-Host "`n========================================" -ForegroundColor Green
    Write-Host "Results" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "Total Requests:    $($results.TotalRequests)" -ForegroundColor White
    Write-Host "Errors:            $($results.Errors)" -ForegroundColor White
    Write-Host "Duration:          $($results.Duration) seconds" -ForegroundColor White
    Write-Host "Throughput:        $($results.Throughput) req/sec" -ForegroundColor Yellow
    Write-Host "" -ForegroundColor White
    Write-Host "Latency:" -ForegroundColor White
    Write-Host "  Min:             $($results.LatencyMin) ms" -ForegroundColor White
    Write-Host "  Avg:             $($results.LatencyAvg) ms" -ForegroundColor White
    Write-Host "  Max:             $($results.LatencyMax) ms" -ForegroundColor White
    Write-Host "  P50:             $($results.LatencyP50) ms" -ForegroundColor Yellow
    Write-Host "  P90:             $($results.LatencyP90) ms" -ForegroundColor Yellow
    Write-Host "  P99:             $($results.LatencyP99) ms" -ForegroundColor Yellow
    Write-Host "" -ForegroundColor White
    Write-Host "Resources:" -ForegroundColor White
    Write-Host "  Memory (final):  $([math]::Round($finalMem, 2)) MB" -ForegroundColor Cyan
    Write-Host "========================================`n" -ForegroundColor Green

} finally {
    # Cleanup: Stop server
    if ($serverProcess) {
        Write-Host "Stopping server..." -ForegroundColor Yellow
        Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
