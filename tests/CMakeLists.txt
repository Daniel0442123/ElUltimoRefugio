add_executable(refugio_tests
    unit/test_pcg32.cpp
    unit/test_grafo.cpp
    unit/test_busqueda.cpp
    unit/test_propagador_alertas.cpp
    unit/test_quadtree.cpp
    unit/test_percepcion.cpp
    unit/test_maquina_estados.cpp
    unit/test_partida.cpp
    unit/test_coordinacion_horda.cpp
    unit/test_horda_coordinada.cpp
    unit/test_senuelos_barricadas.cpp
    unit/test_tacticas.cpp
    unit/test_extraccion_refugio.cpp
    unit/contador_asignaciones.cpp
)
target_link_libraries(refugio_tests PRIVATE refugio_dominio Catch2::Catch2WithMain)
refugio_configurar_objetivo(refugio_tests)

include(Catch)
catch_discover_tests(refugio_tests)

# Los nombres de TEST_CASE deben ser ASCII: CTest pasa el nombre como filtro y en Windows la
# consola no conserva los caracteres acentuados, así que la prueba no se encontraría.
