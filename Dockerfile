# Emscripten Build Environment
# Pinned to 6.0.0 to match the xash3d-fwgs engine's emsdk — the ts gamedll is a
# SIDE_MODULE dlopen'd by the engine (MAIN_MODULE), so the emscripten dynamic-link
# ABI must match the engine's build (WEBCLIENT.md §5). Do not drift from the engine.
FROM emscripten/emsdk:6.0.0

ENV DEBIAN_FRONTEND=noninteractive

# emsdk 6.0.0 already bundles cmake 3.28.3 + emcmake + python3, so we only add
# git + a C/C++ host toolchain (build-essential) for CMake's compiler probes.
# (Do NOT pip-install cmake — the 6.0.0 base is PEP-668 externally-managed and it
# is redundant with the bundled cmake anyway.)
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    ca-certificates \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

CMD ["/bin/bash"]
