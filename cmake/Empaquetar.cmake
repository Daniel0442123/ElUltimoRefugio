# Versión portable (carpeta + zip con el juego listo para jugar, sin instalar nada).
#
#   cmake --build <build> --config Release --target paquete
#
# deja <build>/paquete/ElUltimoRefugio-<versión>-<plataforma>.zip con:
#   refugio(.exe), refugio_bench(.exe), LEEME.txt, config/ia.json, maps/, bench/, opt/ y la
#   licencia de la tipografía. saves/, logs/ y results/ los crea el juego junto al ejecutable.
#
# Solo se instala el componente «juego»: raylib y Catch2 traen sus propias reglas de instalación
# (bibliotecas y cabeceras) que no hacen falta para jugar.

if(WIN32)
    set(REFUGIO_PLATAFORMA "win64")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(REFUGIO_PLATAFORMA "win32")
    endif()
elseif(APPLE)
    set(REFUGIO_PLATAFORMA "macos")
else()
    set(REFUGIO_PLATAFORMA "linux-x64")
endif()
set(REFUGIO_NOMBRE_PAQUETE "ElUltimoRefugio-${PROJECT_VERSION}-${REFUGIO_PLATAFORMA}")

set(_ejecutables refugio_bench)
if(REFUGIO_BUILD_GAME)
    list(PREPEND _ejecutables refugio)
endif()
install(TARGETS ${_ejecutables} RUNTIME DESTINATION . COMPONENT juego)
install(FILES "${CMAKE_SOURCE_DIR}/config/ia.json" DESTINATION config COMPONENT juego)
install(FILES "${CMAKE_SOURCE_DIR}/maps/puerto.json" DESTINATION maps COMPONENT juego)
install(FILES "${CMAKE_SOURCE_DIR}/bench/config.json" DESTINATION bench COMPONENT juego)
install(FILES "${CMAKE_SOURCE_DIR}/opt/ga.json" "${CMAKE_SOURCE_DIR}/opt/sa.json" DESTINATION opt COMPONENT juego)
install(FILES "${CMAKE_SOURCE_DIR}/assets/fuentes/OFL.txt" DESTINATION licencias RENAME Inter-OFL.txt COMPONENT juego)
configure_file("${CMAKE_SOURCE_DIR}/tools/portable/LEEME.txt.in" "${CMAKE_BINARY_DIR}/generado/LEEME.txt" @ONLY
               NEWLINE_STYLE CRLF)
install(FILES "${CMAKE_BINARY_DIR}/generado/LEEME.txt" DESTINATION . COMPONENT juego)

set(_staging "${CMAKE_BINARY_DIR}/paquete")
set(_zip "${_staging}/${REFUGIO_NOMBRE_PAQUETE}.zip")
add_custom_target(paquete
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${_staging}/${REFUGIO_NOMBRE_PAQUETE}" "${_zip}"
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config $<CONFIG> --component juego
            --prefix "${_staging}/${REFUGIO_NOMBRE_PAQUETE}"
    COMMAND "${CMAKE_COMMAND}" -E chdir "${_staging}"
            "${CMAKE_COMMAND}" -E tar cf "${REFUGIO_NOMBRE_PAQUETE}.zip" --format=zip "${REFUGIO_NOMBRE_PAQUETE}"
    COMMAND "${CMAKE_COMMAND}" -E echo "Versión portable: ${_zip}"
    VERBATIM)
add_dependencies(paquete ${_ejecutables})
