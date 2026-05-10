# Multi-stage Dockerfile for Ryazha-Status-Monitor cross-compilation

# Stage 1: Build environment
FROM devkitpro/pacman-base:latest AS builder

# Set working directory
WORKDIR /build

# Install Switch development tools and libraries
RUN sudo dkp-pacman -Syu && \
    sudo dkp-pacman -S \
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
    switch-ffmpeg

# Copy source code
COPY . .

# Build the project
RUN make switch

# Stage 2: Final image with artifacts
FROM ubuntu:22.04 AS final

# Install basic utilities
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Create output directory
WORKDIR /output

# Copy built artifacts from builder stage
COPY --from=builder /build/*.nro ./
COPY --from=builder /build/*.elf ./
COPY --from=builder /build/*.json ./
COPY --from=builder /build/README.md ./

# Expose artifacts
RUN ls -la

# Default command
CMD ["ls", "-la"]
