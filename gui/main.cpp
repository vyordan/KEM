#define TINYFILEDIALOGS_IMPLEMENTATION
#include "tinyfiledialogs.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <future>
#include "AppState.hpp"
#include "ExecutionUtils.hpp"
#include "Theme.hpp"

static AppState gState;
static const char* glsl_version = "#version 130";

void loadLangConfig(const std::string& path) {
    gState.langError.clear();
    try {
        gState.langConfig = std::make_shared<kem::LangConfig>(path);
        gState.langFilePath = path;
    } catch (const std::exception& e) {
        gState.langError = e.what();
        gState.langConfig.reset();
    }
}

void newFile() {
    gState.sourceText.clear();
    gState.filePath.clear();
    gState.modified = false;
    gState.showResult = false;
}

void openFile() {
    const char* filter = "*.kem";
    const char* path = tinyfd_openFileDialog("Abrir archivo KEM", "", 1, &filter, NULL, 0);
    if (path) {
        std::ifstream file(path);
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            gState.sourceText = buffer.str();
            gState.filePath = path;
            gState.modified = false;
        }
    }
}

void saveFile() {
    if (gState.filePath.empty()) {
        const char* filter = "*.kem";
        const char* path = tinyfd_saveFileDialog("Guardar archivo", "", 1, &filter, NULL);
        if (path) gState.filePath = path;
        else return;
    }
    std::ofstream file(gState.filePath);
    if (file) {
        file << gState.sourceText;
        gState.modified = false;
    }
}

void executeCode() {
    if (gState.isExecuting) return;
    if (!gState.langConfig) {
        gState.lastResult.success = false;
        gState.lastResult.errors = "No se ha cargado una configuración de idioma.";
        gState.showResult = true;
        return;
    }

    auto langConfigPtr = gState.langConfig;
    std::string source = gState.sourceText;
    bool emitTokens = gState.emitTokens;
    bool emitAST    = gState.emitAST;
    bool emitIR     = gState.emitIR;
    bool benchmark  = gState.benchmark;
    bool capture    = gState.captureOutput;

    gState.isExecuting = true;
    gState.executionFuture = std::async(std::launch::async, [=]() {
        return executeKemCode(source, *langConfigPtr, emitTokens, emitAST, emitIR, benchmark, capture);
    });
}

void renderUI() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::Button("Nuevo"))    newFile();
        ImGui::SameLine();
        if (ImGui::Button("Abrir"))    openFile();
        ImGui::SameLine();
        if (ImGui::Button("Guardar"))  saveFile();
        ImGui::SameLine();

        if (gState.isExecuting) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::Button("Ejecutando...");
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.596f, 0.792f, 0.247f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.663f, 0.878f, 0.298f, 1.0f));
            if (ImGui::Button("Ejecutar")) executeCode();
            ImGui::PopStyleColor(2);
        }
        ImGui::EndMainMenuBar();
    }

    if (gState.isExecuting && gState.executionFuture.valid()) {
        auto status = gState.executionFuture.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            gState.lastResult = gState.executionFuture.get();
            gState.showResult = true;
            gState.isExecuting = false;
        }
    }

    ImGui::DockSpaceOverViewport();

    // Panel Editor
    ImGui::Begin("Editor");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    constexpr size_t BUF_SIZE = 1024 * 1024;
    static char textBuffer[BUF_SIZE] = {};
    if (gState.sourceText.size() < BUF_SIZE) {
        std::strncpy(textBuffer, gState.sourceText.c_str(), BUF_SIZE - 1);
        textBuffer[gState.sourceText.size()] = '\0';
    }
    if (ImGui::InputTextMultiline("##editor", textBuffer, BUF_SIZE,
        ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput)) {
        gState.sourceText = textBuffer;
        gState.modified = true;
    }
    ImGui::PopStyleVar();
    ImGui::End();

    // Panel Opciones
    ImGui::Begin("Opciones");
    ImGui::Text("Idioma actual:");
    ImGui::TextWrapped("%s", gState.langFilePath.c_str());
    if (ImGui::Button("Cargar otro JSON...")) {
        const char* filter = "*.json";
        const char* path = tinyfd_openFileDialog("Seleccionar archivo de idioma", "", 1, &filter, NULL, 0);
        if (path) loadLangConfig(path);
    }
    if (!gState.langError.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        ImGui::Text("Error: %s", gState.langError.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::Separator();
    ImGui::Text("Mostrar en salida:");
    ImGui::Checkbox("Tokens",    &gState.emitTokens);
    ImGui::Checkbox("AST",       &gState.emitAST);
    ImGui::Checkbox("LLVM IR",   &gState.emitIR);
    ImGui::Checkbox("Benchmark", &gState.benchmark);
    //ImGui::Checkbox("Capturar salida en panel", &gState.captureOutput);
    ImGui::End();

    // Panel Salida
    ImGui::Begin("Salida");
    if (gState.isExecuting) {
        ImGui::Text("Ejecutando código...");
    } else if (gState.showResult) {
        ImGui::BeginTabBar("ResultTabs");
        if (ImGui::BeginTabItem("Salida")) {
            ImGui::TextUnformatted(gState.lastResult.output.c_str());
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Errores")) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            ImGui::TextUnformatted(gState.lastResult.errors.c_str());
            ImGui::PopStyleColor();
            ImGui::EndTabItem();
        }
        if (!gState.lastResult.tokens.empty() && ImGui::BeginTabItem("Tokens")) {
            ImGui::TextUnformatted(gState.lastResult.tokens.c_str());
            ImGui::EndTabItem();
        }
        if (!gState.lastResult.ast.empty() && ImGui::BeginTabItem("AST")) {
            ImGui::TextUnformatted(gState.lastResult.ast.c_str());
            ImGui::EndTabItem();
        }
        if (!gState.lastResult.ir.empty() && ImGui::BeginTabItem("IR")) {
            ImGui::TextUnformatted(gState.lastResult.ir.c_str());
            ImGui::EndTabItem();
        }
        if (!gState.lastResult.benchmark.empty() && ImGui::BeginTabItem("Bench")) {
            ImGui::TextUnformatted(gState.lastResult.benchmark.c_str());
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    } else {
        ImGui::Text("Presiona 'Ejecutar' para ver resultados.");
    }
    ImGui::End();

    // Barra de estado
    ImGui::Begin("Estado", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Text("Modificado: %s", gState.modified ? "Sí" : "No");
    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1280, 800, "KEM Editor", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontFromFileTTF("gui/imgui/fonts/ProggyClean.ttf", 13.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SetupKemTheme();
    loadLangConfig(gState.langFilePath);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.043f, 0.059f, 0.055f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}