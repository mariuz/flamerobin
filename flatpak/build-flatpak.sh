#!/bin/bash
set -e

# This script builds the FlameRobin Flatpak locally.
# It requires flatpak and flatpak-builder to be installed.

# Install the freedesktop runtime and sdk if not already present
flatpak install -y flathub org.freedesktop.Platform//24.08 org.freedesktop.Sdk//24.08

# Build the flatpak
flatpak-builder --force-clean --user --install build-dir org.flamerobin.FlameRobin.yml

# To create a bundle:
# flatpak-builder --bundle flamerobin.flatpak org.flamerobin.FlameRobin.yml
