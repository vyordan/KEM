# KEM — Documentación Completa del Lenguaje

KEM es un lenguaje de programación compilado JIT con sintaxis en español.
El compilador toma archivos `.kem`, los analiza, verifica tipos, genera
LLVM IR y ejecuta el resultado directamente en la CPU — sin archivos intermedios.

El idioma de las palabras clave es configurable mediante un archivo JSON,
lo que permite usar el mismo compilador en cualquier idioma natural.

---

## Índice

1. [Estructura de un programa](#1-estructura-de-un-programa)
2. [Tipos de datos](#2-tipos-de-datos)
3. [Variables](#3-variables)
4. [Operadores](#4-operadores)
5. [Flujo de control](#5-flujo-de-control)
6. [Funciones](#6-funciones)
7. [Procedimientos](#7-procedimientos)
8. [Arreglos](#8-arreglos)
9. [Estructuras](#9-estructuras)
10. [Funciones de consola](#10-funciones-builtin-de-consola)
11. [Interoperabilidad con C](#11-interoperabilidad-con-c-enlazar)
12. [Comentarios](#12-comentarios)
13. [Reglas de continuación de línea](#13-reglas-de-continuación-de-línea)
14. [El sistema multi-idioma](#14-el-sistema-multi-idioma)
15. [Qué se puede hacer](#15-qué-se-puede-hacer)
16. [Qué NO se puede hacer todavía](#16-qué-no-se-puede-hacer-todavía)
17. [Mensajes de error](#17-mensajes-de-error)
18. [Referencia rápida de keywords](#18-referencia-rápida-de-keywords)

---

## 1. Estructura de un programa

Todo programa KEM tiene exactamente un bloque `inicio { }` que es el punto
de entrada — el equivalente de `main` en C. Las funciones, procedimientos
y estructuras se declaran fuera de `inicio`, antes o después de él.

```
// Declaraciones opcionales fuera de inicio
funcion suma(entero a, entero b) entero {
    devolver a + b
}

// Punto de entrada obligatorio
inicio {
    entero resultado = suma(3, 4)
}
```

**Reglas:**
- Debe haber exactamente un bloque `inicio { }` por archivo
- Las funciones pueden llamarse entre sí sin importar el orden de declaración
  (el compilador hace una pasada de registro antes de analizar los cuerpos)
- No existe un concepto de módulos o imports entre archivos en esta versión

---

## 2. Tipos de datos

KEM tiene cuatro tipos primitivos y sus variantes en arreglo.

| Keyword KEM | Tipo LLVM IR | Descripción                        | Ejemplo         |
|-------------|-------------|-------------------------------------|-----------------|
| `entero`    | `i64`       | Entero con signo de 64 bits         | `42`, `-7`, `0` |
| `decimal`   | `double`    | Punto flotante de 64 bits (IEEE 754)| `3.14`, `-0.5`  |
| `booleano`  | `i1`        | Verdadero o falso                   | `verdadero`, `falso` |
| `texto`     | `i8*`       | Puntero a cadena de caracteres      | `"hola mundo"`  |

### Conversiones implícitas

KEM realiza una sola conversión implícita: **entero → decimal** cuando
una operación mezcla los dos tipos. En todos los demás casos los tipos
deben coincidir exactamente.

```
decimal x = 5       // OK: 5 (entero) se convierte a 5.0 (decimal)
decimal y = 3 + 1.5 // OK: 3 se convierte a 3.0, resultado es 4.5
entero z = 3.14     // ERROR: decimal no se convierte a entero automáticamente
```

### No existe conversión explícita todavía

No hay una función de cast como `(entero)x`. Si necesitás convertir
decimal a entero, usá una función auxiliar enlazada de la libc:

```
enlazar entero llrint(decimal)  // redondea decimal → entero

inicio {
    decimal pi = 3.14
    entero aprox = llrint(pi)   // aprox = 3
}
```

---

## 3. Variables

### Declaración

La sintaxis es **tipo primero**, luego el nombre:

```
entero x           // sin inicializador — vale 0 por defecto
entero y = 10
decimal pi = 3.14159
booleano activo = verdadero
texto nombre = "KEM"
```

**El inicializador es opcional.** Sin él, la variable se inicializa a
cero (`0`, `0.0`, `falso`, o puntero nulo según el tipo).

### Asignación

```
x = 20
pi = 2.71828
activo = falso
```

### Scope

Las variables pertenecen al bloque `{ }` donde se declaran. No son
visibles fuera de ese bloque. Los bloques anidados pueden acceder
a variables del bloque exterior.

```
inicio {
    entero x = 10

    si verdadero {
        entero y = 20
        x = x + y    // OK: x es visible desde el bloque exterior
    }

    // y no existe aquí — error semántico si intentás usarla
}
```

### Redeclaración

No se puede declarar la misma variable dos veces en el mismo scope:

```
inicio {
    entero x = 5
    entero x = 10   // ERROR: 'x' ya fue declarado en este scope
}
```

En scopes distintos sí está permitido:

```
inicio {
    si verdadero { entero x = 1 }   // OK
    si verdadero { entero x = 2 }   // OK — scope distinto
}
```

---

## 4. Operadores

### Aritméticos

| Operador | Descripción         | Tipos válidos           |
|----------|---------------------|-------------------------|
| `+`      | Suma                | entero, decimal         |
| `-`      | Resta               | entero, decimal         |
| `*`      | Multiplicación      | entero, decimal         |
| `/`      | División            | entero, decimal         |
| `%`      | Módulo (resto)      | entero únicamente       |

**División de enteros:** es división entera — `7 / 2` da `3`, no `3.5`.
Para obtener `3.5` se necesita al menos un operando decimal: `7.0 / 2`.

### Relacionales — retornan booleano

| Operador | Descripción      |
|----------|-----------------|
| `==`     | Igual a         |
| `!=`     | Diferente de    |
| `<`      | Menor que       |
| `>`      | Mayor que       |
| `<=`     | Menor o igual   |
| `>=`     | Mayor o igual   |

Los operadores relacionales funcionan sobre `entero`, `decimal` y `booleano`.
No se pueden comparar valores de tipos incompatibles.

### Lógicos — sobre booleanos

| Operador | Descripción          | Equivalente C |
|----------|---------------------|---------------|
| `y`      | AND lógico          | `&&`          |
| `o`      | OR lógico           | `\|\|`        |
| `no`     | NOT lógico (unario) | `!`           |

`y` y `o` requieren que **ambos operandos sean booleanos**.
No existe evaluación cortocircuito en esta versión.

```
booleano a = verdadero
booleano b = falso
booleano c = a y b       // falso
booleano d = a o b       // verdadero
booleano e = no a        // falso
booleano f = 5 > 3 y 2 < 4   // verdadero
```

### Precedencia (de menor a mayor)

| Nivel | Operadores         |
|-------|--------------------|
| 1     | `o`                |
| 2     | `y`                |
| 3     | `==`, `!=`         |
| 4     | `<`, `>`, `<=`, `>=` |
| 5     | `+`, `-`           |
| 6     | `*`, `/`, `%`      |
| 7     | `no`, `-` (unario) |
| 8     | `.`, `[]`, `()`    |

Los paréntesis anulan la precedencia:

```
entero a = 2 + 3 * 4     // 14  (3*4 primero)
entero b = (2 + 3) * 4   // 20  (paréntesis primero)
```

### Negación unaria

```
entero x = -5
decimal y = -3.14
entero z = -(2 + 3)     // -5
```

---

## 5. Flujo de control

### si / sino

```
si condicion {
    // se ejecuta si condicion es verdadero
}

si condicion {
    // rama verdadera
} sino {
    // rama falsa
}

si condicion1 {
    // ...
} sino si condicion2 {
    // ...
} sino {
    // ...
}
```

La **condición debe ser de tipo booleano**. No se puede usar un entero
directamente como condición (a diferencia de C):

```
entero x = 5
si x { }          // ERROR: la condición del 'si' debe ser booleana
si x != 0 { }    // OK
```

### mientras

Loop universal. Se ejecuta mientras la condición sea verdadera.
Si la condición es falsa desde el principio, el cuerpo no se ejecuta nunca.

```
entero i = 0
mientras i < 10 {
    i = i + 1
}

// Loop infinito (con condición siempre verdadera)
mientras verdadero {
    // cuidado: no hay break en esta versión
}
```

La condición debe ser booleana — misma restricción que `si`.

### hasta / paso

Equivalente al `for` de otros lenguajes. La variable de iteración
**debe estar declarada antes** del bucle.

```
entero i           // declarar antes
i = 0 hasta 10 {
    // i toma valores: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    // (hasta 10 exclusivo)
}
```

Con paso explícito:

```
entero i
i = 0 hasta 20 paso 2 {
    // i: 0, 2, 4, 6, 8, 10, 12, 14, 16, 18
}

i = 10 hasta 0 paso -1 {
    // i: 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
}
```

**Sin paso** el incremento por defecto es 1.

**La variable de iteración es de tipo entero.** No se puede usar decimal
como variable de iteración.

**Límites:** el rango es `[inicio, fin)` — el valor `fin` nunca se alcanza.
Esto es idéntico al comportamiento de `range()` en Python.

### No existe break ni continue

En esta versión no hay instrucciones de salida temprana de loops.
Para simular `break`, usá una variable booleana como condición:

```
booleano encontrado = falso
entero i
i = 0 hasta 100 {
    si nums[i] == objetivo {
        encontrado = verdadero
    }
    // sin break — el loop sigue pero no hace más trabajo útil
    // esto es una limitación a resolver en versiones futuras
}
```

---

## 6. Funciones

Una `funcion` siempre retorna un valor. Debe declarar su tipo de retorno
y contener al menos un `devolver` en todos los caminos de ejecución.

```
funcion nombre(tipo param1, tipo param2) tipo_retorno {
    devolver valor
}
```

### Ejemplos

```
funcion suma(entero a, entero b) entero {
    devolver a + b
}

funcion max(entero a, entero b) entero {
    si a > b {
        devolver a
    }
    devolver b
}

funcion factorial(entero n) entero {
    si n == 0 {
        devolver 1
    }
    devolver n * factorial(n - 1)
}

funcion esPar(entero n) booleano {
    devolver n % 2 == 0
}
```

### Llamadas

```
entero r = suma(3, 4)
booleano par = esPar(10)
entero f = factorial(factorial(3))   // llamadas anidadas OK
```

### Recursión

Las funciones pueden llamarse a sí mismas recursivamente sin ninguna
declaración especial. También pueden llamarse entre sí (recursión mutua)
porque el compilador registra todas las signaturas antes de analizar los cuerpos.

### Parámetros por referencia

Con `referencia`, la función puede modificar la variable original del llamador:

```
procedimiento incrementar(referencia entero x) {
    x = x + 1
}

inicio {
    entero contador = 0
    incrementar(contador)
    // contador ahora vale 1
}
```

### Restricciones de funciones

- Una `funcion` **siempre** debe retornar un valor — el analizador semántico
  verifica que haya `devolver` en todos los caminos posibles
- El tipo del valor retornado debe coincidir con el tipo declarado
  (o ser convertible implícitamente entero→decimal)
- No se pueden declarar funciones dentro de otras funciones
- No hay funciones anónimas ni lambdas

---

## 7. Procedimientos

Un `procedimiento` no retorna valor. Es el equivalente de funciones `void` en C.

```
procedimiento nombre(tipo param) {
    // cuerpo
}
```

### Ejemplos

```
procedimiento saludar(texto nombre) {
    printf(nombre)
}

procedimiento resetear(referencia entero x) {
    x = 0
}

procedimiento intercambiar(referencia entero a, referencia entero b) {
    entero temp = a
    a = b
    b = temp
}
```

### devolver en procedimientos

Se puede usar `devolver` sin valor en un procedimiento para salir
anticipadamente (equivalente a `return;` en C):

```
procedimiento procesar(entero x) {
    si x < 0 {
        devolver    // salida temprana
    }
    // ... resto del procesamiento
}
```

---

## 8. Arreglos

KEM soporta arreglos **unidimensionales** de tipo fijo. El tamaño se
declara en tiempo de compilación y no puede cambiar.

### Declaración

```
entero nums[5]                      // arreglo de 5 enteros, inicializados a 0
entero primos[6] = [2, 3, 5, 7, 11, 13]
decimal puntos[3] = [1.0, 2.5, 3.7]
booleano flags[4] = [verdadero, falso, verdadero, verdadero]
```

### Acceso y modificación

Los índices empiezan en **0**:

```
entero primero = primos[0]   // 2
entero tercero = primos[2]   // 5
primos[0] = 99               // modificar
```

El índice debe ser de tipo `entero`. No hay verificación de límites
en tiempo de ejecución — acceder fuera del rango es comportamiento
indefinido (igual que en C).

### Arreglos en funciones

Los arreglos se declaran localmente dentro de funciones. No se pueden
pasar arreglos completos como argumentos en esta versión — solo
elementos individuales.

```
funcion suma_arreglo() entero {
    entero nums[5] = [1, 2, 3, 4, 5]
    entero suma = 0
    entero i
    i = 0 hasta 5 {
        suma = suma + nums[i]
    }
    devolver suma
}
```

### Restricciones de arreglos

- El tamaño debe ser un **literal entero positivo** conocido en tiempo de compilación
- No hay arreglos dinámicos (sin `new` ni `malloc`)
- No hay arreglos multidimensionales — solo unidimensionales
- No se pueden pasar arreglos como parámetros directamente

---

## 9. Estructuras

Las estructuras agrupan campos de distintos tipos bajo un nombre.

```
estructura Nombre {
    tipo campo1
    tipo campo2
}
```

### Ejemplo

```
estructura Punto {
    decimal x
    decimal y
}

estructura Persona {
    texto nombre
    entero edad
    booleano activo
}
```

### Uso

```
Punto origen
origen.x = 0.0
origen.y = 0.0

Punto p
p.x = 3.0
p.y = 4.0

decimal distancia = p.x * p.x + p.y * p.y
```

### Estado actual de las estructuras

El soporte de estructuras en esta versión es **parcial**:

- Se pueden declarar estructuras y acceder a sus campos con `.`
- El sistema de tipos de usuario aún no está completamente integrado
  con el analizador semántico — el compilador acepta la sintaxis pero
  no verifica que los campos existan ni sus tipos
- No se pueden pasar estructuras como parámetros por valor
- No se pueden usar como tipo de retorno de funciones

Las estructuras están diseñadas para ser completadas en la siguiente fase.

---

## 10. Funciones builtin de consola
(ESTA FUNCION QUIZA SEA QUITADA MAS ADELANTE Y SE INCLUYA CUANDO YA TENGAMOS UNA LIBRERIA ESTADAR)

KEM incluye funciones de entrada/salida integradas directamente en el
compilador. No requieren `enlazar` ni ninguna declaracion especial.
Estan disponibles en cualquier parte del programa sin importar nada.

### Salida

```
imprimir(texto)          // imprime el texto sin salto de linea al final
imprimirLinea(texto)     // imprime el texto con salto de linea al final
imprimirEntero(entero)   // imprime un entero en formato decimal
imprimirDecimal(decimal) // imprime un decimal con 6 cifras decimales
```

**Ejemplos:**

```
inicio {
    imprimir("Hola, ")
    imprimirLinea("mundo!")          // salida: Hola, mundo!

    imprimirEntero(42)               // salida: 42
    imprimirEntero(-7)               // salida: -7

    imprimirDecimal(3.14159)         // salida: 3.141590
    imprimirDecimal(2.0)             // salida: 2.000000

    imprimir("resultado = ")
    imprimirEntero(3 + 4 * 2)        // salida: resultado = 11
}
```

Se pueden combinar para formatear salida compleja:

```
funcion mostrar(texto etiqueta, entero valor) {
    // procedimiento de salida
}
// Ojo — lo de arriba deberia ser procedimiento, ejemplo:

procedimiento mostrar(texto etiqueta, entero valor) {
    imprimir(etiqueta)
    imprimir(": ")
    imprimirEntero(valor)
    imprimirLinea("")
}

inicio {
    mostrar("resultado", 42)         // salida: resultado: 42
}
```

### Entrada

```
leerLinea()   → texto    // lee una linea completa de stdin
leerEntero()  → entero   // lee un numero entero de stdin
leerDecimal() → decimal  // lee un numero decimal de stdin
```

**Ejemplos:**

```
inicio {
    imprimir("Ingresa tu nombre: ")
    texto nombre = leerLinea()
    imprimir("Hola, ")
    imprimirLinea(nombre)

    imprimir("Ingresa un numero: ")
    entero n = leerEntero()
    imprimir("El doble es: ")
    imprimirEntero(n * 2)
    imprimirLinea("")

    imprimir("Ingresa un decimal: ")
    decimal d = leerDecimal()
    imprimirDecimal(d * d)
}
```

### Uso dentro de funciones y loops

Los builtins funcionan en cualquier contexto — dentro de funciones,
procedimientos, loops y condicionales:

```
procedimiento imprimirTabla(entero n) {
    entero i
    i = 1 hasta 11 {
        imprimir("  ")
        imprimirEntero(n)
        imprimir(" x ")
        imprimirEntero(i)
        imprimir(" = ")
        imprimirEntero(n * i)
        imprimirLinea("")
    }
}

inicio {
    imprimir("Tabla del: ")
    entero n = leerEntero()
    imprimirTabla(n)
}
```

### Restricciones

- `imprimir` e `imprimirLinea` solo aceptan `texto` — para imprimir
  numeros usar `imprimirEntero` o `imprimirDecimal`
- `leerEntero` y `leerDecimal` no validan la entrada — si el usuario
  escribe letras el comportamiento es indefinido
- `imprimirDecimal` siempre muestra 6 cifras decimales

---

## 11. Interoperabilidad con C (`enlazar`)

`enlazar` declara una función externa implementada en C (o cualquier
librería del sistema) que el JIT resuelve en tiempo de ejecución.

```
enlazar tipo_retorno nombre_funcion(tipo1, tipo2, ...)
```

El JIT busca el símbolo automáticamente en las librerías cargadas
en memoria (libc, libm, etc.) — no hay que especificar de dónde viene.

### Funciones de la libc disponibles

```
enlazar vacio printf(texto)
enlazar entero puts(texto)
enlazar entero strlen(texto)
enlazar decimal sin(decimal)
enlazar decimal cos(decimal)
enlazar decimal sqrt(decimal)
enlazar decimal pow(decimal, decimal)
enlazar entero abs(entero)
enlazar vacio exit(entero)
enlazar entero llrint(decimal)     // decimal → entero (redondeo)
```

### Ejemplo completo con printf

```
enlazar vacio printf(texto)

funcion fibonacci(entero n) entero {
    si n < 2 { devolver n }
    devolver fibonacci(n - 1) + fibonacci(n - 2)
}

inicio {
    printf("Calculando fibonacci(10)...\n")
    entero r = fibonacci(10)
    printf("Resultado calculado\n")
}
```

### Limitación importante: printf variádico

`printf` en C acepta número variable de argumentos (`printf("%d", x)`).
KEM aún no soporta funciones variádicas. Solo se puede usar `printf`
con un string literal — no se pueden formatear números directamente.

Para imprimir números, usá `puts` combinado con conversiones, o
declaraciones separadas:

```
// Esto NO funciona (variádico):
// printf("%d\n", resultado)

// Esto SÍ funciona (string fijo):
enlazar vacio printf(texto)
inicio {
    printf("El programa termino\n")
}
```

---

## 11. Comentarios

KEM soporta cuatro estilos de comentarios, todos ignorados por el compilador:

```
// Comentario de línea estilo C/C++

/* Comentario de bloque
   multilínea estilo C */

comentario Esto también es un comentario de línea

comentario{
    Esto es un bloque
    de comentario
    estilo KEM
}
```

Los comentarios `comentario` y `comentario{ }` son la forma nativa
del lenguaje — pensados para hispanohablantes que prefieren no
recordar `//` y `/* */`.

---

## 12. Reglas de continuación de línea

KEM **no usa punto y coma**. El compilador determina el fin de una
sentencia por el salto de línea, con estas excepciones donde la línea
continúa en la siguiente:

1. La línea termina en un operador binario: `+`, `-`, `*`, `/`, `%`,
   `==`, `!=`, `<`, `>`, `<=`, `>=`, `y`, `o`, `=`, `,`
2. Hay paréntesis `( )` o corchetes `[ ]` abiertos sin cerrar
3. La línea termina en `{`

```
// Esto es una sola expresión (termina en operador):
entero resultado = 1 +
                   2 +
                   3

// Esto es una sola expresión (paréntesis abierto):
entero suma = (a +
               b +
               c)

// Estas son DOS sentencias separadas:
entero x = 5
entero y = 10
```

---

## 13. El sistema multi-idioma

El compilador carga un archivo JSON que mapea palabras del idioma
elegido a los tipos de token internos. Esto permite que el mismo
compilador procese código en cualquier idioma natural.

```bash
./kem programa.kem                          # español (por defecto)
./kem --lang=langs/english.json prog.kem    # inglés
```

### Crear un archivo de idioma

Copiar `langs/espanol.json` y reemplazar las palabras:

```json
{
  "_nombre": "mi_idioma",

  "funcion":       "KW_FUNCION",
  "procedimiento": "KW_PROC",
  "devolver":      "KW_DEVOLVER",
  "inicio":        "KW_INICIO",
  "si":            "KW_SI",
  "sino":          "KW_SINO",
  "mientras":      "KW_MIENTRAS",
  "hasta":         "KW_HASTA",
  "paso":          "KW_PASO",
  "entero":        "KW_ENTERO",
  "decimal":       "KW_DECIMAL",
  "texto":         "KW_TEXTO",
  "booleano":      "KW_BOOLEANO",
  "verdadero":     "KW_VERDADERO",
  "falso":         "KW_FALSO",
  "y":             "KW_Y",
  "o":             "KW_O",
  "no":            "KW_NO",
  "estructura":    "KW_ESTRUCTURA",
  "referencia":    "KW_REF",
  "enlazar":       "KW_ENLAZAR"
}
```

El compilador valida que estén presentes las 21 keywords obligatorias
y lanza un error descriptivo si falta alguna.

---

## 14. Qué se puede hacer

Lista completa de lo que KEM soporta en esta versión:

### Tipos y valores
- Literales enteros: `0`, `42`, `-7`, `1000000`
- Literales decimales: `3.14`, `-0.5`, `1.0`
- Literales booleanos: `verdadero`, `falso`
- Literales de texto: `"hola"`, `""`, `"con\nnewline"`
- Secuencias de escape en strings: `\n`, `\t`, `\"`, `\\`

### Operaciones
- Aritmética completa sobre enteros y decimales
- Conversión implícita entero → decimal en operaciones mixtas
- Comparaciones entre tipos compatibles
- Lógica booleana con `y`, `o`, `no`
- Negación unaria `-`

### Control de flujo
- `si` / `sino si` / `sino` anidados a cualquier profundidad
- `mientras` con cualquier condición booleana
- `hasta` / `paso` con enteros
- Recursión (incluida recursión mutua entre funciones)

### Funciones y procedimientos
- Funciones con cualquier número de parámetros
- Procedimientos (sin retorno)
- Parámetros por referencia
- Llamadas recursivas y mutualmente recursivas
- Forward declarations automáticas

### Arreglos
- Arreglos de `entero`, `decimal`, `booleano`, `texto`
- Tamaño fijo declarado en tiempo de compilación
- Inicializadores con lista de valores
- Acceso por índice entero
- Modificación de elementos

### Estructuras
- Declaración con campos tipados
- Acceso a campos con `.`

### Interoperabilidad
- `enlazar` para llamar cualquier función de la libc
- Strings como `i8*` compatibles con C

### Herramientas
- `--emit-tokens`: ver la tokenización
- `--emit-ast`: ver el árbol sintáctico
- `--emit-ir`: ver el LLVM IR generado
- `--benchmark`: medir tiempos por fase
- `--lang`: cambiar el idioma del lenguaje

---

## 15. Qué NO se puede hacer todavía

Limitaciones de la versión actual, planificadas para versiones futuras:

### Control de flujo
- No hay `break` ni `continue` en loops
- No hay `switch` / `cuando`
- No hay goto

### Tipos
- No hay conversión explícita de tipos (cast)
- No hay tipos genéricos o templates
- No hay tipos de usuario completos (structs parcialmente implementadas)
- No hay punteros explícitos (solo `referencia`)
- No hay manejo de nulos / `nil`

### Funciones
- No hay funciones variádicas (impide `printf` con formato)
- No hay funciones anónimas ni lambdas
- No hay funciones como valores (no son first-class)
- No hay parámetros con valores por defecto
- No hay sobrecarga de funciones

### Arreglos y datos
- No hay arreglos multidimensionales
- No hay arreglos dinámicos (sin `new` / `malloc`)
- No se pueden pasar arreglos como parámetros
- No hay strings mutables ni concatenación de strings

### Organización
- No hay módulos ni `importar`
- No hay espacios de nombres
- No hay archivos múltiples en un mismo programa

### Orientación a objetos
- No hay clases ni métodos
- No hay herencia
- No hay interfaces

---

## 16. Mensajes de error

KEM produce mensajes de error en español con la fase donde ocurrió,
la línea y columna exacta:

```
Error [Léxico] línea 5, col 12: Carácter inesperado '@'
Error [Sintáctico] línea 8, col 3: Se esperaba '{' para abrir el bloque
Error [Semántico] línea 12, col 5: Variable 'x' no declarada
Error [Semántico] línea 15, col 8: No se puede sumar 'entero' y 'texto'
Error [Semántico] línea 20, col 1: La función 'calcular' no siempre retorna un valor
Error [Semántico] línea 25, col 10: 'suma' espera 2 argumento(s), recibió 3
```

El compilador intenta **recuperarse** de errores sintácticos para
reportar múltiples problemas en una sola pasada en lugar de detenerse
en el primero.

---

## 17. Referencia rápida de keywords

| Keyword      | Categoría       | Descripción                              |
|--------------|-----------------|------------------------------------------|
| `inicio`     | Estructura      | Punto de entrada del programa            |
| `funcion`    | Funciones       | Declara una función con retorno          |
| `procedimiento` | Funciones    | Declara una función sin retorno          |
| `devolver`   | Funciones       | Retorna un valor (o sale del proc)       |
| `referencia` | Funciones       | Pasar parámetro por referencia           |
| `entero`     | Tipos           | Entero de 64 bits                        |
| `decimal`    | Tipos           | Punto flotante de 64 bits                |
| `texto`      | Tipos           | Cadena de caracteres                     |
| `booleano`   | Tipos           | Verdadero o falso                        |
| `verdadero`  | Literales       | Valor booleano true                      |
| `falso`      | Literales       | Valor booleano false                     |
| `si`         | Control         | Condicional                              |
| `sino`       | Control         | Rama alternativa del condicional         |
| `mientras`   | Control         | Loop con condición booleana              |
| `hasta`      | Control         | Loop con rango numérico                  |
| `paso`       | Control         | Incremento del bucle hasta               |
| `y`          | Lógica          | AND lógico (ambos operandos booleanos)   |
| `o`          | Lógica          | OR lógico (ambos operandos booleanos)    |
| `no`         | Lógica          | NOT lógico (operando booleano)           |
| `estructura` | Tipos compuestos| Define un tipo con campos                |
| `enlazar`    | Interop         | Declara una función externa de C         |
| `comentario` | Comentarios     | Comentario de línea o bloque             |

---

## Apéndice: Gramática EBNF resumida

```ebnf
programa      = { declaracion_top } bloque_inicio EOF ;
bloque_inicio = "inicio" bloque ;
declaracion   = def_funcion | def_proc | def_struct | decl_enlace ;

def_funcion   = "funcion" IDENT "(" params ")" tipo bloque ;
def_proc      = "procedimiento" IDENT "(" params ")" bloque ;
def_struct    = "estructura" IDENT "{" { campo } "}" ;
decl_enlace   = "enlazar" tipo IDENT "(" [ tipos ] ")" ;

params        = [ param { "," param } ] ;
param         = [ "referencia" ] tipo IDENT ;
tipo          = "entero" | "decimal" | "texto" | "booleano"
              | tipo "[" ENTERO_LIT "]" ;

bloque        = "{" { sentencia } "}" ;
sentencia     = decl_var | decl_arr | asignacion | sent_si
              | sent_mientras | sent_hasta | sent_devolver
              | llamada_stmt | bloque ;

decl_var      = tipo IDENT [ "=" expresion ] ;
decl_arr      = tipo IDENT "[" ENTERO_LIT "]" [ "=" "[" exprs "]" ] ;
asignacion    = lvalue "=" expresion ;
sent_si       = "si" expresion bloque [ "sino" ( sent_si | bloque ) ] ;
sent_mientras = "mientras" expresion bloque ;
sent_hasta    = IDENT "=" expr "hasta" expr [ "paso" expr ] bloque ;
sent_devolver = "devolver" [ expresion ] ;

expresion     = expr_o ;
expr_o        = expr_y { "o" expr_y } ;
expr_y        = expr_eq { "y" expr_eq } ;
expr_eq       = expr_rel { ( "==" | "!=" ) expr_rel } ;
expr_rel      = expr_sum { ( "<" | ">" | "<=" | ">=" ) expr_sum } ;
expr_sum      = expr_mul { ( "+" | "-" ) expr_mul } ;
expr_mul      = expr_una { ( "*" | "/" | "%" ) expr_una } ;
expr_una      = "no" expr_una | "-" expr_una | expr_post ;
expr_post     = expr_prim { "." IDENT | "[" expr "]" } ;
expr_prim     = literal | IDENT | IDENT "(" exprs ")" | "(" expr ")" ;

literal       = ENTERO_LIT | DECIMAL_LIT | TEXTO_LIT
              | "verdadero" | "falso" ;
```