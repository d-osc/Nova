# Stream Module Benchmark Script
# Compares Nova, Node.js, and Bun stream performance

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Stream Module Benchmark" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultFile = "stream_bench_results_$timestamp.txt"

# Function to parse benchmark output
function Parse-StreamBenchmark {
    param([string[]]$Output)

    $results = @{
        ReadThroughput = 0.0
        WriteThroughput = 0.0
        TransformThroughput = 0.0
        PipeThroughput = 0.0
    }

    foreach ($line in $Output) {
        if ($line -match 'Read.*Throughput:\s+(\d+\.?\d*)\s+MB/s') {
            $results.ReadThroughput = [double]$matches[1]
        }
        elseif ($line -match 'Wrote.*Throughput:\s+(\d+\.?\d*)\s+MB/s') {
            $results.WriteThroughput = [double]$matches[1]
        }
        elseif ($line -match 'Transformed.*Throughput:\s+(\d+\.?\d*)\s+MB/s') {
            $results.TransformThroughput = [double]$matches[1]
        }
        elseif ($line -match 'Piped.*Throughput:\s+(\d+\.?\d*)\s+MB/s') {
            $results.PipeThroughput = [double]$matches[1]
        }
    }

    return $results
}

# Test Node.js
Write-Host "[1/3] Testing Node.js..." -ForegroundColor Yellow
$nodeOutput = node benchmarks/stream_bench_node.js 2>&1
$nodeResults = Parse-StreamBenchmark -Output $nodeOutput
Write-Host "  Read:      $($nodeResults.ReadThroughput) MB/s" -ForegroundColor White
Write-Host "  Write:     $($nodeResults.WriteThroughput) MB/s" -ForegroundColor White
Write-Host "  Transform: $($nodeResults.TransformThroughput) MB/s" -ForegroundColor White
Write-Host "  Pipe:      $($nodeResults.PipeThroughput) MB/s" -ForegroundColor White
Write-Host ""

# Test Bun
Write-Host "[2/3] Testing Bun..." -ForegroundColor Yellow
$bunOutput = bun benchmarks/stream_bench_bun.ts 2>&1
$bunResults = Parse-StreamBenchmark -Output $bunOutput
Write-Host "  Read:      $($bunResults.ReadThroughput) MB/s" -ForegroundColor White
Write-Host "  Write:     $($bunResults.WriteThroughput) MB/s" -ForegroundColor White
Write-Host "  Transform: $($bunResults.TransformThroughput) MB/s" -ForegroundColor White
Write-Host "  Pipe:      $($bunResults.PipeThroughput) MB/s" -ForegroundColor White
Write-Host ""

# Test Nova
Write-Host "[3/3] Testing Nova..." -ForegroundColor Yellow
$novaOutput = build/Release/nova.exe benchmarks/stream_bench_nova.ts 2>&1
$novaResults = Parse-StreamBenchmark -Output $novaOutput
Write-Host "  Read:      $($novaResults.ReadThroughput) MB/s" -ForegroundColor Cyan
Write-Host "  Write:     $($novaResults.WriteThroughput) MB/s" -ForegroundColor Cyan
Write-Host "  Transform: $($novaResults.TransformThroughput) MB/s" -ForegroundColor Cyan
Write-Host "  Pipe:      $($novaResults.PipeThroughput) MB/s" -ForegroundColor Cyan
Write-Host ""

# Summary
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Benchmark Summary" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Readable Stream
Write-Host "Readable Stream Throughput:" -ForegroundColor Yellow
Write-Host "  Nova:    $($novaResults.ReadThroughput) MB/s"
Write-Host "  Node.js: $($nodeResults.ReadThroughput) MB/s"
Write-Host "  Bun:     $($bunResults.ReadThroughput) MB/s"
if ($novaResults.ReadThroughput -gt 0 -and $nodeResults.ReadThroughput -gt 0) {
    $speedup = $novaResults.ReadThroughput / $nodeResults.ReadThroughput
    if ($speedup -gt 1) {
        Write-Host "  Nova is $([math]::Round($speedup, 2))x faster than Node.js" -ForegroundColor Green
    } else {
        Write-Host "  Node.js is $([math]::Round(1/$speedup, 2))x faster than Nova" -ForegroundColor Yellow
    }
}
Write-Host ""

