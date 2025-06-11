#!/bin/bash

# Local Build Script for Summoner X Serum 2
# This script helps test builds locally before pushing to CI/CD

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🎵 Summoner X Serum 2 - Local Build Script${NC}"
echo "=================================================="

# Function to print colored output
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "Summoner-x-Serum-2/Summoner X Serum2.jucer" ]; then
    print_error "Please run this script from the project root directory"
    exit 1
fi

# Detect platform
PLATFORM=""
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    BUILD_DIR="Summoner-x-Serum-2/Builds/MacOSX"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    PLATFORM="Windows"
    BUILD_DIR="Summoner-x-Serum-2/Builds/VisualStudio2022"
else
    print_error "Unsupported platform: $OSTYPE"
    exit 1
fi

print_status "Detected platform: $PLATFORM"

# Parse command line arguments
BUILD_PLUGIN=false
TEST_LAMBDA=false
BUILD_TYPE="Release"

while [[ $# -gt 0 ]]; do
    case $1 in
        --plugin)
            BUILD_PLUGIN=true
            shift
            ;;
        --lambda)
            TEST_LAMBDA=true
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --plugin    Build the audio plugin"
            echo "  --lambda    Test the Lambda function locally"
            echo "  --debug     Build in Debug mode (default: Release)"
            echo "  --help      Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# If no options specified, show help
if [ "$BUILD_PLUGIN" = false ] && [ "$TEST_LAMBDA" = false ]; then
    print_warning "No build targets specified. Use --plugin and/or --lambda"
    echo "Use --help for more information"
    exit 1
fi

# Build plugin
if [ "$BUILD_PLUGIN" = true ]; then
    echo ""
    echo -e "${BLUE}🔨 Building Audio Plugin ($BUILD_TYPE)${NC}"
    echo "----------------------------------------"
    
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found: $BUILD_DIR"
        print_warning "Please regenerate project files with Projucer"
        exit 1
    fi
    
    cd "$BUILD_DIR"
    
    if [ "$PLATFORM" = "macOS" ]; then
        print_status "Building with Xcode..."
        
        # Check if Xcode command line tools are installed
        if ! command -v xcodebuild &> /dev/null; then
            print_error "Xcode command line tools not found"
            print_warning "Install with: xcode-select --install"
            exit 1
        fi
        
        # Build the project
        xcodebuild -project "Summoner X Serum2.xcodeproj" \
                   -scheme "Summoner X Serum2_All" \
                   -configuration $BUILD_TYPE \
                   -derivedDataPath ./build \
                   build
        
        print_status "Plugin build completed!"
        
        # Show built products
        if [ -d "./build/Build/Products/$BUILD_TYPE" ]; then
            echo ""
            print_status "Built products:"
            ls -la "./build/Build/Products/$BUILD_TYPE/"
        fi
        
    elif [ "$PLATFORM" = "Windows" ]; then
        print_status "Building with MSBuild..."
        
        # Check if MSBuild is available
        if ! command -v msbuild &> /dev/null; then
            print_error "MSBuild not found"
            print_warning "Make sure Visual Studio is installed and MSBuild is in PATH"
            exit 1
        fi
        
        # Build the solution
        msbuild "Summoner X Serum2.sln" /p:Configuration=$BUILD_TYPE /p:Platform=x64 /m
        
        print_status "Plugin build completed!"
        
        # Show built products
        if [ -d "x64/$BUILD_TYPE" ]; then
            echo ""
            print_status "Built products:"
            ls -la "x64/$BUILD_TYPE/"
        fi
    fi
    
    cd - > /dev/null
fi

# Test Lambda function
if [ "$TEST_LAMBDA" = true ]; then
    echo ""
    echo -e "${BLUE}⚡ Testing Lambda Function${NC}"
    echo "----------------------------"
    
    LAMBDA_DIR="Summoner AWS Backend/summoner-aws-backend"
    
    if [ ! -f "$LAMBDA_DIR/main.py" ]; then
        print_error "Lambda function not found: $LAMBDA_DIR/main.py"
        exit 1
    fi
    
    cd "$LAMBDA_DIR"
    
    # Check if Python is available
    if ! command -v python3 &> /dev/null; then
        print_error "Python 3 not found"
        exit 1
    fi
    
    print_status "Installing Lambda dependencies..."
    
    # Create virtual environment if it doesn't exist
    if [ ! -d "venv" ]; then
        python3 -m venv venv
    fi
    
    # Activate virtual environment
    source venv/bin/activate 2>/dev/null || source venv/Scripts/activate
    
    # Install dependencies
    pip install -r requirements.txt
    
    print_status "Testing Lambda function syntax..."
    python3 -m py_compile main.py
    
    print_status "Lambda function syntax check passed!"
    
    # Deactivate virtual environment
    deactivate
    
    cd - > /dev/null
fi

echo ""
print_status "All builds completed successfully!"
echo -e "${BLUE}🎉 Ready for CI/CD deployment!${NC}"