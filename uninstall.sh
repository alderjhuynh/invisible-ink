#!/usr/bin/env bash

set -euo pipefail

INSTALL_DIR="/usr/local/bin"

echo "Uninstalling 'ink' from ${INSTALL_DIR}/ink..."
if [ -f "${INSTALL_DIR}/ink" ]; then
    sudo rm -f "${INSTALL_DIR}/ink"
    echo "Removed."
else
    echo "Not found, skipping."
fi

echo "Uninstalling 'quill' from ${INSTALL_DIR}/quill..."
if [ -f "${INSTALL_DIR}/quill" ]; then
    sudo rm -f "${INSTALL_DIR}/quill"
    echo "Removed."
else
    echo "Not found, skipping."
fi

echo "Uninstalling 'inkide' from ${INSTALL_DIR}/inkide..."
if [ -f "${INSTALL_DIR}/inkide" ]; then
    sudo rm -f "${INSTALL_DIR}/inkide"
    echo "Removed."
else
    echo "Not found, skipping."
fi

echo "Done."