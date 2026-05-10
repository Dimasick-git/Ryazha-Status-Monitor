# Dockerfile for Ryazha-Status-Monitor with devkitPro

FROM ubuntu:22.04

WORKDIR /workspace

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    wget \
    git \
    tar \
    xz-utils \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install devkitPro using the official method
RUN wget https://github.com/devkitPro/pacman/releases/download/v2.1.1/devkitpro-pacman-installer.tar.gz && \
    tar -xzvf devkitpro-pacman-installer.tar.gz && \
    ./devkitpro-pacman-installer.sh --no-confirm

# Set environment variables
ENV DEVKITPRO=/opt/devkitpro
ENV PATH=$DEVKITPRO/tools/bin:$PATH

# Install Switch development tools and libraries
RUN dkp-pacman -Syu --noconfirm && \
    dkp-pacman -S \
    switch-devkitA64 \
    switch-tools \
    switch-libnx \
    switch-mesa \
    switch-glad \
    switch-libdrm_nouveau \
    switch-curl \
    switch-libjpeg-turbo \
    switch-libpng \
    switch-zlib \
    switch-bz2 \
    switch-libogg \
    switch-libvorbis \
    switch-libmpg123 \
    switch-opusfile \
    switch-ffmpeg \
    --noconfirm

# Copy source code
COPY . .

# Copy build scripts
COPY scripts/ ./scripts/
COPY Makefile ./

# Build project
RUN make switch

# Expose artifacts
RUN ls -la

# Default command
CMD ["ls", "-la"]
