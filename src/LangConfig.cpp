#include "kem/LangConfig.hpp"
#include "kem/ErrorHandler.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace kem {

// ─────────────────────────────────────────────
//  Keywords obligatorias
//  Todo archivo de idioma debe definir estas.
// ─────────────────────────────────────────────
const std::vector<std::string>& LangConfig::requiredKeywords() {
    static const std::vector<std::string> required = {
        "KW_ENTERO", "KW_DECIMAL", "KW_TEXTO", "KW_BOOLEANO",
        "KW_FUNCION", "KW_PROC", "KW_DEVOLVER", "KW_INICIO",
        "KW_SI", "KW_SINO", "KW_MIENTRAS", "KW_HASTA", "KW_PASO",
        "KW_VERDADERO", "KW_FALSO", "KW_Y", "KW_O", "KW_NO",
        "KW_ESTRUCTURA", "KW_REF", "KW_ENLAZAR"
    };
    return required;
}

// ─────────────────────────────────────────────
//  tokenTypeFromString
//  Convierte el string del JSON al TokenType interno.
//  Ejemplo: "KW_FUNCION" → TokenType::KW_FUNCION
// ─────────────────────────────────────────────
TokenType LangConfig::tokenTypeFromString(const std::string& name) {
    static const std::unordered_map<std::string, TokenType> table = {
        {"KW_ENTERO",    TokenType::KW_ENTERO},
        {"KW_DECIMAL",   TokenType::KW_DECIMAL},
        {"KW_TEXTO",     TokenType::KW_TEXTO},
        {"KW_BOOLEANO",  TokenType::KW_BOOLEANO},
        {"KW_FUNCION",   TokenType::KW_FUNCION},
        {"KW_PROC",      TokenType::KW_PROC},
        {"KW_DEVOLVER",  TokenType::KW_DEVOLVER},
        {"KW_INICIO",    TokenType::KW_INICIO},
        {"KW_SI",        TokenType::KW_SI},
        {"KW_SINO",      TokenType::KW_SINO},
        {"KW_MIENTRAS",  TokenType::KW_MIENTRAS},
        {"KW_HASTA",     TokenType::KW_HASTA},
        {"KW_PASO",      TokenType::KW_PASO},
        {"KW_VERDADERO", TokenType::KW_VERDADERO},
        {"KW_FALSO",     TokenType::KW_FALSO},
        {"KW_Y",         TokenType::KW_Y},
        {"KW_O",         TokenType::KW_O},
        {"KW_NO",        TokenType::KW_NO},
        {"KW_ESTRUCTURA",TokenType::KW_ESTRUCTURA},
        {"KW_REF",       TokenType::KW_REF},
        {"KW_ENLAZAR",   TokenType::KW_ENLAZAR},
    };

    auto it = table.find(name);
    if (it == table.end()) {
        configError("TokenType desconocido en archivo de idioma: '" + name + "'");
    }
    return it->second;
}

// ─────────────────────────────────────────────
//  loadFromFile
//  Parser JSON mínimo — solo soporta el formato
//  flat de los archivos de idioma:
//  {
//    "keyword": "TOKEN_TYPE",
//    ...
//  }
//  Sin arrays, sin objetos anidados, sin números.
// ─────────────────────────────────────────────
void LangConfig::loadFromFile(const std::string& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        configError("No se puede abrir el archivo de idioma: '" + json_path + "'");
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    // Extraer el nombre del idioma del nombre del archivo
    // langs/espanol.json → "espanol"
    size_t slash = json_path.rfind('/');
    size_t dot   = json_path.rfind('.');
    if (slash != std::string::npos && dot != std::string::npos && dot > slash) {
        lang_name_ = json_path.substr(slash + 1, dot - slash - 1);
    } else {
        lang_name_ = json_path;
    }

    // Parsear pares "clave": "valor"
    size_t pos = 0;
    while (pos < content.size()) {
        // Buscar apertura de string de clave
        size_t key_start = content.find('"', pos);
        if (key_start == std::string::npos) break;
        size_t key_end = content.find('"', key_start + 1);
        if (key_end == std::string::npos) {
            configError("JSON malformado en '" + json_path + "': cadena sin cerrar");
        }
        std::string key = content.substr(key_start + 1, key_end - key_start - 1);

        // Saltar el ':'
        size_t colon = content.find(':', key_end + 1);
        if (colon == std::string::npos) {
            configError("JSON malformado en '" + json_path + "': falta ':' después de '" + key + "'");
        }

        // Buscar el valor (string)
        size_t val_start = content.find('"', colon + 1);
        if (val_start == std::string::npos) {
            configError("JSON malformado en '" + json_path + "': falta valor para '" + key + "'");
        }
        size_t val_end = content.find('"', val_start + 1);
        if (val_end == std::string::npos) {
            configError("JSON malformado en '" + json_path + "': valor sin cerrar para '" + key + "'");
        }
        std::string value = content.substr(val_start + 1, val_end - val_start - 1);

        // Ignorar claves vacías o que empiecen con '_' (comentarios de convención)
        if (!key.empty() && key[0] != '_') {
            TokenType type = tokenTypeFromString(value);
            keyword_map_[key] = type;
        }

        pos = val_end + 1;
    }
}

// ─────────────────────────────────────────────
//  validate
//  Verifica que todas las keywords obligatorias
//  estén presentes en el mapa cargado.
// ─────────────────────────────────────────────
void LangConfig::validate() const {
    // Construir el set de TokenTypes presentes en el mapa
    std::vector<std::string> missing;

    for (const auto& required : requiredKeywords()) {
        // required es un string como "KW_FUNCION"
        // Buscar si algún valor del mapa coincide
        bool found = false;
        for (const auto& [word, type] : keyword_map_) {
            if (tokenTypeName(type) == required) {
                found = true;
                break;
            }
        }
        if (!found) {
            missing.push_back(required);
        }
    }

    if (!missing.empty()) {
        std::string msg = "El archivo de idioma '" + path_ +
                          "' no define las siguientes keywords obligatorias: ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) msg += ", ";
            msg += missing[i];
        }
        configError(msg);
    }
}

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
LangConfig::LangConfig(const std::string& json_path) : path_(json_path) {
    loadFromFile(json_path);
    validate();
}

// ─────────────────────────────────────────────
//  resolve
// ─────────────────────────────────────────────
TokenType LangConfig::resolve(const std::string& word) const {
    auto it = keyword_map_.find(word);
    if (it != keyword_map_.end()) {
        return it->second;
    }
    return TokenType::IDENT;
}

} // namespace kem
