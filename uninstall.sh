#!/usr/bin/env bash

set -euo pipefail

INSTALL_DIR="/usr/local/bin"

echo "Uninstalling 'ink' from ${INSTALL_DIR}/ink..."
if [ -f "${INSTALL_DIR}/ink" ]; then
    sudo rm -f "${INSTALL_DIR}/ink"
    echo "Removed ink."
else
    echo "Not found, skipping."
fi

echo "Uninstalling 'quill' shim from ${INSTALL_DIR}/quill..."
if [ -f "${INSTALL_DIR}/quill" ]; then
    sudo rm -f "${INSTALL_DIR}/quill"
    echo "Removed quill shim."
else
    echo "Not found, skipping."
fi

echo "Uninstalling 'inkide' shim from ${INSTALL_DIR}/inkide..."
if [ -f "${INSTALL_DIR}/inkide" ]; then
    sudo rm -f "${INSTALL_DIR}/inkide"
    echo "Removed inkide."
else
    echo "Not found, skipping."
fi

echo "Also removing legacy 'InkIDE' shim if present..."
if [ -f "${INSTALL_DIR}/InkIDE" ]; then
    sudo rm -f "${INSTALL_DIR}/InkIDE"
    echo "Removed InkIDE."
fi

echo "Done."
