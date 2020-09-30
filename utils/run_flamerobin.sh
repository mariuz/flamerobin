#!/bin/sh
# Util script to run FlameRobin under *nix build directory
# Useage: Copy to the build directory and run
export WX_FLAMEROBIN_DATA_DIR="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
echo "Running FlameRobin in : $WX_FLAMEROBIN_DATA_DIR"
$WX_FLAMEROBIN_DATA_DIR/flamerobin "$@"

