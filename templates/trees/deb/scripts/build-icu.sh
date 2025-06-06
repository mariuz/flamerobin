#!/bin/bash
set -e

ICU_VERSION_MAJOR=74
ICU_VERSION=74.2
INSTALL_PREFIX="/opt/icu"

# Download and extract ICU if not already present
if [ ! -d "icu" ]; then
  curl -LO https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION//./-}/icu4c-${ICU_VERSION//./_}-src.tgz
  tar xf icu4c-${ICU_VERSION//./_}-src.tgz
  mv icu icu-src
fi

# Build ICU
cd icu-src/source
./configure --prefix="${INSTALL_PREFIX}" --disable-samples --disable-tests
make -j$(nproc)
make install

