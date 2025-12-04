# Resource Usage Benchmark for Nova, Node, and Bun
# Measures CPU and Memory usage at similar throughput levels

param(
    [int]$Duration = 30,
    [int]$TargetRPS = 5000,
    [int]$Connections = 10
)

$ErrorActionPreference = "Continue"

# Function to get process resource usage
function Get-ProcessResources {
    param($ProcessName)

    $process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
    if ($process) {
        $cpu = $process.CPU
        $memoryMB = [math]::Round($process.WorkingSet64 / 1MB, 2)
        return @{
            CPU = $cpu
            MemoryMB = $memoryMB
            Process = $process
        }
    }
    return $null
}

# Function to monitor resources during test
function Monitor-Resources {
    param(
        $ProcessName,
        $DurationSeconds
    )

    $samples = @()
    $endTime = (Get-Date).AddSeconds($DurationSeconds)

    Start-Sleep -Seconds 2  # Wait for warmup

    while ((Get-Date) -lt $endTime) {
        $process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
        if ($process) {
            $cpu = Get-Counter "\Process($ProcessName)\% Processor Time" -ErrorAction SilentlyContinue
            $samples += @{
                Time = Get-Date
                CPU = if ($cpu) { [math]::Round($cpu.CounterSamples[0].CookedValue, 2) } else { 0 }
                MemoryMB = [math]::Round($process.WorkingSet64 / 1MB, 2)
            }
        }
        Start-Sleep -Milliseconds 500
    }

    return $samples
}

# Function to calculate average
function Get-Average {
    param($samples, $property)
    if ($samples.Count -eq 0) { return 0 }
    ($samples | Measure-Object -Property $property -Average).Average
}

