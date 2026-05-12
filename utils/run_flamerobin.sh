#!/bin/sh
# Util script to run FlameRobin under *nix build directory
# Useage: Copy to the build directory and run
export FR_HOME="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
echo "Running FlameRobin in : $FR_HOME"
$FR_HOME/flamerobin "$@"

