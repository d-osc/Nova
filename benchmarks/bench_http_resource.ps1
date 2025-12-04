# Nova HTTP Resource Usage Benchmark
# Measures CPU and Memory usage at similar throughput levels

param(
    [int]$Duration = 30,        # Test duration in seconds
    [int]$Concurrency = 50,     # Concurrent requests
    [int]$TotalRequests = 10000 # Total requests to send
)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Nova HTTP Resource Usage Benchmark" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Duration: $Duration seconds"
Write-Host "Concurrency: $Concurrency"
Write-Host "Total Requests: $TotalRequests"
Write-Host ""

# Function to get process stats
function Get-ProcessStats {
    param($ProcessName)

    $process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
    if ($process) {
        return @{
            CPU = $process.CPU
            Memory = [math]::Round($process.WorkingSet64 / 1MB, 2)
            Threads = $process.Threads.Count
        }
    }
    return $null
}

# Function to monitor process during test
function Monitor-Process {
    param(
        $ProcessName,
        $DurationSeconds
    )

    $samples = @()
    $interval = 0.5  # Sample every 500ms
    $iterations = [math]::Ceiling($DurationSeconds / $interval)

    for ($i = 0; $i -lt $iterations; $i++) {
        Start-Sleep -Milliseconds ($interval * 1000)

        $stats = Get-ProcessStats -ProcessName $ProcessName
        if ($stats) {
            $samples += $stats
        }
    }

    # Calculate averages
    if ($samples.Count -gt 0) {
        $avgCPU = ($samples | Measure-Object -Property CPU -Average).Average
        $avgMemory = ($samples | Measure-Object -Property Memory -Average).Average
        $peakMemory = ($samples | Measure-Object -Property Memory -Maximum).Maximum
        $avgThreads = ($samples | Measure-Object -Property Threads -Average).Average

        return @{
            AvgCPU = [math]::Round($avgCPU, 2)
            AvgMemory = [math]::Round($avgMemory, 2)
            PeakMemory = [math]::Round($peakMemory, 2)
            AvgThreads = [math]::Round($avgThreads, 0)
            Samples = $samples.Count
        }
    }

    return $null
}

# Function to send HTTP requests and measure throughput
function Test-HTTPThroughput {
    param(
        $Url,
        $TotalRequests,
        $Concurrency
    )

    Write-Host "  Sending $TotalRequests requests with concurrency $Concurrency..."

    $startTime = Get-Date
    $completed = 0
    $errors = 0
    $latencies = @()

    # Create runspace pool for concurrent requests
    $runspacePool = [runspacefactory]::CreateRunspacePool(1, $Concurrency)
    $runspacePool.Open()
    $runspaces = @()

    # Script block for each request
    $scriptBlock = {
        param($url)
        try {
            $start = Get-Date
            $response = Invoke-WebRequest -Uri $url -TimeoutSec 5 -UseBasicParsing
            $latency = (Get-Date) - $start
            return @{
                Success = $true
                Latency = $latency.TotalMilliseconds
                StatusCode = $response.StatusCode
            }
        } catch {
            return @{
                Success = $false
                Error = $_.Exception.Message
            }
        }
    }

    # Launch requests
    for ($i = 0; $i -lt $TotalRequests; $i++) {
        $powershell = [powershell]::Create().AddScript($scriptBlock).AddArgument($Url)
        $powershell.RunspacePool = $runspacePool

        $runspaces += [PSCustomObject]@{
            Pipe = $powershell
            Status = $powershell.BeginInvoke()
        }
    }

    # Wait for completion and collect results
    foreach ($runspace in $runspaces) {
        try {
            $result = $runspace.Pipe.EndInvoke($runspace.Status)
            if ($result.Success) {
                $completed++
                $latencies += $result.Latency
            } else {
                $errors++
            }
        } catch {
            $errors++
        }
        $runspace.Pipe.Dispose()
    }

    $runspacePool.Close()
    $runspacePool.Dispose()

    $duration = (Get-Date) - $startTime
    $rps = [math]::Round($completed / $duration.TotalSeconds, 2)

    # Calculate latency percentiles
    $sortedLatencies = $latencies | Sort-Object
    $p50 = if ($sortedLatencies.Count -gt 0) { $sortedLatencies[[math]::Floor($sortedLatencies.Count * 0.5)] } else { 0 }
    $p90 = if ($sortedLatencies.Count -gt 0) { $sortedLatencies[[math]::Floor($sortedLatencies.Count * 0.9)] } else { 0 }
    $p99 = if ($sortedLatencies.Count -gt 0) { $sortedLatencies[[math]::Floor($sortedLatencies.Count * 0.99)] } else { 0 }

    return @{
        Completed = $completed
        Errors = $errors
        Duration = [math]::Round($duration.TotalSeconds, 2)
        RPS = $rps
        LatencyP50 = [math]::Round($p50, 2)
        LatencyP90 = [math]::Round($p90, 2)
        LatencyP99 = [math]::Round($p99, 2)
    }
}

