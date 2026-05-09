FROM ubuntu:24.04

ARG CMAKE_VERSION=4.3.0

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH=/usr/local/bin:/usr/lib/llvm-23/bin:${PATH}
ENV CC=/usr/lib/llvm-23/bin/clang
ENV CXX=/usr/lib/llvm-23/bin/clang++

RUN apt-get update && apt-get install -y \
    ca-certificates \
    wget \
    gnupg \
    lsb-release \
    software-properties-common \
    build-essential \
    ninja-build \
    pkg-config \
    git \
    curl \
    zip \
    unzip \
    tar \
    liburing-dev \
    autoconf \
    autoconf-archive \
    automake \
    libtool \
    python3 \
    python3-pip && \
    rm -rf /var/lib/apt/lists/*

RUN wget -q https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 23 all && \
    apt-get update && \
    apt-get install -y libc++-23-dev libc++abi-23-dev && \
    pip3 install --break-system-packages "cmake==${CMAKE_VERSION}" && \
    rm -f llvm.sh && \
    rm -rf /var/lib/apt/lists/*

RUN cmake --version && \
    /usr/lib/llvm-23/bin/clang --version && \
    /usr/lib/llvm-23/bin/clang++ --version && \
    /usr/lib/llvm-23/bin/clang-scan-deps --version && \
    /usr/lib/llvm-23/bin/clang-format --version && \
    test -f /usr/lib/llvm-23/share/libc++/v1/std.compat.cppm

WORKDIR /workspace
