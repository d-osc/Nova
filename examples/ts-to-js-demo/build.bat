@echo off
REM Nova TypeScript to JavaScript Build Script

set NOVA=..\..\build\Release\nova.exe
set OUTDIR=dist

echo Building TypeScript to JavaScript...
echo.

if not exist %OUTDIR%\models mkdir %OUTDIR%\models
if not exist %OUTDIR%\services mkdir %OUTDIR%\services
if not exist %OUTDIR%\utils mkdir %OUTDIR%\utils

%NOVA% -b src/models/User.ts --outDir %OUTDIR%/models %*
%NOVA% -b src/models/Product.ts --outDir %OUTDIR%/models %*
%NOVA% -b src/utils/helpers.ts --outDir %OUTDIR%/utils %*
%NOVA% -b src/services/CartService.ts --outDir %OUTDIR%/services %*
%NOVA% -b src/index.ts --outDir %OUTDIR% %*

echo.
echo Build complete! Output in %OUTDIR%/
echo.
echo Run with: node %OUTDIR%/index.js
