#!/usr/bin/env bash
set -Eeuo pipefail

sudo env DEBIAN_FRONTEND=noninteractive \
    apt-get update -qq

sudo env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y -qq \
        autoconf \
        automake \
        build-essential \
        ca-certificates \
        clang \
        clang-format \
        clang-tidy \
        curl \
        git \
        libegl1-mesa-dev \
        libgl1-mesa-dev \
        libtool \
        libudev-dev \
        libvulkan1 \
        libx11-dev \
        libx11-xcb-dev \
        libxcursor-dev \
        libxext-dev \
        libxfixes-dev \
        libxft-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbcommon-dev \
        libxkbfile-dev \
        libxpresent-dev \
        libxrandr-dev \
        libxrender-dev \
        libxss-dev \
        libxtst-dev \
        libxxf86vm-dev \
        mesa-vulkan-drivers \
        ninja-build \
        pkg-config \
        python3 \
        shellcheck \
        tar \
        unzip \
        vulkan-tools \
        vulkan-validationlayers \
        xauth \
        xvfb \
        zip
