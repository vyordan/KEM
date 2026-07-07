FROM archlinux:latest

# ── Dependencias del sistema ───────────────────────────────────────────────────
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
        clang \
        llvm \
        lld  \
        cmake \
        ninja \
        git && \
    pacman -Scc --noconfirm

# ── Copiar el proyecto ─────────────────────────────────────────────────────────
WORKDIR /kem
COPY . .

# ── Build ──────────────────────────────────────────────────────────────────────
RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DKEM_BUILD_TESTS=OFF && \
    cmake --build build

# ── Punto de entrada ───────────────────────────────────────────────────────────
# langs/ queda en /kem/build/cli/langs/ (copiado por el target copy_langs)
WORKDIR /kem/build/cli
ENTRYPOINT ["./kem"]
CMD ["--help"]

# ── Uso ────────────────────────────────────────────────────────────────────────
# docker build -t kem .
# docker run --rm -v $(pwd)/mi_programa.kem:/programa.kem kem /programa.kem
# docker run --rm -v $(pwd)/mi_programa.kem:/programa.kem kem --emit-tokens /programa.kem
