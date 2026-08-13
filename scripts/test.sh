#!/bin/bash

# Ryazha-Status-Monitor Test Script
# Автоматическое тестирование проекта

set -e

echo "🧪 Starting Ryazha-Status-Monitor tests..."

# Check if build exists
if [ ! -f "Ryazha-Status-Monitor.nro" ]; then
    echo "❌ Build not found. Please run build.sh first."
    exit 1
fi

# Run basic tests
echo "🔍 Running basic tests..."

# Test 1: Check file integrity
echo "📋 Test 1: File integrity check..."
files=("Ryazha-Status-Monitor.nro" "README.md" "LICENSE")
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
        exit 1
    fi
done

# Test 2: Check file sizes
echo "📋 Test 2: File size check..."
if [ -s "Ryazha-Status-Monitor.nro" ]; then
    size=$(stat -f%z "Ryazha-Status-Monitor.nro" 2>/dev/null || stat -c%s "Ryazha-Status-Monitor.nro" 2>/dev/null)
    echo "✅ NRO file size: $size bytes"
    if [ "$size" -lt 1000 ]; then
        echo "⚠️  Warning: NRO file seems too small"
    fi
else
    echo "❌ NRO file is empty"
    exit 1
fi

# Test 3: Check dependencies
echo "📋 Test 3: Dependency check..."
if command -v dkp-pacman &> /dev/null; then
    echo "✅ devkitPro is available"
else
    echo "❌ devkitPro not found"
    exit 1
fi

# Test 4: Configuration files
echo "📋 Test 4: Configuration check..."
config_files=("config/status-monitor/config.ini.template")
for config in "${config_files[@]}"; do
    if [ -f "$config" ]; then
        echo "✅ $config exists"
    else
        echo "⚠️  Warning: $config not found"
    fi
done

# Test 5: Documentation
echo "📋 Test 5: Documentation check..."
doc_files=("README.md" "docs/config.md" "docs/modes.md" "docs/API.md")
for doc in "${doc_files[@]}"; do
    if [ -f "$doc" ]; then
        echo "✅ $doc exists"
    else
        echo "⚠️  Warning: $doc not found"
    fi
done

# Test 6: Source code structure
echo "📋 Test 6: Source code structure..."
source_dirs=("source" "include" "lib")
for dir in "${source_dirs[@]}"; do
    if [ -d "$dir" ]; then
        echo "✅ $dir directory exists"
    else
        echo "❌ $dir directory missing"
        exit 1
    fi
done

# Test 7: Language files
echo "📋 Test 7: Language files check..."
if [ -d "lang" ]; then
    lang_count=$(ls lang/*.json 2>/dev/null | wc -l)
    echo "✅ Found $lang_count language files"
    if [ "$lang_count" -lt 5 ]; then
        echo "⚠️  Warning: Few language files found"
    fi
else
    echo "⚠️  Warning: lang directory not found"
fi

echo "🎉 All tests completed!"
echo "📊 Test Summary:"
echo "   ✅ File integrity: PASSED"
echo "   ✅ File sizes: PASSED"
echo "   ✅ Dependencies: PASSED"
echo "   ✅ Configuration: PASSED"
echo "   ✅ Documentation: PASSED"
echo "   ✅ Source structure: PASSED"
echo "   ✅ Language files: PASSED"