# Writable Stream
Write-Host "Writable Stream Throughput:" -ForegroundColor Yellow
Write-Host "  Nova:    $($novaResults.WriteThroughput) MB/s"
Write-Host "  Node.js: $($nodeResults.WriteThroughput) MB/s"
Write-Host "  Bun:     $($bunResults.WriteThroughput) MB/s"
if ($novaResults.WriteThroughput -gt 0 -and $nodeResults.WriteThroughput -gt 0) {
    $speedup = $novaResults.WriteThroughput / $nodeResults.WriteThroughput
    if ($speedup -gt 1) {
        Write-Host "  Nova is $([math]::Round($speedup, 2))x faster than Node.js" -ForegroundColor Green
    } else {
        Write-Host "  Node.js is $([math]::Round(1/$speedup, 2))x faster than Nova" -ForegroundColor Yellow
    }
}
Write-Host ""

# Transform Stream
Write-Host "Transform Stream Throughput:" -ForegroundColor Yellow
Write-Host "  Nova:    $($novaResults.TransformThroughput) MB/s"
Write-Host "  Node.js: $($nodeResults.TransformThroughput) MB/s"
Write-Host "  Bun:     $($bunResults.TransformThroughput) MB/s"
if ($novaResults.TransformThroughput -gt 0 -and $nodeResults.TransformThroughput -gt 0) {
    $speedup = $novaResults.TransformThroughput / $nodeResults.TransformThroughput
    if ($speedup -gt 1) {
        Write-Host "  Nova is $([math]::Round($speedup, 2))x faster than Node.js" -ForegroundColor Green
    } else {
        Write-Host "  Node.js is $([math]::Round(1/$speedup, 2))x faster than Nova" -ForegroundColor Yellow
    }
}
Write-Host ""

# Pipe Performance
Write-Host "Pipe Throughput:" -ForegroundColor Yellow
Write-Host "  Nova:    $($novaResults.PipeThroughput) MB/s"
Write-Host "  Node.js: $($nodeResults.PipeThroughput) MB/s"
Write-Host "  Bun:     $($bunResults.PipeThroughput) MB/s"
if ($novaResults.PipeThroughput -gt 0 -and $nodeResults.PipeThroughput -gt 0) {
    $speedup = $novaResults.PipeThroughput / $nodeResults.PipeThroughput
    if ($speedup -gt 1) {
        Write-Host "  Nova is $([math]::Round($speedup, 2))x faster than Node.js" -ForegroundColor Green
    } else {
        Write-Host "  Node.js is $([math]::Round(1/$speedup, 2))x faster than Nova" -ForegroundColor Yellow
    }
}
Write-Host ""

# Save results
@"
Stream Module Benchmark Results
Generated: $(Get-Date)
=====================================

Readable Stream Throughput (MB/s):
  Nova:    $($novaResults.ReadThroughput)
  Node.js: $($nodeResults.ReadThroughput)
  Bun:     $($bunResults.ReadThroughput)

Writable Stream Throughput (MB/s):
  Nova:    $($novaResults.WriteThroughput)
  Node.js: $($nodeResults.WriteThroughput)
  Bun:     $($bunResults.WriteThroughput)

Transform Stream Throughput (MB/s):
  Nova:    $($novaResults.TransformThroughput)
  Node.js: $($nodeResults.TransformThroughput)
  Bun:     $($bunResults.TransformThroughput)

Pipe Throughput (MB/s):
  Nova:    $($novaResults.PipeThroughput)
  Node.js: $($nodeResults.PipeThroughput)
  Bun:     $($bunResults.PipeThroughput)
"@ | Out-File $resultFile

Write-Host "Results saved to: $resultFile" -ForegroundColor Green
Write-Host ""
