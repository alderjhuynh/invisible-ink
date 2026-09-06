#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="/usr/local/bin"

echo "Installing 'ink' from ${SCRIPT_DIR}/src"
pushd "${SCRIPT_DIR}/src" > /dev/null

if [ ! -f "ink" ]; then
    echo "No 'ink' binary found, building..."
    make ink
fi

if [ ! -f "ink" ]; then
    echo "Error: 'ink' binary still not found, exiting." >&2
    exit 1
fi

echo "Installing 'ink' to ${INSTALL_DIR}/ink..."
sudo cp -f "ink" "${INSTALL_DIR}/ink"
sudo chmod +x "${INSTALL_DIR}/ink"

if [ ! -f "quill" ]; then
    echo "No 'quill' binary found, building..."
    make quill
fi

if [ ! -f "quill" ]; then
    echo "Error: 'quill' binary still not found, exiting." >&2
    exit 1
fi

echo "Installing 'quill' to ${INSTALL_DIR}/quill..."
sudo cp -f "quill" "${INSTALL_DIR}/quill"
sudo chmod +x "${INSTALL_DIR}/quill"

popd > /dev/null

echo "Installing 'InkIDE' from ${SCRIPT_DIR}/ide"
pushd "${SCRIPT_DIR}/ide" > /dev/null

if [ ! -f "InkIDE" ]; then
    echo "No 'InkIDE' binary found, building..."
    gcc -o InkIDE ink-ide.c
fi

if [ ! -f "InkIDE" ]; then
    echo "Error: 'InkIDE' binary still not found after compiling, exiting" >&2
    exit 1
fi

echo "    Installing 'InkIDE' to ${INSTALL_DIR}/inkide..."
sudo cp -f "InkIDE" "${INSTALL_DIR}/inkide"
sudo chmod +x "${INSTALL_DIR}/inkide"

popd > /dev/null

echo "Done."
echo "'ink <file>'    is now available globally."
echo "'quill <cmd>'  is now available globally."
echo "'inkide <file>' is now available globally."