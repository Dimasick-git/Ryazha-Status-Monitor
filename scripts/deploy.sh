#!/bin/bash

# Ryazha-Status-Monitor Deploy Script
# Автоматическое развертывание и релиз

set -e

echo "🚀 Starting Ryazha-Status-Monitor deployment..."

# Configuration
REPO_NAME="Ryazha-Status-Monitor"
GITHUB_USER="Dimasick-git"
RELEASE_DIR="releases"

# Check if we're in a git repository
if ! git rev-parse --git-head > /dev/null 2>&1; then
    echo "❌ Not in a git repository"
    exit 1
fi

# Get current version from git tag or use timestamp
VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "v$(date +%Y.%m.%d)")
echo "📦 Version: $VERSION"

# Create release directory
mkdir -p "$RELEASE_DIR"

# Copy all necessary files to release directory
echo "📋 Preparing release files..."
cp *.nro "$RELEASE_DIR/" 2>/dev/null || echo "⚠️  No .nro files found"
cp *.elf "$RELEASE_DIR/" 2>/dev/null || echo "⚠️  No .elf files found"
cp *.json "$RELEASE_DIR/" 2>/dev/null || echo "⚠️  No .json files found"
cp README.md "$RELEASE_DIR/" 2>/dev/null || echo "⚠️  README.md not found"
cp LICENSE "$RELEASE_DIR/" 2>/dev/null || echo "⚠️  LICENSE not found"

# Copy configuration files
if [ -d "config" ]; then
    cp -r config "$RELEASE_DIR/"
    echo "✅ Configuration files copied"
fi

# Copy documentation
if [ -d "docs" ]; then
    cp -r docs "$RELEASE_DIR/"
    echo "✅ Documentation copied"
fi

# Create installation script
cat > "$RELEASE_DIR/install.sh" << 'EOF'
#!/bin/bash

# Ryazha-Status-Monitor Installation Script

echo "🎮 Ryazha-Status-Monitor Installation"
echo "===================================="

# Check if we're on Switch
if [ ! -d "/switch" ]; then
    echo "❌ This script must be run on Nintendo Switch"
    exit 1
fi

# Create installation directory
INSTALL_DIR="/switch/Ryazha-Status-Monitor"
mkdir -p "$INSTALL_DIR"

# Copy files
echo "📦 Installing files..."
cp *.nro "$INSTALL_DIR/"
cp -r config "$INSTALL_DIR/" 2>/dev/null || true
cp -r docs "$INSTALL_DIR/" 2>/dev/null || true

echo "✅ Installation completed!"
echo "📂 Files installed to: $INSTALL_DIR"
echo "🎮 Launch Ryazha-Status-Monitor from Tesla Menu"
EOF

chmod +x "$RELEASE_DIR/install.sh"

# Create package info
cat > "$RELEASE_DIR/package.json" << EOF
{
  "name": "Ryazha-Status-Monitor",
  "version": "$VERSION",
  "description": "Nintendo Switch system monitoring overlay",
  "author": "Dimasick-git",
  "license": "GPL-2.0",
  "platform": "Nintendo Switch",
  "dependencies": {
    "tesla": ">=1.2.3",
    "libnx": ">=4.0.0"
  },
  "files": [
    "Ryazha-Status-Monitor.nro",
    "config/",
    "docs/",
    "README.md",
    "LICENSE"
  ]
}
EOF

# Create checksums
echo "🔐 Creating checksums..."
cd "$RELEASE_DIR"
sha256sum *.nro *.elf *.json 2>/dev/null > checksums.txt || sha256sum *.nro 2>/dev/null > checksums.txt || echo "No files to checksum"
cd ..

# Create zip archive
echo "📦 Creating archive..."
ARCHIVE_NAME="${REPO_NAME}-${VERSION}.zip"
cd "$RELEASE_DIR"
zip -r "../$ARCHIVE_NAME" *
cd ..
echo "✅ Archive created: $ARCHIVE_NAME"

# Git operations
if [ "$1" = "--release" ]; then
    echo "🏷️  Creating release..."
    
    # Add and commit changes
    git add .
    git commit -m "Release $VERSION" || echo "No changes to commit"
    
    # Create tag
    git tag -a "$VERSION" -m "Release $VERSION" || echo "Tag already exists"
    
    # Push to GitHub
    git push origin main
    git push origin "$VERSION" || echo "Tag push failed"
    
    echo "✅ Release pushed to GitHub"
fi

echo "🎉 Deployment completed!"
echo "📦 Package: $ARCHIVE_NAME"
echo "📂 Release directory: $RELEASE_DIR"
echo "🔐 Checksums: $RELEASE_DIR/checksums.txt"

if [ "$1" = "--release" ]; then
    echo "🏷️  Release version: $VERSION"
    echo "🌐 GitHub: https://github.com/$GITHUB_USER/$REPO_NAME/releases"
fi
