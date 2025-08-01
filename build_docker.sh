#!/bin/bash

set -e

echo "Installing dependencies"
apt-get update
apt-get install -y --no-install-recommends \
    build-essential cmake git curl ca-certificates \
    libxtst-dev libxinerama-dev libx11-dev libxfixes-dev libavahi-client3

echo "Checking out git submodules"
git submodule update --init --recursive

echo "Installing NDI SDK"
pushd lib
ndi_release="Install_NDI_SDK_v6_Linux"
ndi_installer="${ndi_release}.tar.gz"
curl -o "$ndi_installer" -L "https://downloads.ndi.tv/SDK/NDI_SDK_Linux/${ndi_release}.tar.gz"
tar -xzf "$ndi_installer"
yes | PAGER=cat sh "./${ndi_release}.sh"
cp -a "NDI SDK for Linux/lib/x86_64-linux-gnu/lib"* "/usr/local/lib"
ldconfig
popd

echo "Building ndi-streamer"
cmake .
make
cp /usr/local/lib/libndi.so.6 build

echo "Build successful. See 'build' directory for output"