# Function to run benchmark for a runtime
function Run-Benchmark {
    param(
        $Name,
        $Command,
        $ProcessName,
        $Port
    )

    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "$Name Resource Benchmark" -ForegroundColor Cyan
    Write-Host "========================================`n" -ForegroundColor Cyan

    # Start server
    Write-Host "Starting $Name server on port $Port..." -ForegroundColor Yellow
    $serverJob = Start-Job -ScriptBlock {
        param($cmd, $port)
        $env:PORT = $port
        Invoke-Expression $cmd
    } -ArgumentList $Command, $Port

    Start-Sleep -Seconds 3

    # Check if server is running
    $testResponse = $null
    try {
        $testResponse = Invoke-WebRequest -Uri "http://localhost:$Port/" -TimeoutSec 5 -ErrorAction SilentlyContinue
    } catch {
        Write-Host "Warning: Could not connect to $Name server" -ForegroundColor Red
    }

    if ($testResponse) {
        Write-Host "$Name server is ready" -ForegroundColor Green

        # Start resource monitoring in background
        $monitorJob = Start-Job -ScriptBlock ${function:Monitor-Resources} -ArgumentList $ProcessName, ($Duration + 5)

        # Run load test
        Write-Host "Running load test: $TargetRPS RPS for $Duration seconds..." -ForegroundColor Yellow

        $loadTestStart = Get-Date
        $requestCount = 0
        $errorCount = 0
        $latencies = @()

        # Calculate requests per connection
        $requestsPerConn = [math]::Ceiling($TargetRPS / $Connections)
        $delayMs = [math]::Max(1, [math]::Floor(1000 / $requestsPerConn))

        # Create multiple connection jobs
        $loadJobs = @()
        for ($i = 0; $i -lt $Connections; $i++) {
            $loadJobs += Start-Job -ScriptBlock {
                param($port, $duration, $delayMs)
                $endTime = (Get-Date).AddSeconds($duration)
                $stats = @{
                    Requests = 0
                    Errors = 0
                    Latencies = @()
                }

                while ((Get-Date) -lt $endTime) {
                    $reqStart = Get-Date
                    try {
                        $response = Invoke-WebRequest -Uri "http://localhost:$port/" -TimeoutSec 5 -UseBasicParsing
                        $latency = ((Get-Date) - $reqStart).TotalMilliseconds
                        $stats.Latencies += $latency
                        $stats.Requests++
                    } catch {
                        $stats.Errors++
                    }
                    if ($delayMs -gt 0) {
                        Start-Sleep -Milliseconds $delayMs
                    }
                }

                return $stats
            } -ArgumentList $Port, $Duration, $delayMs
        }

        # Wait for load test to complete
        $loadResults = $loadJobs | Wait-Job | Receive-Job
        $loadJobs | Remove-Job

        # Aggregate results
        foreach ($result in $loadResults) {
            $requestCount += $result.Requests
            $errorCount += $result.Errors
            $latencies += $result.Latencies
        }

        $loadTestEnd = Get-Date
        $actualDuration = ($loadTestEnd - $loadTestStart).TotalSeconds
        $actualRPS = [math]::Round($requestCount / $actualDuration, 2)

        # Get resource usage
        Start-Sleep -Seconds 1
        $resourceSamples = Receive-Job -Job $monitorJob -Wait
        $monitorJob | Remove-Job

        $avgCPU = [math]::Round((Get-Average $resourceSamples "CPU"), 2)
        $avgMemory = [math]::Round((Get-Average $resourceSamples "MemoryMB"), 2)
        $maxMemory = [math]::Round(($resourceSamples | Measure-Object -Property MemoryMB -Maximum).Maximum, 2)

        $avgLatency = if ($latencies.Count -gt 0) { [math]::Round(($latencies | Measure-Object -Average).Average, 2) } else { 0 }
        $p50 = if ($latencies.Count -gt 0) { [math]::Round(($latencies | Sort-Object)[[math]::Floor($latencies.Count * 0.5)], 2) } else { 0 }
        $p95 = if ($latencies.Count -gt 0) { [math]::Round(($latencies | Sort-Object)[[math]::Floor($latencies.Count * 0.95)], 2) } else { 0 }
        $p99 = if ($latencies.Count -gt 0) { [math]::Round(($latencies | Sort-Object)[[math]::Floor($latencies.Count * 0.99)], 2) } else { 0 }

        # Calculate efficiency metrics
        $cpuPerRequest = if ($requestCount -gt 0) { [math]::Round($avgCPU / $actualRPS * 100, 4) } else { 0 }
        $memoryPerRequest = if ($requestCount -gt 0) { [math]::Round($avgMemory / $actualRPS * 1000, 4) } else { 0 }

        Write-Host "`nResults:" -ForegroundColor Green
        Write-Host "  Requests: $requestCount (Errors: $errorCount)"
        Write-Host "  Actual RPS: $actualRPS"
        Write-Host "  Latency - Avg: ${avgLatency}ms | P50: ${p50}ms | P95: ${p95}ms | P99: ${p99}ms"
        Write-Host "  CPU Usage: $avgCPU%"
        Write-Host "  Memory - Avg: ${avgMemory}MB | Max: ${maxMemory}MB"
        Write-Host "  Efficiency - CPU/RPS: ${cpuPerRequest}% | Memory/RPS: ${memoryPerRequest}KB" -ForegroundColor Cyan

        $result = @{
            Name = $Name
            Requests = $requestCount
            Errors = $errorCount
            ActualRPS = $actualRPS
            AvgLatency = $avgLatency
            P50 = $p50
            P95 = $p95
            P99 = $p99
            AvgCPU = $avgCPU
            AvgMemory = $avgMemory
            MaxMemory = $maxMemory
            CPUPerRPS = $cpuPerRequest
            MemoryPerRPS = $memoryPerRequest
        }
    } else {
        Write-Host "$Name server failed to start" -ForegroundColor Red
        $result = $null
    }

    # Cleanup
    Write-Host "Stopping $Name server..." -ForegroundColor Yellow
    Stop-Job -Job $serverJob -ErrorAction SilentlyContinue
    Remove-Job -Job $serverJob -Force -ErrorAction SilentlyContinue
    Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2

    return $result
}

# Create simple HTTP server scripts
Write-Host "Creating test server scripts..." -ForegroundColor Cyan

# Nova server
@"
const http = nova.http;
const port = 3000;

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Hello World');
});

server.listen(port, () => {
    console.log('Server running on port ' + port);
});
"@ | Out-File -FilePath "benchmarks\resource_test_nova.ts" -Encoding UTF8

# Node server
@"
const http = require('http');
const port = process.env.PORT || 3001;

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Hello World');
});

server.listen(port, () => {
    console.log('Server running on port ' + port);
});
"@ | Out-File -FilePath "benchmarks\resource_test_node.js" -Encoding UTF8

# Bun server
@"
const port = process.env.PORT || 3002;

