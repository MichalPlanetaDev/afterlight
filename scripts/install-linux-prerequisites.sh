#!/usr/bin/env bash
set -Eeuo pipefail

sudo apt-get update

sudo apt-get install -y \
    autoconf \
    automake \
    build-essential \
    ca-certificates \
    clang \
    curl \
    git \
    libegl1-mesa-dev \
    libgl1-mesa-dev \
    libtool \
    libudev-dev \
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
    ninja-build \
    pkg-config \
    tar \
    unzip \
    zip
