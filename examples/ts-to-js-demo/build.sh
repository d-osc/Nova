#!/bin/bash
# Nova TypeScript to JavaScript Build Script

NOVA="../../build/Release/nova.exe"
OUTDIR="dist"

echo "Building TypeScript to JavaScript..."
echo

mkdir -p $OUTDIR/models $OUTDIR/services $OUTDIR/utils

$NOVA -b src/models/User.ts --outDir $OUTDIR/models "$@"
$NOVA -b src/models/Product.ts --outDir $OUTDIR/models "$@"
$NOVA -b src/utils/helpers.ts --outDir $OUTDIR/utils "$@"
$NOVA -b src/services/CartService.ts --outDir $OUTDIR/services "$@"
$NOVA -b src/index.ts --outDir $OUTDIR "$@"

echo
echo "Build complete! Output in $OUTDIR/"
echo
echo "Run with: node $OUTDIR/index.js"
