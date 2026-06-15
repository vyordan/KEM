# Estructura de archivos — Proyecto KEM
> Versión 0.1 — pre-implementación

---

## Árbol completo

```
kem/
├── CMakeLists.txt                  ← build raíz, ensambla todos los sub-targets
├── README.md
├── Dockerfile
├── .gitignore
│
├── include/
│   └── kem/
│       ├── LangConfig.hpp          ← carga el JSON de idioma, resuelve keywords → TokenType
│       ├── Token.hpp               ← struct Token + enum TokenType (sin lógica, solo datos)
│       ├── Lexer.hpp               ← tokenizador
│       ├── AST.hpp                 ← todos los nodos del AST + interfaz Visitor
│       ├── Parser.hpp              ← recursive descent + Pratt
│       ├── SemanticAnalyzer.hpp    ← Visitor: verifica tipos y scopes
│       ├── IRGenerator.hpp         ← Visitor: AST → LLVM IR
│       ├── JITEngine.hpp           ← wrapper del ORC JIT
│       └── ErrorHandler.hpp        ← errores con línea, columna y mensaje localizable
│
├── src/
│   ├── LangConfig.cpp
│   ├── Lexer.cpp
│   ├── AST.cpp                     ← implementación de métodos de los nodos (si los hay)
│   ├── Parser.cpp
│   ├── SemanticAnalyzer.cpp
│   ├── IRGenerator.cpp
│   └── JITEngine.cpp
│   └── ErrorHandler.cpp
│
├── cli/
│   ├── CMakeLists.txt              ← target ejecutable: kem
│   └── main.cpp                    ← parsea args, carga LangConfig, corre el pipeline
│
├── langs/                          ← archivos de idioma intercambiables
│   ├── espanol.json                ← idioma por defecto
│   └── english.json                ← demostración del sistema multi-idioma
│
├── examples/                       ← programas KEM de ejemplo (también usados en tests)
│   ├── hola.kem
│   ├── fibonacci.kem
│   ├── factorial.kem
│   ├── arreglos.kem
│   └── estructuras.kem
│
├── tests/
│   ├── CMakeLists.txt
│   ├── lexer/
│   │   ├── test_tokens.cpp         ← tokeniza strings conocidos, verifica output
│   │   └── test_comentarios.cpp
│   ├── parser/
│   │   ├── test_expresiones.cpp
│   │   ├── test_funciones.cpp
│   │   └── test_flujo.cpp
│   ├── semantic/
│   │   ├── test_tipos.cpp          ← errores semánticos esperados
│   │   └── test_scopes.cpp
│   ├── codegen/
│   │   └── test_ir.cpp             ← verifica que el IR generado sea válido
│   └── integration/
│       └── test_ejemplos.cpp       ← compila y ejecuta examples/*.kem, verifica resultado
│
├── grpc/                           ← (futuro) servidor gRPC para usar KEM como servicio
└── web/                            ← (futuro) playground web
```

---

## Por qué cada decisión

### `Token.hpp` separado de `Lexer.hpp`
`Token` es un struct de datos puro — solo tiene `TokenType`, `lexeme`, `line` y `col`.
Si está dentro de `Lexer.hpp`, cualquier archivo que solo necesite el tipo `Token`
(el Parser, el AST, el ErrorHandler) tiene que incluir el Lexer completo.
Separado, las dependencias quedan limpias:

```
LangConfig  →  Token
Lexer       →  Token, LangConfig
Parser      →  Token, Lexer, AST
Semantic    →  AST
IRGenerator →  AST, LLVM headers
JITEngine   →  LLVM headers
```

### `SemanticAnalyzer` separado de `IRGenerator`
Ambos son Visitors sobre el AST, pero tienen responsabilidades distintas.
El Semántico puede fallar y reportar errores sin tocar LLVM.
El IRGenerator asume que el AST ya fue validado — eso simplifica el codegen
porque no necesita manejar casos de error de tipo.

### `langs/` en el raíz
No dentro de `src/` ni de `include/` porque no es código — es datos de configuración.
El Dockerfile los copia al contenedor. El `.gitignore` los incluye siempre
(a diferencia de build artifacts). Un usuario puede agregar su propio `miarabe.json`
sin tocar nada del compilador.

### `examples/` doble función
Son programas de demostración para el README y al mismo tiempo los fixtures
de los tests de integración. `test_ejemplos.cpp` compila cada `.kem` de esa carpeta,
lo ejecuta via JIT, y verifica que el resultado sea el esperado. Así no hay que
mantener dos conjuntos de código de prueba.

### `cli/CMakeLists.txt` propio
El ejecutable `kem` es solo un target — no es el compilador.
Separar su `CMakeLists.txt` significa que en el futuro `grpc/` y `web/` pueden
linkear la misma librería `libkem` sin depender del CLI.
La librería central se define en el `CMakeLists.txt` raíz como `libkem`.

---

## Dependencias entre targets (CMake)

```
libkem  (STATIC)
  ├── src/LangConfig.cpp
  ├── src/Lexer.cpp
  ├── src/AST.cpp
  ├── src/Parser.cpp
  ├── src/SemanticAnalyzer.cpp
  ├── src/IRGenerator.cpp
  ├── src/JITEngine.cpp
  └── src/ErrorHandler.cpp

kem  (EXECUTABLE)  →  libkem
  └── cli/main.cpp

kem_tests  (EXECUTABLE)  →  libkem, GTest
  └── tests/**/*.cpp
```

Cuando en el futuro lleguemos a `grpc/`, solo se hace:
```cmake
add_executable(kem_server grpc/server.cpp)
target_link_libraries(kem_server PRIVATE libkem)
```
Sin tocar nada del compilador.

---

## Flags de invocación del CLI

```bash
# Ejecutar un archivo con el idioma por defecto
./kem programa.kem

# Especificar idioma
./kem --lang=langs/english.json programa.kem

# Emitir el IR en lugar de ejecutar (debug / benchmarks)
./kem --emit-ir programa.kem

# Emitir el AST en formato texto (debug)
./kem --emit-ast programa.kem

# Modo REPL interactivo
./kem --repl

# Medir tiempos de cada fase (para gráficas de tesis)
./kem --benchmark programa.kem
```

---

## `.gitignore` relevante

```gitignore
# Build
build/
cmake-build-*/
*.o
*.a

# IDE
.vscode/
.idea/
compile_commands.json

# OS
.DS_Store

# KEM runtime (no commitear binarios)
kem
kem_tests
```

---

## Dockerfile (esquema)

```dockerfile
FROM archlinux:latest

RUN pacman -Syu --noconfirm \
    llvm clang lld cmake ninja git

WORKDIR /kem
COPY . .

RUN cmake -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build

ENTRYPOINT ["./build/cli/kem"]
CMD ["--help"]
```

> El contenedor copia `langs/` junto con el código, así `espanol.json`
> está disponible dentro del contenedor sin ningún volumen extra.

---