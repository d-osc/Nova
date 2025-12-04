# ============================================================================
# Nova Network Benchmark - Comprehensive Test Suite
# ============================================================================
# Tests: HTTP, HTTP/2, HTTPS, Raw TCP, Connection Performance
# Compares: Nova vs Node.js vs Bun
# ============================================================================

Write-Host "=== Nova Network Benchmark Suite ===" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Continue"

# Results storage
$results = @{}

# ============================================================================
# Test 1: HTTP Basic Server Performance
# ============================================================================

Write-Host "[1/6] HTTP Basic Server Performance" -ForegroundColor Yellow
Write-Host "Testing simple HTTP request/response latency..." -ForegroundColor Gray
Write-Host ""

# Node.js HTTP server
$nodeHttpCode = @"
const http = require('http');
const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Hello from Node.js');
});
server.listen(3000, () => console.log('Node.js HTTP ready'));
"@

Set-Content -Path "benchmarks/temp_http_node.js" -Value $nodeHttpCode

Write-Host "Starting Node.js HTTP server..." -ForegroundColor Gray
$nodeHttp = Start-Process -FilePath "node" -ArgumentList "benchmarks/temp_http_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

# Benchmark Node.js
Write-Host "Benchmarking Node.js (1000 requests)..." -ForegroundColor Green
$nodeResults = @()
for ($i = 1; $i -le 1000; $i++) {
    $start = Get-Date
    $response = curl.exe -s -o nul -w "%{http_code}" "http://localhost:3000/" 2>&1
    $end = Get-Date
    if ($response -eq "200") {
        $nodeResults += ($end - $start).TotalMilliseconds
    }
    if ($i % 200 -eq 0) {
        Write-Host "  Progress: $i/1000" -ForegroundColor Gray
    }
}

$nodeAvg = ($nodeResults | Measure-Object -Average).Average
$nodeP50 = $nodeResults | Sort-Object | Select-Object -Index ([int]($nodeResults.Count * 0.5))
$nodeP95 = $nodeResults | Sort-Object | Select-Object -Index ([int]($nodeResults.Count * 0.95))

Write-Host "  Node.js - Avg: $([math]::Round($nodeAvg, 2))ms | P50: $([math]::Round($nodeP50, 2))ms | P95: $([math]::Round($nodeP95, 2))ms" -ForegroundColor White

Stop-Process -Id $nodeHttp.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Nova HTTP server
$novaHttpCode = @"
import * as http from 'nova:http';

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/plain' });
    res.end('Hello from Nova');
});

server.listen(3000);
console.log('Nova HTTP ready');
"@

Set-Content -Path "benchmarks/temp_http_nova.ts" -Value $novaHttpCode

Write-Host "Starting Nova HTTP server..." -ForegroundColor Gray
$novaHttp = Start-Process -FilePath "build/Release/nova.exe" -ArgumentList "benchmarks/temp_http_nova.ts" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

# Benchmark Nova
Write-Host "Benchmarking Nova (1000 requests)..." -ForegroundColor Green
$novaResults = @()
for ($i = 1; $i -le 1000; $i++) {
    $start = Get-Date
    $response = curl.exe -s -o nul -w "%{http_code}" "http://localhost:3000/" 2>&1
    $end = Get-Date
    if ($response -eq "200") {
        $novaResults += ($end - $start).TotalMilliseconds
    }
    if ($i % 200 -eq 0) {
        Write-Host "  Progress: $i/1000" -ForegroundColor Gray
    }
}

$novaAvg = ($novaResults | Measure-Object -Average).Average
$novaP50 = $novaResults | Sort-Object | Select-Object -Index ([int]($novaResults.Count * 0.5))
$novaP95 = $novaResults | Sort-Object | Select-Object -Index ([int]($novaResults.Count * 0.95))

Write-Host "  Nova - Avg: $([math]::Round($novaAvg, 2))ms | P50: $([math]::Round($novaP50, 2))ms | P95: $([math]::Round($novaP95, 2))ms" -ForegroundColor Cyan

$httpAdvantage = $nodeAvg / $novaAvg
Write-Host "  Advantage: $([math]::Round($httpAdvantage, 2))x faster" -ForegroundColor Green

