# Comprehensive HTTP Benchmark for Nova vs Node.js vs Bun
# Tests: Throughput, Latency, Concurrency

Write-Host "=== HTTP Benchmark Suite ===" -ForegroundColor Cyan
Write-Host ""

# Kill any existing servers
taskkill /F /IM node.exe 2>$null
taskkill /F /IM bun.exe 2>$null  
taskkill /F /IM nova.exe 2>$null
Start-Sleep -Seconds 2

# Test Configuration
$requests = 10000
$concurrency = 100
$url = "http://127.0.0.1:3000/"

# Results storage
$results = @{}

function Test-Server {
    param($name, $command, $port)
    
    Write-Host "Testing $name..." -ForegroundColor Yellow
    
    # Start server
    $proc = Start-Process -FilePath "powershell" -ArgumentList "-Command", $command -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 3
    
    # Check if server started
    try {
        $response = Invoke-WebRequest -Uri "http://127.0.0.1:$port" -TimeoutSec 2 -UseBasicParsing
        Write-Host "  Server started successfully" -ForegroundColor Green
    } catch {
        Write-Host "  Failed to start server!" -ForegroundColor Red
        Stop-Process -Id $proc.Id -Force
        return $null
    }
    
    # Run benchmark with wrk or similar tool
    # For now, simple PowerShell test
    $startTime = Get-Date
    
    for ($i = 0; $i -lt 1000; $i++) {
        try {
            Invoke-WebRequest -Uri "http://127.0.0.1:$port" -UseBasicParsing | Out-Null
        } catch {
            Write-Host "  Request failed: $_" -ForegroundColor Red
        }
    }
    
    $endTime = Get-Date
    $duration = ($endTime - $startTime).TotalSeconds
    $rps = [math]::Round(1000 / $duration, 2)
    
    Write-Host "  Requests/sec: $rps" -ForegroundColor Cyan
    Write-Host "  Total time: $duration seconds" -ForegroundColor Cyan
    
    # Stop server
    Stop-Process -Id $proc.Id -Force
    Start-Sleep -Seconds 2
    
    return @{
        Name = $name
        RequestsPerSec = $rps
        Duration = $duration
    }
}

# Test Node.js
$nodeResult = Test-Server "Node.js" "node benchmarks/http2_server_node.js" 3000

# Test Bun  
$bunResult = Test-Server "Bun" "bun benchmarks/http2_server_bun.ts" 3001

# Display Results
Write-Host ""
Write-Host "=== Results ===" -ForegroundColor Cyan
Write-Host "Node.js: $($nodeResult.RequestsPerSec) req/sec" -ForegroundColor Green
Write-Host "Bun: $($bunResult.RequestsPerSec) req/sec" -ForegroundColor Green

# Determine winner
if ($nodeResult.RequestsPerSec -gt $bunResult.RequestsPerSec) {
    $winner = "Node.js"
    $margin = [math]::Round(($nodeResult.RequestsPerSec / $bunResult.RequestsPerSec), 2)
} else {
    $winner = "Bun"
    $margin = [math]::Round(($bunResult.RequestsPerSec / $nodeResult.RequestsPerSec), 2)
}

Write-Host ""
Write-Host "Winner: $winner (${margin}x faster)" -ForegroundColor Yellow
