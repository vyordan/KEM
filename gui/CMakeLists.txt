find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# ImGui con TODOS los archivos necesarios
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp      
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)
target_link_libraries(imgui PUBLIC OpenGL::GL glfw)

add_executable(kem_gui
    main.cpp
    tinyfiledialogs.c          # ← añadido de nuevo
)
target_include_directories(kem_gui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/gui/imgui
)
target_link_libraries(kem_gui PRIVATE
    libkem
    imgui
    glfw
    OpenGL::GL
)
target_compile_definitions(kem_gui PRIVATE
    KEM_LANG_DIR="${CMAKE_SOURCE_DIR}/langs"
)
add_custom_command(TARGET kem_gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/fonts
        $<TARGET_FILE_DIR:kem_gui>/fonts
    COMMENT "Copiando fuentes al directorio del ejecutable"
)