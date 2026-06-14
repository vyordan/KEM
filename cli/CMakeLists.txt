add_executable(kem main.cpp)

target_link_libraries(kem PRIVATE libkem)

# El ejecutable se llama "kem", no "cli"
set_target_properties(kem PROPERTIES OUTPUT_NAME "kem")

# Copiar langs/ junto al ejecutable para que lo encuentre
add_dependencies(kem copy_langs)
