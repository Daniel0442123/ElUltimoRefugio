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
    unit/test_superviviente.cpp
    unit/test_monitoreo.cpp
    unit/test_comparacion.cpp
    unit/test_benchmark.cpp
    unit/test_aislados_coordinados.cpp
    unit/test_persistencia.cpp
    unit/test_configuracion.cpp
    unit/test_tutorial.cpp
    unit/test_menu_pausa.cpp
    unit/test_optimizacion.cpp
    unit/contador_asignaciones.cpp
)
target_link_libraries(refugio_tests PRIVATE refugio_dominio refugio_archivo refugio_cli refugio_menu Catch2::Catch2WithMain)
# CP-080: las pruebas leen tests/data/bench_mini.json y escriben en el directorio de compilación.
target_compile_definitions(refugio_tests PRIVATE REFUGIO_DATOS_PRUEBA="${CMAKE_CURRENT_SOURCE_DIR}/data"
                                                 REFUGIO_DIR_SALIDA="${CMAKE_CURRENT_BINARY_DIR}/salida"
                                                 REFUGIO_DIR_FUENTES="${PROJECT_SOURCE_DIR}")
refugio_configurar_objetivo(refugio_tests)

include(Catch)
catch_discover_tests(refugio_tests)

# Los nombres de TEST_CASE deben ser ASCII: CTest pasa el nombre como filtro y en Windows la
# consola no conserva los caracteres acentuados, así que la prueba no se encontraría.