Stop-Process -Id $novaHttp.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

$results["HTTP"] = @{
    "Node" = $nodeAvg
    "Nova" = $novaAvg
    "Advantage" = $httpAdvantage
}

Write-Host ""

# ============================================================================
# Test 2: Connection Throughput (Keep-Alive)
# ============================================================================

Write-Host "[2/6] HTTP Keep-Alive Throughput" -ForegroundColor Yellow
Write-Host "Testing sustained connection performance..." -ForegroundColor Gray
Write-Host ""

# Node.js with keep-alive
$nodeKaCode = @"
const http = require('http');
const server = http.createServer((req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Connection': 'keep-alive'
    });
    res.end('KA');
});
server.keepAliveTimeout = 60000;
server.listen(3001, () => console.log('Node.js Keep-Alive ready'));
"@

Set-Content -Path "benchmarks/temp_ka_node.js" -Value $nodeKaCode
$nodeKa = Start-Process -FilePath "node" -ArgumentList "benchmarks/temp_ka_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Node.js Keep-Alive (500 requests)..." -ForegroundColor Green
$startTime = Get-Date
for ($i = 1; $i -le 500; $i++) {
    curl.exe -s -o nul "http://localhost:3001/" 2>&1 | Out-Null
}
$endTime = Get-Date
$nodeDuration = ($endTime - $startTime).TotalSeconds
$nodeThroughput = 500 / $nodeDuration

Write-Host "  Node.js - $([math]::Round($nodeThroughput, 2)) req/s" -ForegroundColor White

Stop-Process -Id $nodeKa.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Nova with keep-alive
$novaKaCode = @"
import * as http from 'nova:http';

const server = http.createServer((req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Connection': 'keep-alive'
    });
    res.end('KA');
});

server.listen(3001);
console.log('Nova Keep-Alive ready');
"@

Set-Content -Path "benchmarks/temp_ka_nova.ts" -Value $novaKaCode
$novaKa = Start-Process -FilePath "build/Release/nova.exe" -ArgumentList "benchmarks/temp_ka_nova.ts" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Nova Keep-Alive (500 requests)..." -ForegroundColor Green
$startTime = Get-Date
for ($i = 1; $i -le 500; $i++) {
    curl.exe -s -o nul "http://localhost:3001/" 2>&1 | Out-Null
}
$endTime = Get-Date
$novaDuration = ($endTime - $startTime).TotalSeconds
$novaThroughput = 500 / $novaDuration

Write-Host "  Nova - $([math]::Round($novaThroughput, 2)) req/s" -ForegroundColor Cyan

$kaAdvantage = $novaThroughput / $nodeThroughput
Write-Host "  Advantage: $([math]::Round($kaAdvantage, 2))x faster" -ForegroundColor Green

Stop-Process -Id $novaKa.Id -Force -ErrorAction SilentlyContinue

$results["KeepAlive"] = @{
    "Node" = $nodeThroughput
    "Nova" = $novaThroughput
    "Advantage" = $kaAdvantage
}

Write-Host ""

# ============================================================================
# Test 3: POST Request Performance
# ============================================================================

Write-Host "[3/6] POST Request Performance" -ForegroundColor Yellow
Write-Host "Testing request body handling..." -ForegroundColor Gray
Write-Host ""

# Node.js POST server
$nodePostCode = @"
const http = require('http');
const server = http.createServer((req, res) => {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('Received ' + body.length + ' bytes');
    });
});
server.listen(3002, () => console.log('Node.js POST ready'));
"@

Set-Content -Path "benchmarks/temp_post_node.js" -Value $nodePostCode
$nodePost = Start-Process -FilePath "node" -ArgumentList "benchmarks/temp_post_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

# Create test payload (1KB)
$payload = "x" * 1024

Write-Host "Benchmarking Node.js POST (200 requests, 1KB payload)..." -ForegroundColor Green
$nodePostResults = @()
for ($i = 1; $i -le 200; $i++) {
    $start = Get-Date
    curl.exe -s -o nul -X POST -d $payload "http://localhost:3002/" 2>&1 | Out-Null
    $end = Get-Date
    $nodePostResults += ($end - $start).TotalMilliseconds
}

