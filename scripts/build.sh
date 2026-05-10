#!/bin/bash

# Ryazha-Status-Monitor Build Script
# Автоматическая сборка для Nintendo Switch

set -e

echo "🚀 Starting Ryazha-Status-Monitor build..."

# Check if devkitPro is installed
if ! command -v dkp-pacman &> /dev/null; then
    echo "❌ devkitPro not found. Please install devkitPro first."
    exit 1
fi

# Check if required packages are installed
echo "📦 Checking dependencies..."
required_packages=("switch-devkitA64" "switch-tools" "switch-libnx" "switch-mesa" "switch-glad")

for package in "${required_packages[@]}"; do
    if ! dkp-pacman -Qi "$package" &> /dev/null; then
        echo "📦 Installing $package..."
        sudo dkp-pacman -S "$package"
    else
        echo "✅ $package is already installed"
    fi
done

# Clean previous builds
echo "🧹 Cleaning previous builds..."
make clean || true

# Build the project
echo "🔨 Building Ryazha-Status-Monitor..."
make switch

# Check if build was successful
if [ -f "Ryazha-Status-Monitor.nro" ]; then
    echo "✅ Build successful!"
    echo "📁 Output files:"
    ls -la *.nro *.elf *.json 2>/dev/null || echo "No output files found"
else
    echo "❌ Build failed!"
    exit 1
fi

# Create release directory
echo "📦 Creating release package..."
mkdir -p releases
cp *.nro *.elf *.json README.md LICENSE releases/ 2>/dev/null || true

echo "🎉 Build completed successfully!"
echo "📂 Release files are in the 'releases' directory"
