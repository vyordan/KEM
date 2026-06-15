# Gramática EBNF — Lenguaje KEM
> Versión 0.1 — Documento de diseño (pre-implementación)
> Las palabras clave mostradas corresponden a `langs/espanol.json`.
> El compilador las resuelve via `LangConfig` — son intercambiables por idioma.

---

## Notación usada

| Símbolo | Significado |
|---------|-------------|
| `=`     | define la regla |
| `;`     | termina la regla |
| `\|`    | alternativa (OR) |
| `( )` | agrupación |
| `[ ]`   | opcional (0 o 1 vez) |
| `{ }`   | repetición (0 o más veces) |
| `" "`   | terminal literal |
| `MAYUS` | token producido por el Lexer |

---

## 1. Programa (punto de entrada)

```ebnf
programa        = { declaracion_top } bloque_inicio EOF ;

declaracion_top = def_funcion
                | def_procedimiento
                | def_estructura
                | decl_variable
                ;

bloque_inicio   = "inicio" bloque ;
```

> `inicio { }` es el equivalente de `main`. Todo programa KEM debe tener exactamente uno.

---

## 2. Tipos

```ebnf
tipo            = tipo_primitivo
                | tipo_arreglo
                ;

tipo_primitivo  = "entero"
                | "decimal"
                | "texto"
                | "booleano"
                ;

tipo_arreglo    = tipo_primitivo "[" ENTERO_LIT "]" ;
```

**Ejemplos:**
```
entero
decimal
texto
booleano
entero[10]
decimal[32]
```

---

## 3. Literales

```ebnf
literal         = ENTERO_LIT
                | DECIMAL_LIT
                | TEXTO_LIT
                | "verdadero"
                | "falso"
                ;

ENTERO_LIT      = DIGITO { DIGITO } ;
DECIMAL_LIT     = DIGITO { DIGITO } "." DIGITO { DIGITO } ;
TEXTO_LIT       = '"' { cualquier_caracter_excepto_comilla } '"' ;
DIGITO          = "0" | "1" | ... | "9" ;
LETRA           = "a" | ... | "z" | "A" | ... | "Z" | "_" ;
IDENT           = LETRA { LETRA | DIGITO } ;
```

---

## 4. Declaración de variables

```ebnf
decl_variable   = tipo IDENT [ "=" expresion ] NEWLINE
                | tipo_arreglo IDENT [ "=" inicializador_arreglo ] NEWLINE
                ;

inicializador_arreglo = "[" [ expresion { "," expresion } ] "]" ;
```

**Ejemplos:**
```
entero x = 5
decimal pi = 3.14
booleano activo = verdadero
texto nombre = "KEM"
entero nums[3] = [1, 2, 3]
entero vacio[10]
```

---

## 5. Asignación

```ebnf
asignacion      = lvalue "=" expresion NEWLINE ;

lvalue          = IDENT
                | IDENT "[" expresion "]"
                | IDENT { "." IDENT }
                | IDENT { "." IDENT } "[" expresion "]"
                ;
```

**Ejemplos:**
```
x = 10
nums[0] = 42
persona.edad = 25
persona.scores[2] = 100
```

---

## 6. Expresiones

Las reglas están ordenadas de **menor a mayor precedencia**.
El parser usa Pratt parsing — esta jerarquía guía la tabla de precedencias.

```ebnf
expresion       = expr_o ;

expr_o          = expr_y { "o" expr_y } ;

expr_y          = expr_igual { "y" expr_igual } ;

expr_igual      = expr_relacional { ( "==" | "!=" ) expr_relacional } ;

expr_relacional = expr_suma { ( "<" | ">" | "<=" | ">=" ) expr_suma } ;

expr_suma       = expr_mul { ( "+" | "-" ) expr_mul } ;

expr_mul        = expr_unaria { ( "*" | "/" | "%" ) expr_unaria } ;

expr_unaria     = "no" expr_unaria
                | "-" expr_unaria
                | expr_postfija
                ;

expr_postfija   = expr_primaria { postfijo } ;

postfijo        = "[" expresion "]"
                | "." IDENT
                | "(" [ lista_args ] ")"
                ;

expr_primaria   = literal
                | IDENT
                | "(" expresion ")"
                ;

lista_args      = expresion { "," expresion } ;
```

