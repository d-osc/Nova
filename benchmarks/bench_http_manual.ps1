# Simple HTTP Benchmark for Nova vs Node.js vs Bun
Write-Host "=== HTTP Network Module Benchmark ===" -ForegroundColor Cyan
Write-Host ""

function Test-Server {
    param(
        [string]$Name,
        [string]$Command,
        [string]$Args,
        [int]$Port,
        [int]$Requests = 500
    )

    Write-Host "Testing $Name..." -ForegroundColor Yellow

    # Start server
    if ($Args) {
        $proc = Start-Process -FilePath $Command -ArgumentList $Args -PassThru -WindowStyle Hidden
    } else {
        $proc = Start-Process -FilePath $Command -PassThru -WindowStyle Hidden
    }

    Start-Sleep -Seconds 2

    # Test if server is responding
    try {
        $test = curl.exe -s "http://localhost:$Port/" 2>&1
        if ($test -match "Hello World") {
            Write-Host "  Server started successfully" -ForegroundColor Green
        } else {
            Write-Host "  Server not responding correctly" -ForegroundColor Red
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
            return $null
        }
    } catch {
        Write-Host "  Failed to connect to server" -ForegroundColor Red
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        return $null
    }

    # Benchmark
    Write-Host "  Running $Requests requests..." -ForegroundColor Gray
    $times = @()
    $successful = 0
    $failed = 0

    for ($i = 0; $i -lt $Requests; $i++) {
        $start = Get-Date
        try {
            $response = curl.exe -s -o nul -w "%{http_code}" "http://localhost:$Port/" 2>&1
            $end = Get-Date
            if ($response -eq "200") {
                $times += ($end - $start).TotalMilliseconds
                $successful++
            } else {
                $failed++
            }
        } catch {
            $failed++
        }

        if (($i + 1) % 100 -eq 0) {
            Write-Host "    Progress: $($i + 1)/$Requests" -ForegroundColor Gray
        }
    }

    # Stop server
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

    # Calculate stats
    if ($times.Count -gt 0) {
        $avg = ($times | Measure-Object -Average).Average
        $min = ($times | Measure-Object -Minimum).Minimum
        $max = ($times | Measure-Object -Maximum).Maximum
        $sorted = $times | Sort-Object
        $p50 = $sorted[[int]($sorted.Count * 0.5)]
        $p95 = $sorted[[int]($sorted.Count * 0.95)]
        $p99 = $sorted[[int]($sorted.Count * 0.99)]

        Write-Host ""
        Write-Host "  Results:" -ForegroundColor Cyan
        Write-Host "    Successful: $successful / $Requests" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Yellow" })
        Write-Host "    Failed: $failed" -ForegroundColor $(if ($failed -gt 0) { "Red" } else { "Green" })
        Write-Host "    Average: $([math]::Round($avg, 2))ms"
        Write-Host "    Min: $([math]::Round($min, 2))ms"
        Write-Host "    Max: $([math]::Round($max, 2))ms"
        Write-Host "    P50: $([math]::Round($p50, 2))ms"
        Write-Host "    P95: $([math]::Round($p95, 2))ms"
        Write-Host "    P99: $([math]::Round($p99, 2))ms"
        Write-Host ""

        return @{
            Name = $Name
            Successful = $successful
            Failed = $failed
            Avg = $avg
            Min = $min
            Max = $max
            P50 = $p50
            P95 = $p95
            P99 = $p99
        }
    } else {
        Write-Host "  No successful requests" -ForegroundColor Red
        return $null
    }
}

# Test Nova
$novaResult = Test-Server -Name "Nova" -Command "build\Release\nova.exe" -Args "benchmarks\test_http_simple_nova.ts" -Port 3000

Start-Sleep -Seconds 1

# Test Node.js
$nodeResult = Test-Server -Name "Node.js" -Command "node" -Args "benchmarks\test_http_simple_node.js" -Port 3001

Start-Sleep -Seconds 1

# Test Bun
$bunResult = Test-Server -Name "Bun" -Command "bun" -Args "benchmarks\test_http_simple_bun.ts" -Port 3002

# Summary
Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host ""

if ($novaResult -and $nodeResult -and $bunResult) {
    Write-Host "Average Latency:" -ForegroundColor Yellow
    Write-Host "  Nova:     $([math]::Round($novaResult.Avg, 2))ms"
    Write-Host "  Node.js:  $([math]::Round($nodeResult.Avg, 2))ms"
    Write-Host "  Bun:      $([math]::Round($bunResult.Avg, 2))ms"
    Write-Host ""

    Write-Host "P95 Latency:" -ForegroundColor Yellow
    Write-Host "  Nova:     $([math]::Round($novaResult.P95, 2))ms"
    Write-Host "  Node.js:  $([math]::Round($nodeResult.P95, 2))ms"
    Write-Host "  Bun:      $([math]::Round($bunResult.P95, 2))ms"
    Write-Host ""

    # Calculate relative performance
    if ($novaResult.Avg -lt $nodeResult.Avg) {
        $speedup = $nodeResult.Avg / $novaResult.Avg
        Write-Host "Nova is $([math]::Round($speedup, 2))x faster than Node.js" -ForegroundColor Green
    } else {
        $speedup = $novaResult.Avg / $nodeResult.Avg
        Write-Host "Node.js is $([math]::Round($speedup, 2))x faster than Nova" -ForegroundColor Yellow
    }

    if ($novaResult.Avg -lt $bunResult.Avg) {
        $speedup = $bunResult.Avg / $novaResult.Avg
        Write-Host "Nova is $([math]::Round($speedup, 2))x faster than Bun" -ForegroundColor Green
    } else {
        $speedup = $novaResult.Avg / $bunResult.Avg
        Write-Host "Bun is $([math]::Round($speedup, 2))x faster than Nova" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Benchmark complete!" -ForegroundColor Green
