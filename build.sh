#!/bin/bash

set -e

# Parse arguments
REBUILD_WEBCC=false
REBUILD_SCHEMA=false
for arg in "$@"; do
    case $arg in
        --rebuild-webcc|--force-webcc)
            REBUILD_WEBCC=true
            ;;
        --rebuild-schema|--force-schema)
            REBUILD_SCHEMA=true
            ;;
        --help|-h)
            echo "Usage: ./build.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --rebuild-webcc   Force rebuild the webcc toolchain"
            echo "  --rebuild-schema  Force regenerate COI schema files (coi_schema.h/cc and def/*.d.coi)"
            echo "  --help            Show this help message"
            exit 0
            ;;
    esac
done

# Force rebuild webcc if requested
if [ "$REBUILD_WEBCC" = true ]; then
    echo "[COI] Force rebuilding webcc toolchain..."
    if [ -d "deps/webcc" ]; then
        pushd deps/webcc > /dev/null
        ./build.sh
        popd > /dev/null
    else
        echo "[COI] Error: deps/webcc not found"
        exit 1
    fi
fi

# Force rebuild schema if requested
if [ "$REBUILD_SCHEMA" = true ]; then
    echo "[COI] Force rebuilding COI schema..."
    # Remove generated schema files to force regeneration
    rm -f src/coi_schema.h src/coi_schema.cc
    rm -f build/gen_schema build/obj/gen_schema.o
    # Also regenerate .d.coi definition files
    rm -f def/*.d.coi
    echo "[COI] Schema files removed."
fi

# Check for webcc dependency
if ! command -v webcc >/dev/null 2>&1; then
    echo "[COI] webcc not found in PATH. Checking submodule..."
    if [ ! -f "deps/webcc/build.sh" ]; then
        echo "[COI] Initializing webcc submodule..."
        git submodule update --init --recursive
    fi
    
    if [ ! -f "deps/webcc/webcc" ]; then
        echo "[COI] Building and installing webcc toolchain (required dependency)..."
        pushd deps/webcc > /dev/null
        # Automatically answer 'y' to the install prompt in webcc's build script
        echo "y" | ./build.sh
        popd > /dev/null
    fi
    
    # Add webcc to PATH for the rest of the script if it was built locally
    export PATH="$PWD/deps/webcc:$PATH"
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "[COI] Ninja not found. Please install ninja-build."
    exit 1
fi

echo "[COI] Running Ninja..."
ninja

# Check if coi is already linked correctly
if command -v coi >/dev/null 2>&1 && [ "$(command -v coi)" -ef "$PWD/coi" ]; then
    echo "coi is already configured in PATH."
    exit 0
fi

# Only offer install if /usr/local/bin exists (common on Linux/macOS)
# And we are in an interactive terminal and not in CIs
if [ -d "/usr/local/bin" ] && [ -t 0 ] && [ -z "$CI" ]; then
    echo ""
    
    # Offer to install coi
    if ! command -v coi >/dev/null 2>&1 || [ ! "$(command -v coi)" -ef "$PWD/coi" ]; then
        read -p "Would you like to create a symlink for 'coi' in /usr/local/bin? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            TARGET="/usr/local/bin/coi"
            if [ -w /usr/local/bin ]; then
                ln -sf "$PWD/coi" "$TARGET"
            else
                echo "Need sudo access to write to /usr/local/bin"
                sudo ln -sf "$PWD/coi" "$TARGET"
            fi
            echo "Symlink created: $TARGET -> $PWD/coi"
            echo "You can now use 'coi' command from any directory."
        fi
    fi
fi
