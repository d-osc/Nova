# Nova Installer for Windows
# Usage: powershell -c "irm nova-lang.org/install.ps1 | iex"

$ErrorActionPreference = "Stop"

$NovaVersion = $env:NOVA_VERSION
if (-not $NovaVersion) {
    $NovaVersion = "latest"
}

$GithubRepo = "d-osc/Nova"
$InstallDir = $env:NOVA_INSTALL_DIR
if (-not $InstallDir) {
    $InstallDir = "$env:USERPROFILE\.nova"
}
$BinDir = "$InstallDir\bin"

# Colors
function Write-Color {
    param(
        [string]$Text,
        [string]$Color = "White"
    )
    Write-Host $Text -ForegroundColor $Color
}

# Print header
Write-Host ""
Write-Color "┌─────────────────────────────────────┐" "Cyan"
Write-Color "│         Nova Installer              │" "Cyan"
Write-Color "│   TypeScript to Native Compiler     │" "Cyan"
Write-Color "└─────────────────────────────────────┘" "Cyan"
Write-Host ""

# Detect architecture
$Arch = [System.Environment]::Is64BitOperatingSystem
if (-not $Arch) {
    Write-Color "✗ Error: Only 64-bit Windows is supported" "Red"
    exit 1
}

Write-Color "→ Detected platform: " "Blue" -NoNewline
Write-Color "windows-x64" "Green"

# Download Nova
Write-Color "→ Downloading Nova..." "Blue"

$DownloadUrl = if ($NovaVersion -eq "latest") {
    "https://github.com/$GithubRepo/releases/latest/download/nova-windows-x64.zip"
} else {
    "https://github.com/$GithubRepo/releases/download/$NovaVersion/nova-windows-x64.zip"
}

# Create directory
New-Item -ItemType Directory -Force -Path $BinDir | Out-Null

$TempZip = "$env:TEMP\nova-windows-x64.zip"
$NovaExe = "$BinDir\nova.exe"

try {
    # Download with progress
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $DownloadUrl -OutFile $TempZip -UseBasicParsing
    Write-Color "✓ Nova downloaded successfully" "Green"

    # Extract zip
    Write-Color "→ Extracting Nova..." "Blue"
    Expand-Archive -Path $TempZip -DestinationPath $BinDir -Force

    # Rename exe if needed
    $ExtractedExe = "$BinDir\nova-windows-x64.exe"
    if (Test-Path $ExtractedExe) {
        Move-Item -Path $ExtractedExe -Destination $NovaExe -Force
    }

    # Clean up temp file
    Remove-Item -Path $TempZip -Force -ErrorAction SilentlyContinue

    Write-Color "✓ Nova extracted successfully" "Green"
} catch {
    Write-Color "✗ Error downloading/extracting Nova: $_" "Red"
    exit 1
}

# Add to PATH
Write-Color "→ Setting up PATH..." "Blue"

$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($CurrentPath -notlike "*$BinDir*") {
    [Environment]::SetEnvironmentVariable(
        "Path",
        "$BinDir;$CurrentPath",
        "User"
    )
    Write-Color "✓ Added Nova to PATH" "Green"
} else {
    Write-Color "! Nova is already in PATH" "Yellow"
}

# Update current session PATH
$env:Path = "$BinDir;$env:Path"

# Verify installation
Write-Color "→ Verifying installation..." "Blue"

if (Test-Path $NovaExe) {
    try {
        $Version = & $NovaExe --version 2>&1
        Write-Color "✓ Nova installed successfully!" "Green"
        Write-Color "  Version: $Version" "Cyan"
        Write-Color "  Location: $NovaExe" "Cyan"
    } catch {
        Write-Color "✗ Installation verification failed" "Red"
        exit 1
    }
} else {
    Write-Color "✗ Installation verification failed" "Red"
    exit 1
}

# Print next steps
Write-Host ""
Write-Color "╔════════════════════════════════════════════════════╗" "Cyan"
Write-Color "║             Installation Complete! 🎉              ║" "Cyan"
Write-Color "╚════════════════════════════════════════════════════╝" "Cyan"
Write-Host ""
Write-Color "To start using Nova, open a new terminal and run:" "Yellow"
Write-Host ""
Write-Color "  nova --version" "Green"
Write-Color "  nova --help" "Green"
Write-Host ""
Write-Color "Quick start:" "Yellow"
Write-Color "  nova run app.ts" "Green"
Write-Color "  nova build app.ts -o app.exe" "Green"
Write-Host ""
Write-Color "Documentation: " "Blue" -NoNewline
Write-Host "https://github.com/$GithubRepo"
Write-Host ""
Write-Color "Note: Restart your terminal to use Nova" "Yellow"
Write-Host ""