**Ejemplos de expresiones:**
```
2 + 3 * 4          → 14  (mul antes que suma)
no verdadero       → falso
x >= 0 y x < 10
persona.edad + 1
nums[i] * 2
factorial(n - 1)
(a + b) * c
```

---

## 7. Sentencias

```ebnf
sentencia       = decl_variable
                | asignacion
                | sent_si
                | sent_mientras
                | sent_hasta
                | sent_devolver
                | llamada_stmt
                | bloque
                ;

llamada_stmt    = IDENT "(" [ lista_args ] ")" NEWLINE ;

bloque          = "{" { sentencia } "}" ;
```

---

## 8. Flujo de control

### si / sino

```ebnf
sent_si         = "si" expresion bloque
                  [ "sino" ( sent_si | bloque ) ]
                ;
```

**Ejemplos:**
```
si x > 0 {
    resultado = x
}

si x > 0 {
    resultado = x
} sino {
    resultado = 0
}

si x > 100 {
    nivel = "alto"
} sino si x > 50 {
    nivel = "medio"
} sino {
    nivel = "bajo"
}
```

### mientras (loop universal)

```ebnf
sent_mientras   = "mientras" expresion bloque ;
```

**Ejemplos:**
```
mientras activo {
    procesar()
}

mientras i < 10 {
    i = i + 1
}
```

### hasta / paso (loop con rango)

```ebnf
sent_hasta      = IDENT "=" expresion "hasta" expresion
                  [ "paso" expresion ]
                  bloque
                ;
```

> La variable de iteración debe estar declarada antes del bucle.
> `paso` es opcional — si se omite, el incremento es 1.

**Ejemplos:**
```
entero i = 0
i = 0 hasta 10 {
    imprimir(i)
}

i = 0 hasta 100 paso 5 {
    suma = suma + i
}

i = 10 hasta 0 paso -1 {
    cuenta_regresiva(i)
}
```

---

## 9. Funciones y procedimientos

```ebnf
def_funcion     = "funcion" IDENT "(" [ lista_params ] ")" tipo_primitivo bloque ;

def_procedimiento = "procedimiento" IDENT "(" [ lista_params ] ")" bloque ;

lista_params    = parametro { "," parametro } ;

parametro       = tipo IDENT
                | "referencia" tipo IDENT
                ;

sent_devolver   = "devolver" [ expresion ] NEWLINE ;
```

> `funcion` siempre declara un tipo de retorno y debe contener `devolver`.
> `procedimiento` no retorna valor; `devolver` sin expresión puede usarse para salida temprana.

**Ejemplos:**
```
funcion suma(entero a, entero b) entero {
    devolver a + b
}

procedimiento saludar(texto nombre) {
    imprimir(nombre)
}

funcion maximo(entero a, entero b) entero {
    si a > b {
        devolver a
    }
    devolver b
}

procedimiento intercambiar(referencia entero a, referencia entero b) {
    entero temp = a
    a = b
    b = temp
}
```

---

## 10. Estructuras

```ebnf
def_estructura  = "estructura" IDENT "{" { campo_estructura } "}" ;

campo_estructura = tipo IDENT NEWLINE ;
```

> Las estructuras solo contienen campos tipados — no métodos (por ahora).
> Se accede a los campos con `.`

**Ejemplos:**
```
estructura Persona {
    texto nombre
    entero edad
    decimal altura
}

Persona p
p.nombre = "Ana"
p.edad = 25
```

---

## 11. Llamadas a funciones externas (enlazar)

```ebnf
decl_enlace     = "enlazar" tipo_o_void IDENT "(" [ lista_tipos ] ")" NEWLINE ;

tipo_o_void     = tipo_primitivo | "vacio" ;

lista_tipos     = tipo_primitivo { "," tipo_primitivo } ;
```

> `enlazar` declara una función externa (equivalente a `extern` en C).
> Permite llamar funciones de la stdlib de C directamente desde KEM.

**Ejemplos:**
```
enlazar vacio imprimir(texto)
enlazar entero longitud(texto)
```

---

## 12. Comentarios

```ebnf
comentario      = comentario_linea | comentario_bloque ;

comentario_linea  = ( "//" | "comentario" ) { cualquier_char } NEWLINE ;

comentario_bloque = ( "/*" { cualquier_char } "*/" )
                  | ( "comentario{" { cualquier_char } "}" )
                  ;
```

