# Multi-stage Dockerfile for Ryazha-Status-Monitor cross-compilation

# Stage 1: Build environment with devkitPro
FROM ubuntu:22.04 AS builder

# Set working directory
WORKDIR /build

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    wget \
    git \
    tar \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Install devkitPro
RUN wget https://github.com/devkitPro/pacman-installer/releases/latest/download/devkitpro-pacman-installer.tar.gz && \
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

# Build project
RUN make switch

# Stage 2: Runtime image for testing
FROM ubuntu:22.04 AS runtime

# Install basic utilities
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    unzip \
    file \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /workspace

# Copy built artifacts from builder stage
COPY --from=builder /build/*.nro ./
COPY --from=builder /build/*.elf ./
COPY --from=builder /build/*.json ./
COPY --from=builder /build/README.md ./
COPY --from=builder /build/LICENSE ./

# Copy build scripts for runtime
COPY --from=builder /build/scripts/ ./scripts/
COPY --from=builder /build/Makefile ./

# Set environment for runtime
ENV DEVKITPRO=/opt/devkitpro
ENV PATH=$DEVKITPRO/tools/bin:$PATH

# Expose artifacts
RUN ls -la

# Default command
