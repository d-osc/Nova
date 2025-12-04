# HTTP Performance Benchmark Script
# Measures RPS, Latency (p50/p90/p99), CPU, and Memory usage

param(
    [int]$Duration = 10,
    [int]$Warmup = 2,
    [int[]]$Concurrency = @(1, 50, 500)
)

$ErrorActionPreference = "Stop"

# Benchmark configurations
$benchmarks = @(
    @{
        Name = "Hello World"
        Servers = @(
            @{ Name = "Nova"; Command = "./build/Release/nova.exe"; Args = "benchmarks/http_hello_nova.ts" },
            @{ Name = "Node"; Command = "node"; Args = "benchmarks/http_hello_node.js" },
            @{ Name = "Bun"; Command = "bun"; Args = "benchmarks/http_hello_bun.ts" }
        )
        Url = "http://localhost:3000/"
    },
    @{
        Name = "Routing (GET /users)"
        Servers = @(
            @{ Name = "Nova"; Command = "./build/Release/nova.exe"; Args = "benchmarks/http_routing_nova.ts" },
            @{ Name = "Node"; Command = "node"; Args = "benchmarks/http_routing_node.js" },
            @{ Name = "Bun"; Command = "bun"; Args = "benchmarks/http_routing_bun.ts" }
        )
        Url = "http://localhost:3000/users"
    }
)

function Start-BenchmarkServer {
    param($ServerConfig)

    Write-Host "Starting $($ServerConfig.Name) server..." -ForegroundColor Cyan

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ServerConfig.Command
    $psi.Arguments = $ServerConfig.Args
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true

    $process = [System.Diagnostics.Process]::Start($psi)

    # Wait for server to start
    $timeout = [DateTime]::Now.AddSeconds(10)
    $started = $false

    while ([DateTime]::Now -lt $timeout -and -not $started) {
        Start-Sleep -Milliseconds 100
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:3000" -TimeoutSec 1 -ErrorAction SilentlyContinue
            if ($response) {
                $started = $true
            }
        } catch {
            # Server not ready yet
        }
    }

    if (-not $started) {
        $process.Kill()
        throw "Server failed to start within timeout"
    }

    Write-Host "$($ServerConfig.Name) server started (PID: $($process.Id))" -ForegroundColor Green
    return $process
}

function Stop-BenchmarkServer {
    param($Process)

    if ($Process -and -not $Process.HasExited) {
        try {
            $Process.Kill()
            $Process.WaitForExit(2000)
        } catch {
            Write-Host "Warning: Failed to stop process: $_" -ForegroundColor Yellow
        }
    }

    # Wait for port to be released
    Start-Sleep -Milliseconds 500
}

function Measure-HttpPerformance {
    param(
        [string]$Url,
        [int]$Concurrency,
        [int]$Duration
    )

    $results = @()
    $startTime = Get-Date
    $endTime = $startTime.AddSeconds($Duration)

    $runspace = [runspacefactory]::CreateRunspacePool(1, $Concurrency)
    $runspace.Open()

    $jobs = @()

    $scriptBlock = {
        param($Url, $EndTime)

        $results = @()

        while ((Get-Date) -lt $EndTime) {
            try {
                $sw = [System.Diagnostics.Stopwatch]::StartNew()
                $response = Invoke-WebRequest -Uri $Url -UseBasicParsing -TimeoutSec 5
                $sw.Stop()

                $results += @{
                    Latency = $sw.Elapsed.TotalMilliseconds
                    StatusCode = $response.StatusCode
                }
            } catch {
                # Skip errors
            }
        }

        return $results
    }

    # Start workers
    for ($i = 0; $i -lt $Concurrency; $i++) {
        $ps = [powershell]::Create().AddScript($scriptBlock).AddArgument($Url).AddArgument($endTime)
        $ps.RunspacePool = $runspace
        $jobs += @{
            PowerShell = $ps
            Handle = $ps.BeginInvoke()
        }
    }

    # Wait for completion
    foreach ($job in $jobs) {
        $workerResults = $job.PowerShell.EndInvoke($job.Handle)
        $results += $workerResults
        $job.PowerShell.Dispose()
    }

    $runspace.Close()
    $runspace.Dispose()

    $actualDuration = ((Get-Date) - $startTime).TotalSeconds

    # Calculate statistics
    $latencies = $results | ForEach-Object { $_.Latency } | Sort-Object
    $successCount = ($results | Where-Object { $_.StatusCode -eq 200 }).Count

    $p50Index = [Math]::Floor($latencies.Count * 0.5)
    $p90Index = [Math]::Floor($latencies.Count * 0.9)
    $p99Index = [Math]::Floor($latencies.Count * 0.99)

    return @{
        TotalRequests = $results.Count
        SuccessCount = $successCount
        RPS = $results.Count / $actualDuration
        AvgLatency = ($latencies | Measure-Object -Average).Average
        P50 = $latencies[$p50Index]
        P90 = $latencies[$p90Index]
        P99 = $latencies[$p99Index]
        MinLatency = $latencies[0]
        MaxLatency = $latencies[-1]
    }
}

