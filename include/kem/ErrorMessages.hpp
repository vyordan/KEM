#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace kem {

// ─────────────────────────────────────────────
//  ErrorMessages
//
//  Carga las plantillas de mensajes de error desde un archivo JSON
//  (ej: langs/errors/espanol.json). Cada clave es un código de error
//  interno estable (ej: "SEM_VAR_NO_DECLARADA") que NUNCA cambia,
//  y el valor es la plantilla de texto en el idioma elegido, con
//  placeholders {0}, {1}, {2}... que se sustituyen en tiempo de error.
//
//  Ejemplo de archivo:
//  {
//    "SEM_VAR_NO_DECLARADA": "Variable '{0}' no declarada"
//  }
//
//  Uso:
//    ErrorMessages msgs("langs/errors/espanol.json");
//    std::string texto = msgs.format("SEM_VAR_NO_DECLARADA", {"x"});
//    // → "Variable 'x' no declarada"
//
//  Si una clave no está en el archivo cargado, format() retorna
//  el código mismo entre corchetes (ej: "[SEM_VAR_NO_DECLARADA]")
//  en vez de lanzar, para que un archivo de idioma incompleto no
//  tire abajo todo el compilador — simplemente se ve menos prolijo.
// ─────────────────────────────────────────────
class ErrorMessages {
public:
    // Carga el archivo de mensajes de error del idioma indicado.
    // Lanza KemError si el archivo no existe o el JSON es inválido.
    explicit ErrorMessages(const std::string& json_path);

    // Construye una instancia "nula" que siempre retorna el código
    // entre corchetes. Útil como fallback antes de que el JSON cargue,
    // o en tests que no necesitan mensajes localizados reales.
    ErrorMessages() = default;

    // Formatea un mensaje sustituyendo {0}, {1}, etc. por los argumentos.
    // El orden de args define qué reemplaza cada placeholder.
    std::string format(const std::string& code,
                        const std::vector<std::string>& args = {}) const;

    // Retorna la ruta del archivo cargado (vacío si es la instancia nula)
    const std::string& path() const { return path_; }

private:
    std::string path_;
    std::unordered_map<std::string, std::string> templates_;

    void loadFromFile(const std::string& json_path);
};

// ─────────────────────────────────────────────
//  Acceso global a los mensajes de error activos
//
//  El compilador carga UN solo archivo de mensajes por ejecución,
//  elegido junto con el idioma de las keywords (mismo flag --lang,
//  o uno separado --error-lang si se quiere desacoplar).
//  Todas las fases (Lexer, Parser, SemanticAnalyzer, IRGenerator)
//  consultan esta instancia global a través de errorMessages().
//
//  setErrorMessages() se llama una vez al arrancar el CLI, antes
//  de correr cualquier fase del pipeline.
// ─────────────────────────────────────────────
void setErrorMessages(ErrorMessages msgs);
const ErrorMessages& errorMessages();

} // namespace kem
