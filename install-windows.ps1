# Nova Language - Windows Installation Script
# This script installs Nova and all required dependencies on Windows

param(
    [switch]$SkipLLVM = $false,
    [string]$InstallDir = "C:\Program Files\Nova"
)

# Requires admin privileges
#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

# Colors
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

function Write-Info { Write-ColorOutput Cyan $args }
function Write-Success { Write-ColorOutput Green $args }
function Write-Warning { Write-ColorOutput Yellow $args }
function Write-Error { Write-ColorOutput Red $args }

Write-Info "═══════════════════════════════════════════════════════"
Write-Info "                                                       "
Write-Info "         Nova Language Installer - Windows            "
Write-Info "                                                       "
Write-Info "═══════════════════════════════════════════════════════"
Write-Output ""

# Detect Windows version
$WindowsVersion = [System.Environment]::OSVersion.Version
$WindowsName = (Get-WmiObject -class Win32_OperatingSystem).Caption

Write-Info "Detected OS: $WindowsName ($WindowsVersion)"
Write-Output ""

# Function to check if command exists
function Test-CommandExists {
    param($Command)
    $null -ne (Get-Command $Command -ErrorAction SilentlyContinue)
}

# Function to install Chocolatey
function Install-Chocolatey {
    if (Test-CommandExists choco) {
        Write-Success "✓ Chocolatey is already installed"
        return
    }

    Write-Warning "Installing Chocolatey package manager..."

    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

    # Refresh environment variables
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "✓ Chocolatey installed"
}

# Function to install Visual Studio Build Tools
function Install-VisualStudio {
    Write-Info "Checking Visual Studio Build Tools..."

    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path $vsWhere) {
        $vsInstall = & $vsWhere -latest -property installationPath
        if ($vsInstall) {
            Write-Success "✓ Visual Studio Build Tools found at: $vsInstall"
            return
        }
    }

    Write-Warning "Visual Studio Build Tools not found. Installing..."

    # Install via Chocolatey
    choco install visualstudio2019buildtools -y --package-parameters "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

    Write-Success "✓ Visual Studio Build Tools installed"
}

# Function to install dependencies
function Install-Dependencies {
    Write-Info "Installing dependencies via Chocolatey..."

    # Install required tools
    $packages = @(
        "cmake",
        "ninja",
        "git",
        "python3"
    )

    foreach ($package in $packages) {
        Write-Info "Installing $package..."
        choco install $package -y --force
    }

    # Refresh environment
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "✓ Dependencies installed"
}

# Function to install LLVM 18
function Install-LLVM {
    if ($SkipLLVM) {
        Write-Warning "Skipping LLVM installation (--SkipLLVM specified)"
        return
    }

    Write-Info "Checking LLVM installation..."

    # Check if LLVM 18 is installed
    if (Test-Path "C:\Users\ondev\llvm\bin\llvm-config.exe") {
        Write-Success "✓ LLVM 18 is already installed"
        return
    }

    Write-Warning "Installing LLVM 18..."
    Write-Warning "This may take 10-15 minutes..."

    # Download LLVM 18 pre-built binary
    $llvmUrl = "https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.7/LLVM-18.1.7-win64.exe"
    $llvmInstaller = "$env:TEMP\llvm-18.1.7-installer.exe"

    Write-Info "Downloading LLVM 18..."
    Invoke-WebRequest -Uri $llvmUrl -OutFile $llvmInstaller -UseBasicParsing

    Write-Info "Installing LLVM 18..."
    Start-Process -FilePath $llvmInstaller -ArgumentList "/S" -Wait

    # Add LLVM to PATH
    $llvmPath = "C:\Program Files\LLVM\bin"
    if (Test-Path $llvmPath) {
        $currentPath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
        if ($currentPath -notlike "*$llvmPath*") {
            [System.Environment]::SetEnvironmentVariable(
                "Path",
                "$currentPath;$llvmPath",
                "Machine"
            )
        }
    }

    # Refresh environment
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Remove-Item $llvmInstaller -Force

    Write-Success "✓ LLVM 18 installed"
}

