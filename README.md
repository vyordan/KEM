<div align="center">
  <img src="doc/xd (1).png" alt="KEM logo extendido" width="250"/>
  
  **Un Lenguaje que habla tu Idioma**
</div>

## Lenguaje de Programación y Compilador JIT (C++/LLVM)

**Kem** (*"tejer"* en el idioma K'iche') es un lenguaje de programación de sintaxis sencilla diseñado originalmente en español, que se ejecuta de forma nativa a través de su propio compilador JIT (Just-In-Time) de alto rendimiento.

El proyecto es un ecosistema integrado que unifica un lenguaje accesible con una infraestructura de bajo nivel basada en **LLVM**.

* **Compilación JIT Nativa:** Toma el código fuente y lo compila directamente a código de máquina al vuelo vía LLVM, garantizando una ejecución rápida.
* **Modularidad Lingüística Absoluta:** Aunque la sintaxis por defecto es en español, el idioma de las palabras clave es 100% configurable mediante un archivo JSON externo. Esto permite reutilizar el mismo compilador para mapear el lenguaje a cualquier idioma natural o variante maya (como el Kaqchikel o K'iche').

---

## Requisitos

```bash
# Arch Linux
sudo pacman -S llvm lld cmake ninja git
```

Versión mínima de LLVM: **17**. Probado con LLVM 22.

---

## Compilar

```bash
git clone https://github.com/tu-usuario/kem
cd kem

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Verificar que funciona
./build/cli/kem --help
```

---

## Uso

```bash
# Ejecutar un programa KEM
./build/cli/kem programa.kem

# Usar otro idioma
./build/cli/kem --lang=langs/english.json programa.kem

# Ver el LLVM IR generado
./build/cli/kem --emit-ir programa.kem

# Ver el AST
./build/cli/kem --emit-ast programa.kem

# Ver los tokens
./build/cli/kem --emit-tokens programa.kem

# Medir tiempos de cada fase
./build/cli/kem --benchmark programa.kem
```

---

## Ejemplo de programa KEM

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

---

## El sistema multi-idioma

KEM soporta cualquier idioma humano mediante archivos JSON en `langs/`.
Cada archivo mapea las palabras clave del idioma al `TokenType` interno del compilador.

```json
{
  "funcion":    "KW_FUNCION",
  "si":         "KW_SI",
  "devolver":   "KW_DEVOLVER",
  "entero":     "KW_ENTERO"
}
```

El mismo programa escrito en inglés:

```json
{
  "function":   "KW_FUNCION",
  "if":         "KW_SI",
  "return":     "KW_DEVOLVER",
  "int":        "KW_ENTERO"
}
```

```bash
./kem --lang=langs/english.json programa.kem
```

---

## Referencia del lenguaje

### Tipos de datos

| KEM       | LLVM IR  | Descripción          |
|-----------|----------|----------------------|
| `entero`  | `i64`    | Entero de 64 bits    |
| `decimal` | `double` | Punto flotante 64b   |
| `texto`   | `i8*`    | Cadena de caracteres |
| `booleano`| `i1`     | Verdadero o falso    |

### Funciones y procedimientos

```
funcion nombre(tipo param) tipo_retorno {
    devolver valor
}

procedimiento nombre(tipo param) {
    // no retorna valor
}
```

### Flujo de control

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

### Arreglos

```
entero nums[5] = [10, 20, 30, 40, 50]
entero x = nums[2]   // 30
nums[0] = 99
```

### Estructuras

```
estructura Punto {
    decimal x
    decimal y
}

Punto p
p.x = 1.0
p.y = 2.0
```

### Interoperabilidad con C

```
enlazar vacio printf(texto)
enlazar entero strlen(texto)

inicio {
    printf("Hola desde KEM!\n")
}
```

### Comentarios

```
// Comentario de línea
/* Comentario
   de bloque */
comentario Esto también es un comentario
comentario{ Y esto
            también }
```

---

## Correr los tests

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

---

## Arquitectura del compilador

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

### Módulos

| Archivo                  | Responsabilidad                              |
|--------------------------|----------------------------------------------|
| `LangConfig`             | Carga el JSON de idioma                      |
| `Lexer`                  | Texto → lista de tokens                      |
| `AST`                    | Nodos del árbol sintáctico + Visitor         |
| `Parser`                 | Tokens → AST (Recursive Descent + Pratt)     |
| `SemanticAnalyzer`       | Verificación de tipos y scopes               |
| `IRGenerator`            | AST → LLVM IR vía IRBuilder                  |
| `JITEngine`              | IR → código nativo + ejecución (ORC JIT)     |
| `ErrorHandler`           | Mensajes de error uniformes con línea/col    |

---

## Agregar un nuevo idioma

1. Copiar `langs/espanol.json` con el nuevo nombre
2. Reemplazar las palabras clave por las del idioma objetivo
3. Ejecutar: `./kem --lang=langs/mi_idioma.json programa.kem`

El compilador valida que el archivo contenga todas las keywords
obligatorias al arrancar y lanza un error descriptivo si falta alguna.
