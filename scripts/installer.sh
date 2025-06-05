#!/usr/bin/env bash

# Bash script to download and optionally install a VS Code extension from the Marketplace.
# Usage: ./installer.sh [-i] <publisher.extension>

set -e

usage() {
    echo "Usage: $0 [-i] <publisher.extension>"
    exit 1
}

# Check for required commands
require_cmd() {
    for cmd in "$@"; do
        if ! command -v "$cmd" &>/dev/null; then
            echo "Missing required command: $cmd"
            exit 1
        fi
    done
}

# Detect code CLI
detect_code_command() {
    for cmd in code codium vscodium; do
        if command -v "$cmd" &>/dev/null; then
            echo "$cmd"
            return 0
        fi
    done
    echo "No VS Code CLI tool found (tried 'code', 'codium', 'vscodium')." >&2
    exit 1
}

INSTALL_FLAG=0
EXTENSION_ID=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i)
            INSTALL_FLAG=1
            shift
            ;;
        *)
            if [[ "$1" == *.* ]]; then
                EXTENSION_ID="$1"
                shift
            else
                usage
            fi
            ;;
    esac
done

if [[ -z "$EXTENSION_ID" ]]; then
    echo "Error: Missing extension name (e.g., publisher.extension)"
    usage
fi

if [[ "$EXTENSION_ID" != *.* ]]; then
    echo "Extension ID must be in format 'publisher.extension'"
    exit 1
fi

PUBLISHER="${EXTENSION_ID%%.*}"
EXTENSION="${EXTENSION_ID#*.}"
FILENAME="${EXTENSION}.vsix"
MARKETPLACE_URL="https://marketplace.visualstudio.com/items?itemName=${PUBLISHER}.${EXTENSION}&ssr=false#overview"

require_cmd curl grep sed awk

# Fetch the Marketplace page
echo "Fetching Marketplace page for $PUBLISHER.$EXTENSION..."
HTML=$(curl -fsSL "$MARKETPLACE_URL")

# Extract assetUri or fallbackAssetUri
ASSET_URI=$(echo "$HTML" | grep -oP '"assetUri"\s*:\s*"\K([^"]+)' | head -n1)
FALLBACK_URI=$(echo "$HTML" | grep -oP '"fallbackAssetUri"\s*:\s*"\K([^"]+)' | head -n1)

if [[ -n "$ASSET_URI" ]]; then
    VSIX_URL="${ASSET_URI}/Microsoft.VisualStudio.Services.VSIXPackage"
    echo "Found VSIX URL via assetUri: $VSIX_URL"
elif [[ -n "$FALLBACK_URI" ]]; then
    VSIX_URL="${FALLBACK_URI}/Microsoft.VisualStudio.Services.VSIXPackage"
    echo "Found VSIX URL via fallbackAssetUri: $VSIX_URL"
else
    echo "Could not find assetUri or fallbackAssetUri in page."
    exit 1
fi

# Download the VSIX file
echo "Downloading VSIX to $FILENAME..."
curl -fsSL "$VSIX_URL" -o "$FILENAME"
echo "Download complete."

# Optionally install the extension
if [[ "$INSTALL_FLAG" -eq 1 ]]; then
    CODE_CMD=$(detect_code_command)
    echo "Installing extension using $CODE_CMD from $FILENAME..."
    "$CODE_CMD" --install-extension "$FILENAME"
    echo "Installation complete!"
else
    echo "Skipping installation. Use '-i' to install."
fi