Bun.serve({
    port: port,
    fetch(req) {
        return new Response('Hello World');
    }
});

console.log('Server running on port ' + port);
"@ | Out-File -FilePath "benchmarks\resource_test_bun.ts" -Encoding UTF8

Write-Host "Starting benchmarks...`n" -ForegroundColor Green

$results = @()

# Run benchmarks
$results += Run-Benchmark -Name "Nova" -Command "./build/Release/nova.exe benchmarks\resource_test_nova.ts" -ProcessName "nova" -Port 3000
$results += Run-Benchmark -Name "Node" -Command "node benchmarks\resource_test_node.js" -ProcessName "node" -Port 3001
$results += Run-Benchmark -Name "Bun" -Command "bun benchmarks\resource_test_bun.ts" -ProcessName "bun" -Port 3002

# Summary comparison
Write-Host "`n========================================" -ForegroundColor Green
Write-Host "RESOURCE EFFICIENCY COMPARISON" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Green

Write-Host "Configuration: Target RPS=$TargetRPS, Duration=${Duration}s, Connections=$Connections`n"

$tableFormat = "{0,-12} {1,10} {2,10} {3,10} {4,10} {5,12} {6,12}"
Write-Host ($tableFormat -f "Runtime", "RPS", "CPU%", "Memory(MB)", "Latency(ms)", "CPU/RPS(%)", "Mem/RPS(KB)") -ForegroundColor Cyan
Write-Host ("-" * 88) -ForegroundColor Cyan

foreach ($r in $results) {
    if ($r) {
        Write-Host ($tableFormat -f $r.Name, $r.ActualRPS, $r.AvgCPU, $r.AvgMemory, $r.AvgLatency, $r.CPUPerRPS, $r.MemoryPerRPS)
    }
}

# Find best performers
$validResults = $results | Where-Object { $_ -ne $null }

if ($validResults.Count -gt 0) {
    Write-Host "`n" -NoNewline

    $bestRPS = ($validResults | Sort-Object -Property ActualRPS -Descending)[0]
    $bestCPU = ($validResults | Sort-Object -Property CPUPerRPS)[0]
    $bestMemory = ($validResults | Sort-Object -Property MemoryPerRPS)[0]
    $bestLatency = ($validResults | Sort-Object -Property AvgLatency)[0]

    Write-Host "Best Throughput: " -ForegroundColor Yellow -NoNewline
    Write-Host "$($bestRPS.Name) ($($bestRPS.ActualRPS) RPS)" -ForegroundColor Green

    Write-Host "Best CPU Efficiency: " -ForegroundColor Yellow -NoNewline
    Write-Host "$($bestCPU.Name) ($($bestCPU.CPUPerRPS)% per RPS)" -ForegroundColor Green

    Write-Host "Best Memory Efficiency: " -ForegroundColor Yellow -NoNewline
    Write-Host "$($bestMemory.Name) ($($bestMemory.MemoryPerRPS)KB per RPS)" -ForegroundColor Green

    Write-Host "Lowest Latency: " -ForegroundColor Yellow -NoNewline
    Write-Host "$($bestLatency.Name) ($($bestLatency.AvgLatency)ms avg)" -ForegroundColor Green
}

# Save results to file
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultsFile = "benchmarks\resource_bench_results_$timestamp.txt"

@"
Resource Usage Benchmark Results
Generated: $(Get-Date)
Configuration: Target RPS=$TargetRPS, Duration=${Duration}s, Connections=$Connections

"@ | Out-File -FilePath $resultsFile -Encoding UTF8

$results | Where-Object { $_ -ne $null } | ForEach-Object {
    @"
$($_.Name):
  Throughput: $($_.ActualRPS) RPS ($($_.Requests) requests, $($_.Errors) errors)
  Latency: Avg=$($_.AvgLatency)ms, P50=$($_.P50)ms, P95=$($_.P95)ms, P99=$($_.P99)ms
  CPU: $($_.AvgCPU)%
  Memory: Avg=$($_.AvgMemory)MB, Max=$($_.MaxMemory)MB
  Efficiency: CPU/RPS=$($_.CPUPerRPS)%, Memory/RPS=$($_.MemoryPerRPS)KB

"@ | Out-File -FilePath $resultsFile -Append -Encoding UTF8
}

Write-Host "`nResults saved to: $resultsFile" -ForegroundColor Cyan