> Los comentarios son ignorados completamente por el Lexer — no producen tokens.

**Ejemplos:**
```
// esto es un comentario de línea
comentario esto también es un comentario de línea

/* esto es un
   comentario de bloque */

comentario{
    esto también es
    un bloque comentado
}
```

---

## 13. Reglas de continuación de línea (sin punto y coma)

El Lexer emite un token `NEWLINE` al final de cada línea física.
El Parser **descarta** ese `NEWLINE` (la sentencia continúa) cuando:

1. La línea termina en un operador binario: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `y`, `o`, `=`, `,`
2. Hay paréntesis `()` o corchetes `[]` abiertos sin cerrar
3. La última línea visible terminó en `{` (inicio de bloque)

En todos los demás casos, `NEWLINE` termina la sentencia actual.

**Ejemplos:**
```
entero resultado = 1 +    ← continúa (termina en operador)
                  2 +
                  3

entero suma = (a +        ← continúa (paréntesis abierto)
               b +
               c)

entero x = 5              ← NEWLINE termina la sentencia ✓
```

---

## 14. Tabla de palabras reservadas

| Categoría | Keyword KEM | `TokenType` interno |
|-----------|-------------|---------------------|
| Tipos | `entero` | `KW_ENTERO` |
| Tipos | `decimal` | `KW_DECIMAL` |
| Tipos | `texto` | `KW_TEXTO` |
| Tipos | `booleano` | `KW_BOOLEANO` |
| Funciones | `funcion` | `KW_FUNCION` |
| Funciones | `procedimiento` | `KW_PROC` |
| Funciones | `devolver` | `KW_DEVOLVER` |
| Entrada | `inicio` | `KW_INICIO` |
| Control | `si` | `KW_SI` |
| Control | `sino` | `KW_SINO` |
| Control | `mientras` | `KW_MIENTRAS` |
| Control | `hasta` | `KW_HASTA` |
| Control | `paso` | `KW_PASO` |
| Lógica | `verdadero` | `KW_VERDADERO` |
| Lógica | `falso` | `KW_FALSO` |
| Lógica | `y` | `KW_Y` |
| Lógica | `o` | `KW_O` |
| Lógica | `no` | `KW_NO` |
| Tipos compuestos | `estructura` | `KW_ESTRUCTURA` |
| Memoria | `referencia` | `KW_REF` |
| Interop | `enlazar` | `KW_ENLAZAR` |
| Comentarios | `comentario` | *(descartado por Lexer)* |

---

## 15. Programa de ejemplo completo

```
enlazar vacio imprimir(entero)

funcion fibonacci(entero n) entero {
    si n < 2 {
        devolver n
    }
    devolver fibonacci(n - 1) + fibonacci(n - 2)
}

estructura Punto {
    decimal x
    decimal y
}

procedimiento mostrar_punto(Punto p) {
    imprimir(p.x)
    imprimir(p.y)
}

inicio {
    // Calcular fibonacci
    entero resultado = fibonacci(10)
    imprimir(resultado)

    // Arreglo y bucle
    entero nums[5] = [10, 20, 30, 40, 50]
    entero i = 0
    entero suma = 0

    i = 0 hasta 5 {
        suma = suma + nums[i]
    }
    imprimir(suma)

    // Estructura
    Punto origen
    origen.x = 0.0
    origen.y = 0.0
    mostrar_punto(origen)
}
```

---

## 16. Decisiones de diseño documentadas

| Decisión | Elección | Razón |
|----------|----------|-------|
| Declaración de variables | `tipo nombre = valor` | Legible, el tipo va primero como en C/Go |
| Bucle for | `var = inicio hasta fin paso n` | Más natural en español que sintaxis con `;` |
| Arreglos | `tipo nombre[N]` | Similar a C, tamaño explícito en el tipo |
| Sin punto y coma | Sí, basado en NEWLINE | Menos ruido sintáctico, más legible |
| `funcion` vs `procedimiento` | Ambos | Explícito y educativo; el tipo de retorno aclara intención |
| Keywords por archivo JSON | Sí | Permite compilador multi-idioma sin cambiar el código |
| `inicio {}` como main | Sí | Punto de entrada claro y uniforme |
| Comentarios duales | `//` y `comentario` | Universal + intuitivo para hispanohablantes |

---
