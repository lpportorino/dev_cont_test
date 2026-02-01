# Single-stage Dev Container for Jettison WASM OSD C Development
# Combines all toolchains in one layer for CLion compatibility
# Works with both CLion IDE and devcontainer-build.sh CLI

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# =============================================================================
# All dependencies in one stage (avoids multi-stage caching issues with CLion)
# =============================================================================

# Base build tools and utilities
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    tar \
    xz-utils \
    git \
    make \
    pkg-config \
    ca-certificates \
    gnupg \
    lsb-release \
    software-properties-common \
    # C/C++ compilers for test harness
    gcc \
    g++ \
    # Code quality tools
    clang-format \
    clang-tidy \
    # Libraries
    libpng-dev \
    libpng16-16 \
    # GStreamer (for video generation)
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-tools \
    # Video processing
    ffmpeg \
    bc \
    # Deployment tools
    rsync \
    openssh-client \
    # Dev UX tools
    vim \
    nano \
    less \
    sudo \
    zsh \
    bash-completion \
    htop \
    tree \
    jq \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# WASI SDK 29 - WebAssembly compilation toolchain (LLVM 21.1)
# =============================================================================
ENV WASI_VERSION=29
ENV WASI_VERSION_FULL=${WASI_VERSION}.0
ENV WASI_SDK_PATH=/opt/wasi-sdk

RUN wget -q https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_VERSION}/wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz \
    && tar xzf wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz \
    && mv wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux ${WASI_SDK_PATH} \
    && rm wasi-sdk-${WASI_VERSION_FULL}-x86_64-linux.tar.gz

ENV PATH="${WASI_SDK_PATH}/bin:${PATH}"

# =============================================================================
# JSON validation libraries (for config schema validation)
# =============================================================================
RUN wget -q https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp \
    && mkdir -p /usr/local/include/nlohmann \
    && mv json.hpp /usr/local/include/nlohmann/

RUN git clone --depth 1 --branch 2.3.0 https://github.com/pboettch/json-schema-validator.git /tmp/json-schema-validator \
    && cp /tmp/json-schema-validator/src/nlohmann/json-schema.hpp /usr/local/include/nlohmann/ \
    && cp /tmp/json-schema-validator/src/json-patch.hpp /usr/local/include/nlohmann/ \
    && cp /tmp/json-schema-validator/src/smtp-address-validator.hpp /usr/local/include/ \
    && cp /tmp/json-schema-validator/src/json-uri.cpp /usr/local/src/ \
    && cp /tmp/json-schema-validator/src/json-validator.cpp /usr/local/src/ \
    && cp /tmp/json-schema-validator/src/json-schema-draft7.json.cpp /usr/local/src/ \
    && cp /tmp/json-schema-validator/src/string-format-check.cpp /usr/local/src/ \
    && cp /tmp/json-schema-validator/src/smtp-address-validator.cpp /usr/local/src/ \
    && cp /tmp/json-schema-validator/src/json-patch.cpp /usr/local/src/ \
    && rm -rf /tmp/json-schema-validator

# =============================================================================
# Wasmtime v41.0.1 - WebAssembly runtime + C API
# =============================================================================
ENV WASMTIME_VERSION=v41.0.1

RUN wget -q https://github.com/bytecodealliance/wasmtime/releases/download/${WASMTIME_VERSION}/wasmtime-${WASMTIME_VERSION}-x86_64-linux.tar.xz \
    && tar xf wasmtime-${WASMTIME_VERSION}-x86_64-linux.tar.xz \
    && mv wasmtime-${WASMTIME_VERSION}-x86_64-linux/wasmtime /usr/local/bin/ \
    && rm -rf wasmtime-${WASMTIME_VERSION}-x86_64-linux* \
    && chmod +x /usr/local/bin/wasmtime

RUN wget -q https://github.com/bytecodealliance/wasmtime/releases/download/${WASMTIME_VERSION}/wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api.tar.xz \
    && tar xf wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api.tar.xz \
    && cp -r wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api/include/* /usr/local/include/ \
    && cp -r wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api/lib/* /usr/local/lib/ \
    && rm -rf wasmtime-${WASMTIME_VERSION}-x86_64-linux-c-api* \
    && ldconfig

# =============================================================================
# Node.js 20.x - For TypeScript snapshot tool
# =============================================================================
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && npm install -g typescript ts-node \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Non-root user with sudo access
# =============================================================================
ARG USER_ID=1000
ARG GROUP_ID=1000
ARG USERNAME=vscode

RUN groupadd -g ${GROUP_ID} ${USERNAME} || true \
    && useradd -l -m -u ${USER_ID} -g ${GROUP_ID} -s /bin/bash ${USERNAME} || true \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# =============================================================================
# Workspace setup
# =============================================================================
WORKDIR /workspace
RUN chown -R ${USER_ID}:${GROUP_ID} /workspace

USER ${USERNAME}

# =============================================================================
# Git configuration (prevents CLion "Setting global Git config" hang)
# These are fallback values - host .gitconfig is mounted via devcontainer.json
# =============================================================================
RUN git config --global user.email "devcontainer@localhost" \
    && git config --global user.name "Dev Container User" \
    && git config --global init.defaultBranch main \
    && git config --global safe.directory "*"

# Pre-create JetBrains config directory (CLion expects this)
RUN sudo mkdir -p /.jbdevcontainer/config/JetBrains \
    && sudo chmod -R 777 /.jbdevcontainer

# Shell customization
RUN echo 'export PS1="\[\033[01;32m\]\u@devcontainer\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ "' >> ~/.bashrc \
    && echo 'alias ll="ls -lah"' >> ~/.bashrc \
    && echo 'alias gs="git status"' >> ~/.bashrc \
    && echo 'alias gd="git diff"' >> ~/.bashrc

# Environment
ENV WASI_SDK_PATH=/opt/wasi-sdk
ENV WASI_SDK_PREFIX=/opt/wasi-sdk
ENV PATH="${WASI_SDK_PATH}/bin:${PATH}"

# Default: show environment info
CMD ["bash", "-c", "echo '=== Jettison WASM OSD Dev Container ===' && echo && echo 'WASI SDK:' && ${WASI_SDK_PATH}/bin/clang --version | head -n1 && echo 'Wasmtime:' && wasmtime --version && echo 'Node.js:' && node --version && echo && echo 'Run: make help'"]
