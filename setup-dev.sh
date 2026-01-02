#!/bin/bash
#
# Setup script for histograms development environment
#
# This script:
# 1. Installs pre-commit hooks
# 2. Creates cmake-build-clang-tidy directory for clang-tidy checks
# 3. Verifies required tools are installed

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOOKS_DIR="$SCRIPT_DIR/hooks"
GIT_HOOKS_DIR="$SCRIPT_DIR/.git/hooks"

echo "Setting up histograms development environment..."
echo ""

# Install pre-commit hooks
echo "1. Installing pre-commit hooks..."

# Check if .git/hooks directory exists
if [ ! -d "$GIT_HOOKS_DIR" ]; then
    echo "Error: .git/hooks directory not found."
    echo "Are you running this from the repository root?"
    exit 1
fi

# Install pre-commit hook
if [ -f "$HOOKS_DIR/pre-commit" ]; then
    cp "$HOOKS_DIR/pre-commit" "$GIT_HOOKS_DIR/pre-commit"
    chmod +x "$GIT_HOOKS_DIR/pre-commit"
    echo "   ✅ pre-commit hook installed"
else
    echo "   ❌ hooks/pre-commit not found"
    exit 1
fi

# Create cmake-build-clang-tidy directory for clang-tidy
echo ""
echo "2. Configuring CMake for clang-tidy..."
if [ ! -d "cmake-build-clang-tidy" ]; then
    cmake -S . -B cmake-build-clang-tidy \
        -DCMAKE_CXX_COMPILER=clang++-19 \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_BUILD_TYPE=Debug
    echo "   ✅ Created cmake-build-clang-tidy/"
else
    echo "   ✅ cmake-build-clang-tidy/ already exists"
fi

# Verify required tools
echo ""
echo "3. Verifying required tools..."

MISSING_TOOLS=0

if command -v clang-format &> /dev/null; then
    echo "   ✅ clang-format found"
else
    echo "   ❌ clang-format not found"
    MISSING_TOOLS=1
fi

if command -v clang-tidy-19 &> /dev/null; then
    echo "   ✅ clang-tidy-19 found"
else
    echo "   ❌ clang-tidy-19 not found"
    MISSING_TOOLS=1
fi

if command -v cmake &> /dev/null; then
    echo "   ✅ cmake found"
else
    echo "   ❌ cmake not found"
    MISSING_TOOLS=1
fi

echo ""
if [ $MISSING_TOOLS -eq 0 ]; then
    echo "✅ Development environment setup complete!"
    echo ""
    echo "Pre-commit hooks will now:"
    echo "  - Auto-format C++ files with clang-format"
    echo "  - Check production headers with clang-tidy"
else
    echo "⚠️  Setup complete, but some tools are missing."
    echo ""
    echo "To install missing tools on Ubuntu 24.04:"
    echo "  sudo apt-get install clang-format clang-tidy-19 cmake"
fi

echo ""
echo "Happy coding! 🚀"