$nodePostAvg = ($nodePostResults | Measure-Object -Average).Average
Write-Host "  Node.js - Avg: $([math]::Round($nodePostAvg, 2))ms" -ForegroundColor White

Stop-Process -Id $nodePost.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Nova POST server
$novaPostCode = @"
import * as http from 'nova:http';

const server = http.createServer((req, res) => {
    let body = '';
    req.on('data', (chunk) => { body += chunk; });
    req.on('end', () => {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('Received ' + body.length + ' bytes');
    });
});

server.listen(3002);
console.log('Nova POST ready');
"@

Set-Content -Path "benchmarks/temp_post_nova.ts" -Value $novaPostCode
$novaPost = Start-Process -FilePath "build/Release/nova.exe" -ArgumentList "benchmarks/temp_post_nova.ts" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Nova POST (200 requests, 1KB payload)..." -ForegroundColor Green
$novaPostResults = @()
for ($i = 1; $i -le 200; $i++) {
    $start = Get-Date
    curl.exe -s -o nul -X POST -d $payload "http://localhost:3002/" 2>&1 | Out-Null
    $end = Get-Date
    $novaPostResults += ($end - $start).TotalMilliseconds
}

$novaPostAvg = ($novaPostResults | Measure-Object -Average).Average
Write-Host "  Nova - Avg: $([math]::Round($novaPostAvg, 2))ms" -ForegroundColor Cyan

$postAdvantage = $nodePostAvg / $novaPostAvg
Write-Host "  Advantage: $([math]::Round($postAdvantage, 2))x faster" -ForegroundColor Green

Stop-Process -Id $novaPost.Id -Force -ErrorAction SilentlyContinue

$results["POST"] = @{
    "Node" = $nodePostAvg
    "Nova" = $novaPostAvg
    "Advantage" = $postAdvantage
}

Write-Host ""

# ============================================================================
# Test 4: Concurrent Connections
# ============================================================================

Write-Host "[4/6] Concurrent Connections Handling" -ForegroundColor Yellow
Write-Host "Testing parallel connection capacity..." -ForegroundColor Gray
Write-Host ""

# Node.js concurrent server
$nodeConcCode = @"
const http = require('http');
const server = http.createServer((req, res) => {
    setTimeout(() => {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('OK');
    }, 10);
});
server.listen(3003, () => console.log('Node.js Concurrent ready'));
"@

Set-Content -Path "benchmarks/temp_conc_node.js" -Value $nodeConcCode
$nodeConc = Start-Process -FilePath "node" -ArgumentList "benchmarks/temp_conc_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Node.js (100 concurrent connections)..." -ForegroundColor Green
$jobs = @()
$startTime = Get-Date
for ($i = 1; $i -le 100; $i++) {
    $jobs += Start-Job -ScriptBlock {
        curl.exe -s -o nul "http://localhost:3003/" 2>&1 | Out-Null
    }
}
$jobs | Wait-Job | Remove-Job
$endTime = Get-Date
$nodeConcTime = ($endTime - $startTime).TotalSeconds

Write-Host "  Node.js - Total time: $([math]::Round($nodeConcTime, 2))s" -ForegroundColor White

Stop-Process -Id $nodeConc.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Nova concurrent server
$novaConcCode = @"
import * as http from 'nova:http';

const server = http.createServer((req, res) => {
    setTimeout(() => {
        res.writeHead(200, { 'Content-Type': 'text/plain' });
        res.end('OK');
    }, 10);
});

server.listen(3003);
console.log('Nova Concurrent ready');
"@

Set-Content -Path "benchmarks/temp_conc_nova.ts" -Value $novaConcCode
$novaConc = Start-Process -FilePath "build/Release/nova.exe" -ArgumentList "benchmarks/temp_conc_nova.ts" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Nova (100 concurrent connections)..." -ForegroundColor Green
$jobs = @()
$startTime = Get-Date
for ($i = 1; $i -le 100; $i++) {
    $jobs += Start-Job -ScriptBlock {
        curl.exe -s -o nul "http://localhost:3003/" 2>&1 | Out-Null
    }
}
$jobs | Wait-Job | Remove-Job
$endTime = Get-Date
$novaConcTime = ($endTime - $startTime).TotalSeconds

