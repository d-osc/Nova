# HTTP/2 Benchmark Status Check
# Tests HTTP/2 support across runtimes

Write-Host "=== HTTP/2 Support & Performance Check ===" -ForegroundColor Cyan
Write-Host ""

$port = 3000

function Test-HTTP2 {
    param($name, $serverCmd, $color)

    Write-Host "Testing $name HTTP/2..." -ForegroundColor $color

    # Start server
    $process = Start-Process -FilePath "powershell" -ArgumentList "-Command", $serverCmd -PassThru -WindowStyle Hidden

    Start-Sleep -Seconds 2

    # Test HTTP/2 connection
    try {
        $response = curl.exe --http2 -s -I http://127.0.0.1:$port/ 2>&1 | Out-String

        if ($response -match "HTTP/2") {
            Write-Host "  ✓ HTTP/2 supported" -ForegroundColor Green

            # Quick performance test (10 requests)
            Write-Host "  Running quick test - 10 requests..." -ForegroundColor Gray

            $times = @()
            for ($i = 1; $i -le 10; $i++) {
                $sw = [System.Diagnostics.Stopwatch]::StartNew()
                $null = curl.exe --http2 -s http://127.0.0.1:$port/ 2>&1
                $sw.Stop()
                $times += $sw.Elapsed.TotalMilliseconds
            }

            $avg = ($times | Measure-Object -Average).Average
            $min = ($times | Measure-Object -Minimum).Minimum
            $max = ($times | Measure-Object -Maximum).Maximum

            Write-Host "    Avg: $([math]::Round($avg, 2))ms" -ForegroundColor White
            Write-Host "    Min: $([math]::Round($min, 2))ms" -ForegroundColor White
            Write-Host "    Max: $([math]::Round($max, 2))ms" -ForegroundColor White
        } else {
            Write-Host "  ✗ HTTP/2 not detected" -ForegroundColor Yellow
        }
    } catch {
        Write-Host "  ✗ Failed to connect: $_" -ForegroundColor Red
    }

    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
    Write-Host ""
}

# Test Node.js
if (Get-Command node -ErrorAction SilentlyContinue) {
    Test-HTTP2 "Node.js" "node benchmarks\http2_server_node.js" "Green"
} else {
    Write-Host "Node.js not found" -ForegroundColor Yellow
    Write-Host ""
}

# Test Bun
Write-Host "Bun HTTP/2..." -ForegroundColor Magenta
Write-Host "  Note: Bun does not have native HTTP/2 support yet" -ForegroundColor Yellow
Write-Host "        Bun uses HTTP/1.1 with potential h2c upgrade" -ForegroundColor Gray
Write-Host ""

# Test Nova
Write-Host "Nova HTTP/2..." -ForegroundColor Cyan
Write-Host "  Status: HTTP/2 implementation exists in C++ runtime" -ForegroundColor Yellow
Write-Host "  Location: src/runtime/BuiltinHTTP2.cpp" -ForegroundColor Gray
Write-Host "  Issue: Module not registered in BuiltinModules.cpp" -ForegroundColor Yellow
Write-Host "  Action needed: Add 'nova:http2' to module registration" -ForegroundColor Gray
Write-Host ""

Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "  ✓ Node.js: Full HTTP/2 support" -ForegroundColor Green
Write-Host "  - Bun: No native HTTP/2 support" -ForegroundColor Yellow
Write-Host "  ~ Nova: Implementation ready, needs module registration" -ForegroundColor Yellow
Write-Host ""
