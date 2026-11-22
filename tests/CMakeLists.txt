add_executable(refugio_tests
    unit/test_pcg32.cpp
    unit/test_grafo.cpp
    unit/test_busqueda.cpp
    unit/test_propagador_alertas.cpp
    unit/test_quadtree.cpp
)
target_link_libraries(refugio_tests PRIVATE refugio_dominio Catch2::Catch2WithMain)
refugio_configurar_objetivo(refugio_tests)

include(Catch)
catch_discover_tests(refugio_tests)