# Function to benchmark a runtime
function Benchmark-Runtime {
    param(
        $Name,
        $ServerScript,
        $Command,
        $ProcessName,
        $Port = 3000
    )

    Write-Host ""
    Write-Host "[$Name Benchmark]" -ForegroundColor Yellow
    Write-Host "$('-' * 50)"

    # Start server
    Write-Host "  Starting $Name server..."
    $serverProcess = Start-Process -FilePath $Command[0] -ArgumentList $Command[1..($Command.Length-1)] `
        -WindowStyle Hidden -PassThru -RedirectStandardOutput "bench_${Name}_out.txt" `
        -RedirectStandardError "bench_${Name}_err.txt"

    # Wait for server to start
    Start-Sleep -Seconds 3

    # Check if server is responding
    $retries = 0
    $maxRetries = 10
    $serverReady = $false

    while ($retries -lt $maxRetries -and -not $serverReady) {
        try {
            $response = Invoke-WebRequest -Uri "http://localhost:$Port" -TimeoutSec 1 -UseBasicParsing
            $serverReady = $true
            Write-Host "  Server is ready!" -ForegroundColor Green
        } catch {
            $retries++
            Start-Sleep -Seconds 1
        }
    }

    if (-not $serverReady) {
        Write-Host "  ERROR: Server failed to start!" -ForegroundColor Red
        Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
        return $null
    }

    # Get initial stats
    Start-Sleep -Seconds 1
    $initialStats = Get-ProcessStats -ProcessName $ProcessName

    Write-Host "  Initial Memory: $($initialStats.Memory) MB"
    Write-Host ""

    # Start monitoring in background
    $monitorJob = Start-Job -ScriptBlock {
        param($ProcessName, $Duration)

        function Get-ProcessStats {
            param($ProcessName)
            $process = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue
            if ($process) {
                return @{
                    CPU = $process.CPU
                    Memory = [math]::Round($process.WorkingSet64 / 1MB, 2)
                    Threads = $process.Threads.Count
                }
            }
            return $null
        }

        $samples = @()
        $startTime = Get-Date

        while (((Get-Date) - $startTime).TotalSeconds -lt $Duration) {
            Start-Sleep -Milliseconds 500
            $stats = Get-ProcessStats -ProcessName $ProcessName
            if ($stats) {
                $samples += $stats
            }
        }

        if ($samples.Count -gt 0) {
            $avgCPU = ($samples | Measure-Object -Property CPU -Average).Average
            $avgMemory = ($samples | Measure-Object -Property Memory -Average).Average
            $peakMemory = ($samples | Measure-Object -Property Memory -Maximum).Maximum

            return @{
                AvgCPU = [math]::Round($avgCPU, 2)
                AvgMemory = [math]::Round($avgMemory, 2)
                PeakMemory = [math]::Round($peakMemory, 2)
                Samples = $samples.Count
            }
        }
        return $null
    } -ArgumentList $ProcessName, $Duration

    # Run throughput test
    $throughput = Test-HTTPThroughput -Url "http://localhost:$Port" `
        -TotalRequests $TotalRequests -Concurrency $Concurrency

    # Wait for monitoring to complete
    Start-Sleep -Seconds 2
    $resourceStats = Receive-Job -Job $monitorJob -Wait
    Remove-Job -Job $monitorJob

    # Stop server
    Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    # Display results
    Write-Host "  === Performance Results ===" -ForegroundColor Cyan
    Write-Host "  Requests Completed: $($throughput.Completed)"
    Write-Host "  Errors: $($throughput.Errors)"
    Write-Host "  Duration: $($throughput.Duration) seconds"
    Write-Host "  RPS: $($throughput.RPS) req/sec" -ForegroundColor Green
    Write-Host ""
    Write-Host "  === Latency ===" -ForegroundColor Cyan
    Write-Host "  p50: $($throughput.LatencyP50) ms"
    Write-Host "  p90: $($throughput.LatencyP90) ms"
    Write-Host "  p99: $($throughput.LatencyP99) ms"
    Write-Host ""

    if ($resourceStats) {
        Write-Host "  === Resource Usage ===" -ForegroundColor Cyan
        Write-Host "  Avg CPU: $($resourceStats.AvgCPU)%" -ForegroundColor Yellow
        Write-Host "  Avg Memory: $($resourceStats.AvgMemory) MB" -ForegroundColor Yellow
        Write-Host "  Peak Memory: $($resourceStats.PeakMemory) MB" -ForegroundColor Yellow
        Write-Host "  Samples: $($resourceStats.Samples)"
    }

    return @{
        Name = $Name
        RPS = $throughput.RPS
        Completed = $throughput.Completed
        Errors = $throughput.Errors
        LatencyP50 = $throughput.LatencyP50
        LatencyP90 = $throughput.LatencyP90
        LatencyP99 = $throughput.LatencyP99
        AvgCPU = $resourceStats.AvgCPU
        AvgMemory = $resourceStats.AvgMemory
        PeakMemory = $resourceStats.PeakMemory
    }
}

# Create simple HTTP servers for testing
Write-Host "Creating test servers..." -ForegroundColor Cyan

# Nova server
$novaServer = @"
import { createServer } from "http";

const server = createServer((req, res) => {
  res.writeHead(200, { "Content-Type": "text/plain" });
  res.end("Hello World");
});

server.listen(3000);
server.run(999999999);
"@
$novaServer | Out-File -FilePath "benchmarks/bench_http_nova_simple.ts" -Encoding UTF8

# Node server
$nodeServer = @"
const http = require('http');

const server = http.createServer((req, res) => {
  res.writeHead(200, { 'Content-Type': 'text/plain' });
  res.end('Hello World');
});

server.listen(3000);
"@
$nodeServer | Out-File -FilePath "benchmarks/bench_http_node_simple.js" -Encoding UTF8

# Bun server
$bunServer = @"
const server = Bun.serve({
  port: 3000,
  fetch(req) {
    return new Response("Hello World");
  },
});
"@
$bunServer | Out-File -FilePath "benchmarks/bench_http_bun_simple.ts" -Encoding UTF8

Write-Host "Test servers created!" -ForegroundColor Green
Write-Host ""

# Run benchmarks
$results = @()

# Benchmark Nova
$novaResult = Benchmark-Runtime -Name "Nova" `
    -ServerScript "benchmarks/bench_http_nova_simple.ts" `
    -Command @("./build/Release/nova.exe", "benchmarks/bench_http_nova_simple.ts") `
    -ProcessName "nova" -Port 3000

if ($novaResult) { $results += $novaResult }

# Benchmark Node
$nodeResult = Benchmark-Runtime -Name "Node" `
    -ServerScript "benchmarks/bench_http_node_simple.js" `
    -Command @("node", "benchmarks/bench_http_node_simple.js") `
    -ProcessName "node" -Port 3000

if ($nodeResult) { $results += $nodeResult }

# Benchmark Bun (if available)
try {
    $null = Get-Command bun -ErrorAction Stop
    $bunResult = Benchmark-Runtime -Name "Bun" `
        -ServerScript "benchmarks/bench_http_bun_simple.ts" `
        -Command @("bun", "benchmarks/bench_http_bun_simple.ts") `
        -ProcessName "bun" -Port 3000

    if ($bunResult) { $results += $bunResult }
} catch {
    Write-Host "[Bun] Not available, skipping..." -ForegroundColor Yellow
}

