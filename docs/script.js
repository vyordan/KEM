/* ============================================================
   KEM · SCRIPT GLOBAL (solo menú y switchers de idioma)
   ============================================================ */

// ----- MENÚ HAMBURGUESA -----
function toggleMenu() {
  document.getElementById('navLinks').classList.toggle('show');
}

// ----- EJEMPLOS DE CÓDIGO PARA EL SNIPPET (index) -----
const SNIPPET_EXAMPLES = {
  es: {
    json: "langs/espanol.json",
    code: `funcion fibonacci(entero n) entero {
    si n < 2 {
        devolver n
    }
    devolver fibonacci(n - 1) + fibonacci(n - 2)
}

inicio {
    entero resultado = fibonacci(10)
}`
  },
  ki: {
    json: "langs/kiche.json",
    code: `chak fibonacci(tz'akat n) tz'akat {
    we n < 2 {
        tzelej n
    }
    tzelej fibonacci(n - 1) + fibonacci(n - 2)
}

majtibal {
    tz'akat resultado = fibonacci(10)
}`
  },
  en: {
    json: "langs/english.json",
    code: `function fibonacci(int n) int {
    if n < 2 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

main {
    int result = fibonacci(10)
}`
  }
};

// ----- INICIALIZAR SNIPPET -----
document.addEventListener("DOMContentLoaded", () => {
  const codeEl = document.getElementById('snippet-code');
  const langInd = document.getElementById('snippet-lang-indicator');
  if (codeEl && langInd) {
    codeEl.textContent = SNIPPET_EXAMPLES.es.code;
    langInd.textContent = SNIPPET_EXAMPLES.es.json;
  }
});

// ----- CAMBIAR IDIOMA DEL SNIPPET -----
function switchSnippetLang(lang) {
  const codeEl = document.getElementById('snippet-code');
  const langInd = document.getElementById('snippet-lang-indicator');
  if (!codeEl || !langInd) return;

  codeEl.textContent = SNIPPET_EXAMPLES[lang].code;
  langInd.textContent = SNIPPET_EXAMPLES[lang].json;

  // Actualizar clases de pestañas
  const buttons = document.querySelectorAll('#snippet-block .tab-btn');
  buttons.forEach(btn => btn.classList.remove('active'));
  const langIndex = {es:0, ki:1, en:2}[lang];
  if (buttons[langIndex]) buttons[langIndex].classList.add('active');
}