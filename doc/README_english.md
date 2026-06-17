[![Docker Image Size](https://img.shields.io/docker/image-size/vyordan/kem?label=Tama%C3%B1o%20de%20la%20imagen)](https://hub.docker.com/r/vyordan/kem)
[![Docker Pulls](https://img.shields.io/docker/pulls/vyordan/kem?label=Descargas)](https://hub.docker.com/r/vyordan/kem)

<div align="center">
  <img src="doc/xd (1).png" alt="Extended KEM logo" width="250"/>

**A Programming Language That Speaks Your Language**

</div>

## Programming Language and JIT Compiler (C++/LLVM)

**Kem** (*"to weave"* in the K'iche' language) is a programming language with simple syntax, originally designed in Spanish, that executes natively through its own high-performance Just-In-Time (JIT) compiler.

The project is an integrated ecosystem that combines an accessible programming language with a low-level infrastructure built on **LLVM**.

* **Native JIT Compilation:** Takes source code and compiles it directly into machine code on the fly through LLVM.
* **Complete Linguistic Modularity:** Although the default syntax is Spanish, all language keywords are 100% configurable through an external JSON file. This allows the same compiler to be reused for mapping the language to any natural language or Mayan variant (such as Kaqchikel or K'iche').

---

## Requirements

```bash
# Arch Linux
sudo pacman -S llvm lld cmake ninja git
```

**Explanation**

This command installs all required development dependencies on Arch Linux:

* `llvm`: LLVM compiler infrastructure.
* `lld`: LLVM linker.
* `cmake`: Build system generator.
* `ninja`: Fast build tool.
* `git`: Version control system.

Minimum LLVM version: **17**. Tested with LLVM 22.

---

## Build

```bash
git clone https://github.com/vyordan/kem
cd kem

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Verificar que funciona
./build/cli/kem --help
```

**Explanation (English):**

* Clones the repository.
* Enters the project directory.
* Configures the project using CMake and the Ninja generator.
* Builds the project in Release mode.
* Finally, runs the executable with the `--help` option to verify that the compiler was built successfully.

---

## Usage

```bash
```bash
# Run a KEM program. Since no .json file from the langs/ directory is provided, the compiler assumes that programa.kem is written in Spanish.
./build/cli/kem programa.kem

# Use another language
./build/cli/kem --lang=langs/english.json programa.kem

# Display the generated LLVM IR
./build/cli/kem --emit-ir programa.kem

# Display the AST
./build/cli/kem --emit-ast programa.kem

# Display the generated tokens
./build/cli/kem --emit-tokens programa.kem

# Measure the execution time of each compilation phase
./build/cli/kem --benchmark programa.kem
```


**Explanation (English):**

This block demonstrates several ways to execute the compiler:

* Run a KEM source file.
* Load an alternative language configuration.
* Display the generated LLVM Intermediate Representation (IR).
* Print the generated Abstract Syntax Tree (AST).
* Display the lexer tokens.
* Benchmark each compilation phase.

---

## Example KEM Program

```
funcion fibonacci(entero n) entero {
    si n < 2 {
        devolver n
    }
    devolver fibonacci(n - 1) + fibonacci(n - 2)
}

inicio {
    entero resultado = fibonacci(10)
}
```

**Explanation (English):**

This program defines a recursive Fibonacci function.

* `funcion fibonacci(entero n) entero` declares a function that receives an integer and returns an integer.
* `si n < 2` checks the base case.
* `devolver n` returns the current value when `n` is less than 2.
* Otherwise, the function recursively computes the sum of the two previous Fibonacci numbers.
* The `inicio` block serves as the program entry point.
* A variable named `resultado` stores the value of `fibonacci(10)`.

---

## The Multi-Language System

KEM supports any human language through JSON files stored inside the `langs/` directory.

Each file maps language keywords to the compiler's internal `TokenType`.

```json
{
  "funcion":    "KW_FUNCION",
  "si":         "KW_SI",
  "devolver":   "KW_DEVOLVER",
  "entero":     "KW_ENTERO"
}
```

**Explanation (English):**

This JSON file defines how Spanish keywords are mapped to the compiler's internal token types.

For example:

* `funcion` maps to the function keyword token.
* `si` maps to the conditional keyword.
* `devolver` maps to the return statement.
* `entero` maps to the integer type.

The compiler internally uses token identifiers rather than language-specific words.

The same language configuration written in English:

```json
{
  "function":   "KW_FUNCION",
  "if":         "KW_SI",
  "return":     "KW_DEVOLVER",
  "int":        "KW_ENTERO"
}
```

**Explanation (English):**

This configuration demonstrates that the compiler can recognize English keywords while producing exactly the same internal tokens.

Only the external vocabulary changes; the parser and compiler implementation remain identical.

```bash
./kem --lang=langs/english.json programa.kem
```

**Explanation (English):**

This command runs the compiler using the English language definition file instead of the default Spanish configuration.

---

## Language Reference

### Data Types

| KEM        | LLVM IR  | Description           |
| ---------- | -------- | --------------------- |
| `entero`   | `i64`    | 64-bit integer        |
| `decimal`  | `double` | 64-bit floating point |
| `texto`    | `i8*`    | Character string      |
| `booleano` | `i1`     | Boolean value         |

### Functions and Procedures

```
funcion nombre(tipo param) tipo_retorno {
    devolver valor
}

procedimiento nombre(tipo param) {
    // no retorna valor
}
```

**Explanation (English):**

This example illustrates the difference between functions and procedures.

* `funcion` declares a function with a return type.
* `devolver` returns a value to the caller.
* `procedimiento` declares a routine that performs actions but does not return a value.

---

### Control Flow

```
si condicion {
    // ...
} sino si otra_condicion {
    // ...
} sino {
    // ...
}

mientras condicion {
    // ...
}

// Bucle con rango (var debe estar declarada antes)
entero i
i = 0 hasta 10 {
    // i va de 0 a 9
}

i = 0 hasta 100 paso 5 {
    // i: 0, 5, 10, 15, ...
}
```

**Explanation (English):**

This block demonstrates the language control structures.

* `si` represents an `if` statement.
* `sino si` corresponds to `else if`.
* `sino` represents `else`.
* `mientras` is a `while` loop.
* The `hasta` syntax defines a range-based loop.
* `paso` specifies the increment value.
* The variable used by the range loop must be declared beforehand.

---

### Arrays

```
entero nums[5] = [10, 20, 30, 40, 50]
entero x = nums[2]   // 30
nums[0] = 99
```

**Explanation (English):**

This example declares an integer array with five elements.

* The array is initialized with predefined values.
* The third element is read and assigned to variable `x`.
* The first element is later updated to `99`.

---

### Structures

```
estructura Punto {
    decimal x
    decimal y
}

Punto p
p.x = 1.0
p.y = 2.0
```

**Explanation (English):**

This code defines a structure named `Punto` with two floating-point fields.

An instance named `p` is created, and its `x` and `y` members are assigned values.

---

### C Interoperability

```
enlazar vacio printf(texto)
enlazar entero strlen(texto)

inicio {
    printf("Hola desde KEM!\n")
}
```

**Explanation (English):**

This example demonstrates interoperability with C functions.

* `enlazar` declares an external function.
* `printf` is imported as a procedure returning no value.
* `strlen` is imported as a function returning an integer.
* The `inicio` block calls `printf` to print a message.

---

### Comments

```
// Comentario de línea
/* Comentario
   de bloque */
comentario Esto también es un comentario
comentario{ Y esto
            también }
```

**Explanation (English):**

KEM supports multiple comment styles.

* `//` introduces a single-line comment.
* `/* ... */` defines a block comment.
* `comentario` provides an alternative single-line comment syntax.
* `comentario{ ... }` defines an alternative multi-line comment syntax.

---

## Running the Tests

```bash
cd build
ctest --output-on-failure

# O correr cada suite individualmente
./tests/lexer/kem_test_lexer
./tests/parser/kem_test_parser
./tests/semantic/kem_test_semantic
./tests/codegen/kem_test_codegen
./tests/integration/kem_test_integration
```

**Explanation (English):**

This block executes the project's automated test suite.

* Change into the build directory.
* Run all tests using CTest.
* Alternatively, execute each testing module independently, including lexer, parser, semantic analysis, code generation, and integration tests.

---

## Docker

```bash
# Construir la imagen
docker build -t kem .

# Ejecutar un programa
docker run --rm -v $(pwd)/mi_programa.kem:/programa.kem kem /programa.kem

# Ver el IR generado
docker run --rm -v $(pwd)/mi_programa.kem:/programa.kem kem --emit-ir /programa.kem
```

**Explanation (English):**

This block shows how to use Docker.

* Build the Docker image.
* Execute a KEM program inside a container.
* Display the generated LLVM IR from within the container.

---

## Compiler Architecture

```
archivo.kem
    │
    ▼
LangConfig ──── langs/espanol.json
    │
    ▼
Lexer ──────────── vector<Token>
    │
    ▼
Parser ─────────── unique_ptr<Program>  (AST)
    │
    ▼
SemanticAnalyzer ── AST anotado + tabla de símbolos
    │
    ▼
IRGenerator ────── llvm::Module
    │
    ▼
ORC JIT ─────────── código nativo x86-64
    │
    ▼
Ejecución
```

**Explanation (English):**

This diagram illustrates the compiler pipeline.

* A source file is processed.
* Language configuration loads keyword mappings.
* The lexer produces tokens.
* The parser constructs the AST.
* Semantic analysis annotates the AST and builds the symbol table.
* The IR generator creates LLVM IR.
* The ORC JIT compiles it into native x86-64 machine code.
* Finally, the program is executed.

---

## Complete Project Tree

```text
kem/
├── CMakeLists.txt                  ← root build file, assembles all sub-targets
├── README.md
├── Dockerfile
├── .gitignore
│
├── include/
│   └── kem/
│       ├── LangConfig.hpp          ← loads the language JSON and resolves keywords → TokenType
│       ├── Token.hpp               ← Token struct + TokenType enum (data only, no logic)
│       ├── Lexer.hpp               ← lexical analyzer (tokenizer)
│       ├── AST.hpp                 ← all AST nodes + Visitor interface
│       ├── Parser.hpp              ← recursive descent + Pratt parser
│       ├── SemanticAnalyzer.hpp    ← Visitor: performs type and scope checking
│       ├── IRGenerator.hpp         ← Visitor: AST → LLVM IR
│       ├── JITEngine.hpp           ← ORC JIT wrapper
│       └── ErrorHandler.hpp        ← errors with line, column, and localizable messages
│
├── src/
│   ├── LangConfig.cpp
│   ├── Lexer.cpp
│   ├── AST.cpp                     ← implementation of AST node methods (if any)
│   ├── Parser.cpp
│   ├── SemanticAnalyzer.cpp
│   ├── IRGenerator.cpp
│   └── JITEngine.cpp
│   └── ErrorHandler.cpp
│
├── cli/
│   ├── CMakeLists.txt              ← executable target: kem
│   └── main.cpp                    ← parses arguments, loads LangConfig, and runs the compilation pipeline
│
├── langs/                          ← interchangeable language files
│   ├── espanol.json                ← default language
│   └── english.json                ← demonstration of the multi-language system
│
├── examples/                       ← example KEM programs (also used in tests)
│   ├── hola.kem
│   ├── fibonacci.kem
│   ├── factorial.kem
│   ├── arreglos.kem
│   └── estructuras.kem
│
├── tests/
│   ├── CMakeLists.txt
│   ├── lexer/
│   │   ├── test_tokens.cpp         ← tokenizes known strings and verifies the output
│   │   └── test_comentarios.cpp
│   ├── parser/
│   │   ├── test_expresiones.cpp
│   │   ├── test_funciones.cpp
│   │   └── test_flujo.cpp
│   ├── semantic/
│   │   ├── test_tipos.cpp          ← expected semantic error tests
│   │   └── test_scopes.cpp
│   ├── codegen/
│   │   └── test_ir.cpp             ← verifies that the generated IR is valid
│   └── integration/
│       └── test_ejemplos.cpp       ← compiles and executes examples/*.kem and verifies the results
```


---

### Modules

| File               | Responsibility                                                        |
| ------------------ | --------------------------------------------------------------------- |
| `LangConfig`       | Loads the language JSON configuration                                 |
| `Lexer`            | Converts text into tokens                                             |
| `AST`              | Syntax tree nodes and Visitor interface                               |
| `Parser`           | Converts tokens into an AST (Recursive Descent + Pratt)               |
| `SemanticAnalyzer` | Performs type checking and scope validation                           |
| `IRGenerator`      | Generates LLVM IR using IRBuilder                                     |
| `JITEngine`        | Produces native code and executes it through ORC JIT                  |
| `ErrorHandler`     | Provides standardized error messages with line and column information |

---

## Adding a New Language

1. Copy `langs/espanol.json` using a new filename.
2. Replace the keywords with those of the target language.
3. Run:

`./kem --lang=langs/my_language.json programa.kem`

The compiler validates that the language file contains all required keywords during startup and emits a descriptive error if any mandatory keyword is missing.