function Measure-ProcessResources {
    param(
        [System.Diagnostics.Process]$Process,
        [int]$Duration
    )

    $samples = @()
    $iterations = $Duration * 2  # Sample every 500ms
    $interval = 500

    for ($i = 0; $i -lt $iterations; $i++) {
        try {
            $proc = Get-Process -Id $Process.Id -ErrorAction SilentlyContinue
            if ($proc) {
                $samples += @{
                    Memory = $proc.WorkingSet64 / 1MB
                    CPU = $proc.CPU
                    Timestamp = Get-Date
                }
            }
        } catch {
            # Process might have exited
        }

        Start-Sleep -Milliseconds $interval
    }

    if ($samples.Count -eq 0) {
        return @{
            AvgMemoryMB = 0
            MaxMemoryMB = 0
            CPUPercent = 0
        }
    }

    # Calculate CPU percentage
    $cpuStart = $samples[0].CPU
    $cpuEnd = $samples[-1].CPU
    $cpuPercent = (($cpuEnd - $cpuStart) / $Duration / $env:NUMBER_OF_PROCESSORS) * 100

    return @{
        AvgMemoryMB = ($samples | Measure-Object -Property Memory -Average).Average
        MaxMemoryMB = ($samples | Measure-Object -Property Memory -Maximum).Maximum
        CPUPercent = [Math]::Max(0, $cpuPercent)
    }
}

function Run-Benchmark {
    param($BenchmarkConfig, $ServerConfig, [int]$Concurrency)

    Write-Host "`n$('=' * 70)" -ForegroundColor Yellow
    Write-Host "Benchmark: $($BenchmarkConfig.Name) | Runtime: $($ServerConfig.Name) | Concurrency: $Concurrency" -ForegroundColor Yellow
    Write-Host "$('=' * 70)" -ForegroundColor Yellow

    # Start server
    $serverProcess = $null
    try {
        $serverProcess = Start-BenchmarkServer -ServerConfig $ServerConfig
    } catch {
        Write-Host "Failed to start server: $_" -ForegroundColor Red
        return $null
    }

    # Warmup
    Write-Host "Warming up for $Warmup seconds..." -ForegroundColor Cyan
    try {
        $null = Measure-HttpPerformance -Url $BenchmarkConfig.Url -Concurrency $Concurrency -Duration $Warmup
    } catch {
        Write-Host "Warmup failed: $_" -ForegroundColor Red
        Stop-BenchmarkServer -Process $serverProcess
        return $null
    }

    # Run benchmark
    Write-Host "Running benchmark for $Duration seconds..." -ForegroundColor Cyan

    # Start resource monitoring in background
    $resourceJob = Start-Job -ScriptBlock {
        param($ProcessId, $Duration)

        $samples = @()
        $iterations = $Duration * 2
        $interval = 500

        for ($i = 0; $i -lt $iterations; $i++) {
            try {
                $proc = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
                if ($proc) {
                    $samples += @{
                        Memory = $proc.WorkingSet64 / 1MB
                        CPU = $proc.CPU
                    }
                }
            } catch { }

            Start-Sleep -Milliseconds $interval
        }

        return $samples
    } -ArgumentList $serverProcess.Id, $Duration

    # Run load test
    $loadTestResult = $null
    try {
        $loadTestResult = Measure-HttpPerformance -Url $BenchmarkConfig.Url -Concurrency $Concurrency -Duration $Duration
    } catch {
        Write-Host "Load test failed: $_" -ForegroundColor Red
        Stop-Job -Job $resourceJob
        Remove-Job -Job $resourceJob
        Stop-BenchmarkServer -Process $serverProcess
        return $null
    }

    # Get resource usage
    $resourceSamples = Wait-Job -Job $resourceJob | Receive-Job
    Remove-Job -Job $resourceJob

    $resources = @{
        AvgMemoryMB = 0
        MaxMemoryMB = 0
        CPUPercent = 0
    }

    if ($resourceSamples.Count -gt 0) {
        $cpuStart = $resourceSamples[0].CPU
        $cpuEnd = $resourceSamples[-1].CPU
        $cpuPercent = (($cpuEnd - $cpuStart) / $Duration / $env:NUMBER_OF_PROCESSORS) * 100

        $resources = @{
            AvgMemoryMB = ($resourceSamples | Measure-Object -Property Memory -Average).Average
            MaxMemoryMB = ($resourceSamples | Measure-Object -Property Memory -Maximum).Maximum
            CPUPercent = [Math]::Max(0, $cpuPercent)
        }
    }

    # Stop server
    Stop-BenchmarkServer -Process $serverProcess

    # Print results
    Write-Host "`n--- Results ---" -ForegroundColor Green
    Write-Host "Total Requests: $($loadTestResult.TotalRequests)"
    Write-Host "Successful: $($loadTestResult.SuccessCount)"
    Write-Host "RPS: $($loadTestResult.RPS.ToString('F2')) req/sec"
    Write-Host "`nLatency:"
    Write-Host "  Min: $($loadTestResult.MinLatency.ToString('F2')) ms"
    Write-Host "  Avg: $($loadTestResult.AvgLatency.ToString('F2')) ms"
    Write-Host "  p50: $($loadTestResult.P50.ToString('F2')) ms"
    Write-Host "  p90: $($loadTestResult.P90.ToString('F2')) ms"
    Write-Host "  p99: $($loadTestResult.P99.ToString('F2')) ms"
    Write-Host "  Max: $($loadTestResult.MaxLatency.ToString('F2')) ms"
    Write-Host "`nResources:"
    Write-Host "  Avg Memory: $($resources.AvgMemoryMB.ToString('F2')) MB"
    Write-Host "  Max Memory: $($resources.MaxMemoryMB.ToString('F2')) MB"
    Write-Host "  CPU: $($resources.CPUPercent.ToString('F2'))%"

    return @{
        Benchmark = $BenchmarkConfig.Name
        Runtime = $ServerConfig.Name
        Concurrency = $Concurrency
        TotalRequests = $loadTestResult.TotalRequests
        SuccessCount = $loadTestResult.SuccessCount
        RPS = $loadTestResult.RPS
        AvgLatency = $loadTestResult.AvgLatency
        P50 = $loadTestResult.P50
        P90 = $loadTestResult.P90
        P99 = $loadTestResult.P99
        MinLatency = $loadTestResult.MinLatency
        MaxLatency = $loadTestResult.MaxLatency
        AvgMemoryMB = $resources.AvgMemoryMB
        MaxMemoryMB = $resources.MaxMemoryMB
        CPUPercent = $resources.CPUPercent
    }
}