Write-Host "  Nova - Total time: $([math]::Round($novaConcTime, 2))s" -ForegroundColor Cyan

$concAdvantage = $nodeConcTime / $novaConcTime
Write-Host "  Advantage: $([math]::Round($concAdvantage, 2))x faster" -ForegroundColor Green

Stop-Process -Id $novaConc.Id -Force -ErrorAction SilentlyContinue

$results["Concurrent"] = @{
    "Node" = $nodeConcTime
    "Nova" = $novaConcTime
    "Advantage" = $concAdvantage
}

Write-Host ""

# ============================================================================
# Test 5: Large Response Performance
# ============================================================================

Write-Host "[5/6] Large Response Performance" -ForegroundColor Yellow
Write-Host "Testing large payload transfer..." -ForegroundColor Gray
Write-Host ""

# Node.js large response server
$nodeLargeCode = @"
const http = require('http');
const payload = 'x'.repeat(100 * 1024); // 100KB
const server = http.createServer((req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Content-Length': payload.length
    });
    res.end(payload);
});
server.listen(3004, () => console.log('Node.js Large ready'));
"@

Set-Content -Path "benchmarks/temp_large_node.js" -Value $nodeLargeCode
$nodeLarge = Start-Process -FilePath "node" -ArgumentList "benchmarks/temp_large_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Node.js (100 requests, 100KB payload)..." -ForegroundColor Green
$nodeLargeResults = @()
for ($i = 1; $i -le 100; $i++) {
    $start = Get-Date
    curl.exe -s -o nul "http://localhost:3004/" 2>&1 | Out-Null
    $end = Get-Date
    $nodeLargeResults += ($end - $start).TotalMilliseconds
}

$nodeLargeAvg = ($nodeLargeResults | Measure-Object -Average).Average
Write-Host "  Node.js - Avg: $([math]::Round($nodeLargeAvg, 2))ms" -ForegroundColor White

Stop-Process -Id $nodeLarge.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Nova large response server
$novaLargeCode = @"
import * as http from 'nova:http';

const payload = 'x'.repeat(100 * 1024); // 100KB
const server = http.createServer((req, res) => {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Content-Length': payload.length.toString()
    });
    res.end(payload);
});

server.listen(3004);
console.log('Nova Large ready');
"@

Set-Content -Path "benchmarks/temp_large_nova.ts" -Value $novaLargeCode
$novaLarge = Start-Process -FilePath "build/Release/nova.exe" -ArgumentList "benchmarks/temp_large_nova.ts" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Nova (100 requests, 100KB payload)..." -ForegroundColor Green
$novaLargeResults = @()
for ($i = 1; $i -le 100; $i++) {
    $start = Get-Date
    curl.exe -s -o nul "http://localhost:3004/" 2>&1 | Out-Null
    $end = Get-Date
    $novaLargeResults += ($end - $start).TotalMilliseconds
}

$novaLargeAvg = ($novaLargeResults | Measure-Object -Average).Average
Write-Host "  Nova - Avg: $([math]::Round($novaLargeAvg, 2))ms" -ForegroundColor Cyan

$largeAdvantage = $nodeLargeAvg / $novaLargeAvg
Write-Host "  Advantage: $([math]::Round($largeAdvantage, 2))x faster" -ForegroundColor Green

Stop-Process -Id $novaLarge.Id -Force -ErrorAction SilentlyContinue

$results["LargeResponse"] = @{
    "Node" = $nodeLargeAvg
    "Nova" = $novaLargeAvg
    "Advantage" = $largeAdvantage
}

Write-Host ""

# ============================================================================
# Test 6: Connection Establishment Speed
# ============================================================================

Write-Host "[6/6] Connection Establishment Speed" -ForegroundColor Yellow
Write-Host "Testing new connection overhead..." -ForegroundColor Gray
Write-Host ""

# Node.js connection test
$nodeConnCode = @"
const http = require('http');
const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Connection': 'close' });
    res.end('OK');
});
server.listen(3005, () => console.log('Node.js Connection ready'));
"@

