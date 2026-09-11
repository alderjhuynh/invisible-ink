#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="/usr/local/bin"

echo "Installing 'ink' from ${SCRIPT_DIR}/src (single unified binary)"
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

popd > /dev/null

echo "Installing 'inkide' compatibility shim to ${INSTALL_DIR}/inkide (forwards to 'ink edit')..."
sudo tee "${INSTALL_DIR}/inkide" > /dev/null <<'EOS'
#!/usr/bin/env bash
# Compatibility shim: editor is now built into `ink`
exec ink edit "$@"
EOS
sudo chmod +x "${INSTALL_DIR}/inkide"
if [ ! -f "${INSTALL_DIR}/InkIDE" ] || [ -L "${INSTALL_DIR}/InkIDE" ]; then
    echo "Installing 'InkIDE' compatibility shim to ${INSTALL_DIR}/InkIDE..."
    sudo tee "${INSTALL_DIR}/InkIDE" > /dev/null <<'EOS'
#!/usr/bin/env bash
exec ink edit "$@"
EOS
    sudo chmod +x "${INSTALL_DIR}/InkIDE"
fi

echo "Installing 'quill' compatibility shim to ${INSTALL_DIR}/quill..."
sudo tee "${INSTALL_DIR}/quill" > /dev/null <<'EOS'
#!/usr/bin/env bash
# Deprecated: use `ink assemble` / `ink disassemble` instead
exec ink "$1" "${@:2}"
# Map old quill subcommands to new ink ones if needed
EOS
sudo chmod +x "${INSTALL_DIR}/quill"

echo "Done."
echo "'ink' is now available globally."
echo "  ink run <file>              — run a program"
echo "  ink dump <file>             — dump AST"
echo "  ink disassemble <file>      — Invisible Ink -> Quill"
echo "  ink assemble <file>         — Quill -> Invisible Ink"
echo "  ink check <file>            — validate without running"
echo "  ink edit [file]             — launch visual editor (alias: ink ide)"
echo "  ink help / ink version      — help / version"
echo "  ink <file>                  — legacy run (still works)"
echo "'quill' shim still installed for compatibility (deprecated)."
echo "'inkide'/'InkIDE' now shims to 'ink edit'."