# Generate comparison report
Write-Host ""
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Resource Usage Comparison" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

$table = $results | ForEach-Object {
    [PSCustomObject]@{
        Runtime = $_.Name
        RPS = $_.RPS
        'CPU (%)' = $_.AvgCPU
        'Avg Memory (MB)' = $_.AvgMemory
        'Peak Memory (MB)' = $_.PeakMemory
        'p50 (ms)' = $_.LatencyP50
        'p99 (ms)' = $_.LatencyP99
    }
}

$table | Format-Table -AutoSize

# Calculate efficiency metrics
Write-Host ""
Write-Host "=== Efficiency Metrics ===" -ForegroundColor Cyan
Write-Host ""

foreach ($result in $results) {
    $rpsPerCPU = if ($result.AvgCPU -gt 0) { [math]::Round($result.RPS / $result.AvgCPU, 2) } else { 0 }
    $rpsPerMB = if ($result.AvgMemory -gt 0) { [math]::Round($result.RPS / $result.AvgMemory, 2) } else { 0 }

    Write-Host "$($result.Name):" -ForegroundColor Yellow
    Write-Host "  RPS per CPU%: $rpsPerCPU"
    Write-Host "  RPS per MB: $rpsPerMB"
    Write-Host ""
}

Write-Host "Benchmark completed!" -ForegroundColor Green
Write-Host ""
Write-Host "Output files:"
Write-Host "  bench_Nova_out.txt / bench_Nova_err.txt"
Write-Host "  bench_Node_out.txt / bench_Node_err.txt"
Write-Host "  bench_Bun_out.txt / bench_Bun_err.txt"
