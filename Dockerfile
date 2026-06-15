FROM archlinux:latest

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
        llvm \
        lld  \
        clang \
        cmake \
        ninja \
        git && \
    pacman -Scc --noconfirm

# Forzar CMake a usar Clang
ENV CC=clang
ENV CXX=clang++

WORKDIR /kem
COPY . .

RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DKEM_BUILD_TESTS=OFF && \
    cmake --build build

WORKDIR /kem/build/cli
ENTRYPOINT ["./kem"]
CMD ["--help"]

# ── Uso ────────────────────────────────────────────────────────────────────────
# docker build -t kem .
# docker run --rm -v $(pwd)/mi_programa.kem:/programa.kem kem /programa.kem
# docker run --rm -v $(pwd)/mi_programa.kem:/programa.kem kem --emit-tokens /programa.kem
