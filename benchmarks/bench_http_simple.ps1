# Simple HTTP Benchmark - Nova vs Node vs Bun
# Tests basic HTTP server performance

param(
    [int]$Requests = 100,
    [int]$Port = 3000
)

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Simple HTTP Benchmark" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Requests: $Requests per runtime"
Write-Host "Port: $Port"
Write-Host ""

# Function to test a server
function Test-Server {
    param(
        $Name,
        $Command,
        $Requests
    )

    Write-Host "Testing $Name..." -ForegroundColor Yellow

    # Start server in background
    $serverProcess = Start-Process -FilePath "powershell" -ArgumentList "-Command", $Command -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2  # Wait for server to start

    if ($serverProcess.HasExited) {
        Write-Host "  ERROR: Server failed to start" -ForegroundColor Red
        return $null
    }

    Write-Host "  Server started (PID: $($serverProcess.Id))"

    # Measure throughput
    $startTime = Get-Date
    $success = 0
    $failed = 0

    for ($i = 0; $i -lt $Requests; $i++) {
        try {
            $response = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/" -TimeoutSec 5 -UseBasicParsing -ErrorAction Stop
            if ($response.StatusCode -eq 200) {
                $success++
            } else {
                $failed++
            }
        } catch {
            $failed++
        }

        # Progress indicator
        if (($i + 1) % 10 -eq 0) {
            Write-Host "  Progress: $($i + 1)/$Requests requests sent" -NoNewline
            Write-Host "`r" -NoNewline
        }
    }

    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds

    # Get final memory usage
    $process = Get-Process -Id $serverProcess.Id -ErrorAction SilentlyContinue
    $memoryMB = 0
    if ($process) {
        $memoryMB = [math]::Round($process.WorkingSet64 / 1MB, 2)
    }

    # Stop server
    Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500

    # Calculate metrics
    $rps = [math]::Round($success / $duration, 2)
    $avgLatency = [math]::Round(($duration / $success) * 1000, 2)

    Write-Host "  Completed: $success successful, $failed failed                   "
    Write-Host "  Duration: $([math]::Round($duration, 2)) seconds"
    Write-Host "  RPS: $rps requests/sec" -ForegroundColor Green
    Write-Host "  Avg Latency: $avgLatency ms"
    Write-Host "  Memory: $memoryMB MB"
    Write-Host ""

    return @{
        Name = $Name
        Success = $success
        Failed = $failed
        Duration = $duration
        RPS = $rps
        AvgLatency = $avgLatency
        Memory = $memoryMB
    }
}

# Test each runtime
$results = @()

# Nova
Write-Host "=== NOVA ===" -ForegroundColor Cyan
$novaCmd = "& 'C:\Users\ondev\Projects\Nova\build\Release\nova.exe' run 'C:\Users\ondev\Projects\Nova\benchmarks\http_hello_nova.ts'"
$novaResult = Test-Server -Name "Nova" -Command $novaCmd -Requests $Requests
if ($novaResult) { $results += $novaResult }

Start-Sleep -Seconds 2

# Node.js
Write-Host "=== NODE.JS ===" -ForegroundColor Cyan
$nodeCmd = "node 'C:\Users\ondev\Projects\Nova\benchmarks\http_hello_node.js'"
$nodeResult = Test-Server -Name "Node" -Command $nodeCmd -Requests $Requests
if ($nodeResult) { $results += $nodeResult }

Start-Sleep -Seconds 2

# Bun
Write-Host "=== BUN ===" -ForegroundColor Cyan
$bunCmd = "bun 'C:\Users\ondev\Projects\Nova\benchmarks\http_hello_bun.ts'"
$bunResult = Test-Server -Name "Bun" -Command $bunCmd -Requests $Requests
if ($bunResult) { $results += $bunResult }

# Display results
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "RESULTS SUMMARY" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Runtime | RPS | Avg Latency | Memory" -ForegroundColor White
Write-Host "------- | --- | ----------- | ------" -ForegroundColor White

foreach ($result in $results) {
    Write-Host "$($result.Name.PadRight(7)) | $($result.RPS.ToString().PadLeft(7)) | $($result.AvgLatency.ToString().PadLeft(9)) ms | $($result.Memory.ToString().PadLeft(6)) MB"
}

Write-Host ""

# Find winner
$winner = $results | Sort-Object -Property RPS -Descending | Select-Object -First 1
if ($winner) {
    Write-Host "Winner: $($winner.Name) with $($winner.RPS) RPS!" -ForegroundColor Green

    # Calculate speedups
    foreach ($result in $results) {
        if ($result.Name -ne $winner.Name) {
            $speedup = [math]::Round($winner.RPS / $result.RPS, 2)
            Write-Host "  - $($speedup)x faster than $($result.Name)" -ForegroundColor Yellow
        }
    }
}

Write-Host ""
Write-Host "Benchmark completed!" -ForegroundColor Green