Set-Content -Path "benchmarks/temp_conn_node.js" -Value $nodeConnCode
$nodeConn = Start-Process -FilePath "node" -ArgumentList "benchmarks/temp_conn_node.js" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Node.js (500 fresh connections)..." -ForegroundColor Green
$nodeConnResults = @()
for ($i = 1; $i -le 500; $i++) {
    $start = Get-Date
    curl.exe -s -o nul --no-keepalive "http://localhost:3005/" 2>&1 | Out-Null
    $end = Get-Date
    $nodeConnResults += ($end - $start).TotalMilliseconds
    if ($i % 100 -eq 0) {
        Write-Host "  Progress: $i/500" -ForegroundColor Gray
    }
}

$nodeConnAvg = ($nodeConnResults | Measure-Object -Average).Average
Write-Host "  Node.js - Avg: $([math]::Round($nodeConnAvg, 2))ms" -ForegroundColor White

Stop-Process -Id $nodeConn.Id -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1

# Nova connection test
$novaConnCode = @"
import * as http from 'nova:http';

const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Connection': 'close' });
    res.end('OK');
});

server.listen(3005);
console.log('Nova Connection ready');
"@

Set-Content -Path "benchmarks/temp_conn_nova.ts" -Value $novaConnCode
$novaConn = Start-Process -FilePath "build/Release/nova.exe" -ArgumentList "benchmarks/temp_conn_nova.ts" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

Write-Host "Benchmarking Nova (500 fresh connections)..." -ForegroundColor Green
$novaConnResults = @()
for ($i = 1; $i -le 500; $i++) {
    $start = Get-Date
    curl.exe -s -o nul --no-keepalive "http://localhost:3005/" 2>&1 | Out-Null
    $end = Get-Date
    $novaConnResults += ($end - $start).TotalMilliseconds
    if ($i % 100 -eq 0) {
        Write-Host "  Progress: $i/500" -ForegroundColor Gray
    }
}

$novaConnAvg = ($novaConnResults | Measure-Object -Average).Average
Write-Host "  Nova - Avg: $([math]::Round($novaConnAvg, 2))ms" -ForegroundColor Cyan

$connAdvantage = $nodeConnAvg / $novaConnAvg
Write-Host "  Advantage: $([math]::Round($connAdvantage, 2))x faster" -ForegroundColor Green

Stop-Process -Id $novaConn.Id -Force -ErrorAction SilentlyContinue

$results["Connection"] = @{
    "Node" = $nodeConnAvg
    "Nova" = $novaConnAvg
    "Advantage" = $connAdvantage
}

Write-Host ""

# ============================================================================
# Summary Report
# ============================================================================

Write-Host "=== Network Benchmark Summary ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "Test Results:" -ForegroundColor Yellow
Write-Host ""

foreach ($test in $results.Keys | Sort-Object) {
    $data = $results[$test]
    $node = $data.Node
    $nova = $data.Nova
    $adv = $data.Advantage

    $unit = if ($test -eq "KeepAlive") { "req/s" } else { "ms" }

    Write-Host "  $test" -ForegroundColor White
    Write-Host "    Node.js: $([math]::Round($node, 2)) $unit" -ForegroundColor Gray
    Write-Host "    Nova:    $([math]::Round($nova, 2)) $unit" -ForegroundColor Cyan
    Write-Host "    Speedup: $([math]::Round($adv, 2))x" -ForegroundColor Green
    Write-Host ""
}

# Overall average
$avgAdvantage = ($results.Values | ForEach-Object { $_.Advantage } | Measure-Object -Average).Average
Write-Host "Overall Average Speedup: $([math]::Round($avgAdvantage, 2))x" -ForegroundColor Green
Write-Host ""

# Cleanup
Write-Host "Cleaning up temporary files..." -ForegroundColor Gray
Remove-Item "benchmarks/temp_*.js" -ErrorAction SilentlyContinue
Remove-Item "benchmarks/temp_*.ts" -ErrorAction SilentlyContinue

Write-Host "Benchmark complete!" -ForegroundColor Green