# Main execution
Write-Host "HTTP Performance Benchmark Suite" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan
Write-Host "Concurrency levels: $($Concurrency -join ', ')"
Write-Host "Duration: ${Duration}s per test"
Write-Host "Warmup: ${Warmup}s per test`n"

$allResults = @()

foreach ($benchmark in $benchmarks) {
    foreach ($server in $benchmark.Servers) {
        foreach ($conc in $Concurrency) {
            $result = Run-Benchmark -BenchmarkConfig $benchmark -ServerConfig $server -Concurrency $conc
            if ($result) {
                $allResults += $result
            }
        }
    }
}

# Print summary
Write-Host "`n`n$('=' * 70)" -ForegroundColor Yellow
Write-Host "SUMMARY" -ForegroundColor Yellow
Write-Host "$('=' * 70)" -ForegroundColor Yellow

foreach ($benchmark in $benchmarks) {
    Write-Host "`n$($benchmark.Name):" -ForegroundColor Cyan
    Write-Host "$('-' * 70)"

    foreach ($conc in $Concurrency) {
        Write-Host "`nConcurrency: $conc" -ForegroundColor Yellow

        $header = "{0,-12} {1,-15} {2,-12} {3,-12} {4,-15} {5,-10}" -f "Runtime", "RPS", "p50 (ms)", "p99 (ms)", "Memory (MB)", "CPU %"
        Write-Host $header
        Write-Host "$('-' * 70)"

        foreach ($server in $benchmark.Servers) {
            $result = $allResults | Where-Object {
                $_.Benchmark -eq $benchmark.Name -and
                $_.Runtime -eq $server.Name -and
                $_.Concurrency -eq $conc
            } | Select-Object -First 1

            if ($result) {
                $line = "{0,-12} {1,-15} {2,-12} {3,-12} {4,-15} {5,-10}" -f `
                    $result.Runtime,
                    $result.RPS.ToString('F2'),
                    $result.P50.ToString('F2'),
                    $result.P99.ToString('F2'),
                    $result.AvgMemoryMB.ToString('F2'),
                    $result.CPUPercent.ToString('F2')

                Write-Host $line
            }
        }
    }
}

Write-Host "`n`nBenchmark completed!" -ForegroundColor Green
