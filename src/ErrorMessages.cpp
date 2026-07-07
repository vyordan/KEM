#include "kem/ErrorMessages.hpp"
#include "kem/ErrorHandler.hpp"

#include <fstream>
#include <sstream>

namespace kem {

// ─────────────────────────────────────────────
//  loadFromFile
//  Reutiliza el mismo parser JSON mínimo que LangConfig:
//  un archivo flat de pares "clave": "valor", sin arrays
//  ni objetos anidados. Las claves que empiezan con '_'
//  se ignoran (metadatos como "_nombre", "_version").
// ─────────────────────────────────────────────
void ErrorMessages::loadFromFile(const std::string& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        configError("No se puede abrir el archivo de mensajes de error: '" +
                    json_path + "'");
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    size_t pos = 0;
    while (pos < content.size()) {
        size_t key_start = content.find('"', pos);
        if (key_start == std::string::npos) break;
        size_t key_end = content.find('"', key_start + 1);
        if (key_end == std::string::npos) {
            configError("JSON malformado en '" + json_path + "': clave sin cerrar");
        }
        std::string key = content.substr(key_start + 1, key_end - key_start - 1);

        size_t colon = content.find(':', key_end + 1);
        if (colon == std::string::npos) {
            configError("JSON malformado en '" + json_path +
                        "': falta ':' después de '" + key + "'");
        }

        // Buscar el valor — soporta strings con \" escapado dentro
        size_t val_start = content.find('"', colon + 1);
        if (val_start == std::string::npos) {
            configError("JSON malformado en '" + json_path +
                        "': falta valor para '" + key + "'");
        }

        size_t val_end = val_start + 1;
        std::string value;
        while (val_end < content.size() && content[val_end] != '"') {
            if (content[val_end] == '\\' && val_end + 1 < content.size()) {
                char next = content[val_end + 1];
                if (next == 'n')      { value += '\n'; val_end += 2; continue; }
                if (next == 't')      { value += '\t'; val_end += 2; continue; }
                if (next == '"')      { value += '"';  val_end += 2; continue; }
                if (next == '\\')     { value += '\\'; val_end += 2; continue; }
            }
            value += content[val_end];
            ++val_end;
        }
        if (val_end >= content.size()) {
            configError("JSON malformado en '" + json_path +
                        "': valor sin cerrar para '" + key + "'");
        }

        if (!key.empty() && key[0] != '_') {
            templates_[key] = value;
        }

        pos = val_end + 1;
    }
}

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
ErrorMessages::ErrorMessages(const std::string& json_path) : path_(json_path) {
    loadFromFile(json_path);
}

// ─────────────────────────────────────────────
//  format
//  Sustituye {0}, {1}, {2}... por los argumentos en orden.
//  Si la clave no existe en el archivo cargado, retorna el
//  código entre corchetes en vez de lanzar — degradación
//  controlada para archivos de idioma incompletos.
// ─────────────────────────────────────────────
std::string ErrorMessages::format(const std::string& code,
                                    const std::vector<std::string>& args) const {
    auto it = templates_.find(code);
    if (it == templates_.end()) {
        // Mensaje de respaldo: visible que falta la traducción,
        // pero no rompe la ejecución del compilador.
        std::string fallback = "[" + code + "]";
        for (const auto& a : args) fallback += " " + a;
        return fallback;
    }

    std::string result = it->second;

    // Sustituir cada {i} por args[i]
    for (size_t i = 0; i < args.size(); ++i) {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.size(), args[i]);
            pos += args[i].size();
        }
    }

    return result;
}

// ─────────────────────────────────────────────
//  Instancia global de mensajes de error activos
// ─────────────────────────────────────────────
static ErrorMessages g_error_messages;

void setErrorMessages(ErrorMessages msgs) {
    g_error_messages = std::move(msgs);
}

const ErrorMessages& errorMessages() {
    return g_error_messages;
}

} // namespace kem
