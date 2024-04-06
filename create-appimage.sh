#!/bin/bash

# exit on error
set -e

cd build-release
ninja

rm -rf AppDir
mkdir AppDir
DESTDIR=AppDir ninja install || true
NO_STRIP=true linuxdeploy \
    --appdir AppDir --output appimage \
    -d ../graph-generation/tool.desktop \
    -i ../graph-generation/tool.png
