#!/bin/bash

echo -e "\e[1;36m holosis download sdk script v0.1 \e[0m"

usage() {
    echo "Usage: $0 <sdk-rootdir>"
    exit 1
}

connection_check() {
    echo -e "\e[1;33mChecking if we're online...\e[0m"
    ping -c 1 -W 5 8.8.8.8 > /dev/null
    if [ ! $? -eq 0 ]; then
        echo -e "\e[1;31mPlease connect to the Internet to download packages.\e[0m"
        exit 1
    fi
}

download_and_verify() {
    local url="$1"
    local checksum="$2"
    local filename="$3"

    local filepath="$PKG_DIR/$filename"

    if [ -f "$filepath" ]; then
        echo "File $filename already exists. Verifying checksum..."
        echo "$checksum $filepath" | sha256sum -c --quiet
        if [ $? -eq 0 ]; then
            echo "Checksum verified. Skipping download for $filename."
            return
        else
            echo "Checksum mismatch. Redownloading $filename..."
            rm -f "$filepath"
        fi
    fi

    echo "Downloading $filename..."
    wget "$url" -O "$filepath" || { echo "Failed to download $filename"; exit 1; }

    echo "Verifying checksum for $filename..."
    echo "$checksum $filepath" | sha256sum -c --quiet
    if [ $? -eq 0 ]; then
        echo "Checksum verified for $filename."
    else
        echo "Checksum verification failed for $filename!"
        rm -f "$filepath"
        exit 1
    fi
}

clone_git_commit() {
    local url="$1"
    local commithash="$2"
    local gitroot="$3"

    git clone $url
    cd $gitroot
    git checkout $commithash
}

WORKING_DIR=$1
mkdir -p ${WORKING_DIR}

SOLIDRUN_SDK=(
    "https://github.com/SolidRun/imx8mp_build.git"
    "02852f8c3c1d693edd8fc6c63e73a82601f60469"
    "imx8mp_build"
)

#TOOLCHAIN=(
#    "https://developer.nvidia.com/embedded/jetson-linux/bootlin-toolchain-gcc-93"
#    "7725b4603193a9d3751d2715ef242bd16ded46b4e0610c83e76d8891cf580975"
#    "aarch64--glibc--stable-final.tar.gz"
#)

connection_check

echo "Entering sdk directory..."
cd "$WORKING_DIR"
clone_git_commit "${SOLIDRUN_SDK[@]}"
cp ./build-sdk.sh $WORKING_DIR
#download_and_verify "${TOOLCHAIN[@]}"
