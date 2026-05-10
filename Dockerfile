# Simplified Dockerfile for Ryazha-Status-Monitor

FROM ubuntu:22.04

WORKDIR /workspace

# Install basic utilities
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    unzip \
    file \
    git \
    make \
    gcc \
    g++ \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . .

# Copy build scripts
COPY scripts/ ./scripts/
COPY Makefile ./

# Expose artifacts
RUN ls -la

# Default command
CMD ["ls", "-la"]