# Function to build Nova
function Build-Nova {
    Write-Info "Building Nova..."

    # Find CMake
    if (-not (Test-CommandExists cmake)) {
        Write-Error "✗ CMake not found. Please restart PowerShell to refresh PATH."
        exit 1
    }

    # Find LLVM
    $llvmPath = $null
    $possibleLLVMPaths = @(
        "C:\Users\ondev\llvm",
        "C:\Program Files\LLVM",
        "C:\LLVM"
    )

    foreach ($path in $possibleLLVMPaths) {
        if (Test-Path "$path\bin\llvm-config.exe") {
            $llvmPath = $path
            break
        }
    }

    if (-not $llvmPath) {
        Write-Error "✗ LLVM not found. Please install LLVM 18 first."
        exit 1
    }

    Write-Info "Using LLVM at: $llvmPath"

    # Configure with CMake
    $buildDir = "build\windows-release"
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

    cmake -B $buildDir -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_CXX_STANDARD=20 `
        -DLLVM_DIR="$llvmPath\lib\cmake\llvm"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "✗ CMake configuration failed"
        exit 1
    }

    # Build
    Write-Info "Compiling Nova (this may take a few minutes)..."
    cmake --build $buildDir --config Release

    if ($LASTEXITCODE -ne 0) {
        Write-Error "✗ Build failed"
        exit 1
    }

    Write-Success "✓ Nova built successfully"
}

# Function to install Nova
function Install-Nova {
    Write-Info "Installing Nova to $InstallDir..."

    # Create install directory
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

    # Copy executable
    $buildDir = "build\windows-release"
    $novaExe = "$buildDir\nova.exe"

    if (-not (Test-Path $novaExe)) {
        Write-Error "✗ Nova executable not found at $novaExe"
        exit 1
    }

    Copy-Item $novaExe -Destination "$InstallDir\nova.exe" -Force

    # Add to PATH
    $currentPath = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
    if ($currentPath -notlike "*$InstallDir*") {
        [System.Environment]::SetEnvironmentVariable(
            "Path",
            "$currentPath;$InstallDir",
            "Machine"
        )
        Write-Success "✓ Added Nova to system PATH"
    }

    # Refresh environment for current session
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    Write-Success "✓ Nova installed to $InstallDir"
}

# Function to verify installation
function Test-Installation {
    Write-Info "Verifying installation..."

    # Refresh PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    if (Test-CommandExists nova) {
        try {
            $version = & nova --version 2>&1
            Write-Success "✓ Nova is installed: $version"
            Write-Output ""
            Write-Info "Installation successful!"
            Write-Output ""
            Write-Warning "Quick start:"
            Write-Output "  nova --version          # Check version"
            Write-Output "  nova --help             # Show help"
            Write-Output "  nova run hello.ts       # Run a TypeScript file"
            Write-Output "  nova build app.ts       # Build to executable"
            Write-Output ""
            Write-Warning "Note: You may need to restart your terminal/PowerShell to use nova."
            return $true
        } catch {
            Write-Error "✗ Nova command failed: $_"
            return $false
        }
    } else {
        Write-Error "✗ Nova installation failed"
        Write-Output "Please check the error messages above."
        return $false
    }
}

# Main installation flow
function Main {
    Write-Info "Starting installation..."
    Write-Output ""

    # Check if we're in the Nova source directory
    if (-not (Test-Path "CMakeLists.txt")) {
        Write-Error "Error: This script must be run from the Nova source directory"
        exit 1
    }

    # Check for admin rights
    $isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $isAdmin) {
        Write-Error "Error: This script requires Administrator privileges."
        Write-Output "Please run PowerShell as Administrator and try again."
        exit 1
    }

    # Ask for confirmation
    Write-Warning "This script will:"
    Write-Output "  1. Install Chocolatey package manager"
    Write-Output "  2. Install Visual Studio Build Tools"
    Write-Output "  3. Install dependencies (CMake, Ninja, Git, Python)"
    Write-Output "  4. Install LLVM 18"
    Write-Output "  5. Build Nova from source"
    Write-Output "  6. Install Nova to $InstallDir"
    Write-Output ""

    $response = Read-Host "Continue? (Y/n)"
    if ($response -eq "n" -or $response -eq "N") {
        Write-Output "Installation cancelled."
        exit 0
    }

    # Run installation steps
    try {
        Install-Chocolatey
        Write-Output ""

        Install-VisualStudio
        Write-Output ""

        Install-Dependencies
        Write-Output ""

        Install-LLVM
        Write-Output ""

        Build-Nova
        Write-Output ""

        Install-Nova
        Write-Output ""

        $success = Test-Installation

        if (-not $success) {
            exit 1
        }
    } catch {
        Write-Error "Installation failed: $_"
        Write-Output $_.ScriptStackTrace
        exit 1
    }
}

# Run main installation
Main
