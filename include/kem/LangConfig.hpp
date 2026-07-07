#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "kem/Token.hpp"

namespace kem {

// ─────────────────────────────────────────────
//  LangConfig
//
//  Se instancia UNA VEZ al arrancar el compilador.
//  Lee el archivo JSON del idioma (ej: langs/espanol.json)
//  y construye la tabla de keywords.
//
//  El Lexer consulta resolve() para saber si un identificador
//  es una keyword del idioma o un nombre de variable/función.
//
//  Todo el resto del compilador maneja solo TokenType —
//  nunca sabe qué palabra concreta usó el programador.
// ─────────────────────────────────────────────
class LangConfig {
public:

    // Carga y valida el archivo JSON de idioma.
    // Lanza std::runtime_error si:
    //   - el archivo no existe o no se puede leer
    //   - el JSON está malformado
    //   - falta alguna keyword obligatoria
    explicit LangConfig(const std::string& json_path);

    // Resuelve un identificador.
    // Retorna el TokenType correspondiente si es una keyword del idioma,
    // o TokenType::IDENT si es un nombre de usuario.
    TokenType resolve(const std::string& word) const;

    // Retorna la ruta del archivo cargado (para mensajes de error)
    const std::string& path() const { return path_; }

    // Retorna el nombre del idioma declarado en el JSON ("español", "english"…)
    const std::string& langName() const { return lang_name_; }

    // ── Palabras de comentario (configurables por idioma) ──────────
    // El JSON puede definir las claves especiales "_comment_line" y
    // "_comment_block" con la palabra nativa de comentario del idioma
    // (ej: "comentario" en español, "comment" en inglés). Si el idioma
    // no las define, el estilo de comentario nativo de KEM queda
    // deshabilitado para ese idioma — solo quedan // y /* */ que son
    // universales y no dependen de ninguna palabra.

    // Palabra que inicia un comentario de línea estilo KEM. Vacío si no está definida.
    const std::string& commentLineWord() const { return comment_line_word_; }

    // Palabra que inicia un comentario de bloque estilo KEM (seguida de '{').
    // Vacío si no está definida.
    const std::string& commentBlockWord() const { return comment_block_word_; }

private:
    std::string path_;
    std::string lang_name_;

    std::string comment_line_word_;
    std::string comment_block_word_;

    // Mapa: palabra en el idioma → TokenType interno
    // Ejemplo: "funcion" → KW_FUNCION  (espanol.json)
    //          "function" → KW_FUNCION  (english.json)
    std::unordered_map<std::string, TokenType> keyword_map_;

    // Parsea el JSON a mano con lógica mínima —
    // sin dependencia de librerías externas de JSON.
    void loadFromFile(const std::string& json_path);

    // Verifica que estén presentes todas las keywords obligatorias.
    // Lanza runtime_error con la lista de las que faltan.
    void validate() const;

    // Mapea el string del JSON al TokenType correspondiente.
    // Ejemplo: "KW_FUNCION" → TokenType::KW_FUNCION
    static TokenType tokenTypeFromString(const std::string& name);

    // Keywords obligatorias — cualquier archivo de idioma debe definirlas
    static const std::vector<std::string>& requiredKeywords();
};

} // namespace kem
