# Incrusta archivos binarios (las fuentes de assets/fuentes) en el ejecutable como arreglos de
# bytes, para que el juego no dependa del directorio desde el que se lanza.
#
# refugio_embeber(<salida.cpp> <espacio de nombres> SIMBOLO archivo [SIMBOLO archivo ...])
# genera <salida.cpp> con, para cada par, `const unsigned char SIMBOLO[]` y
# `const int SIMBOLOTam`. El archivo solo se reescribe si cambia su contenido y CMake vuelve a
# configurar cuando cambia cualquiera de los archivos incrustados.
function(refugio_embeber salida espacio)
    set(cuerpo "// Generado por cmake/Embeber.cmake: no editar.\nnamespace ${espacio} {\n")
    set(pares ${ARGN})
    list(LENGTH pares n)
    math(EXPR ultimo "${n} - 1")
    foreach(i RANGE 0 ${ultimo} 2)
        math(EXPR j "${i} + 1")
        list(GET pares ${i} simbolo)
        list(GET pares ${j} archivo)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${archivo}")
        file(READ "${archivo}" hex HEX)
        string(LENGTH "${hex}" largo)
        math(EXPR bytes "${largo} / 2")
        # 32 bytes por línea para que el archivo generado se pueda leer y compilar sin problemas.
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," hex "${hex}")
        string(REGEX REPLACE "((0x..,){32})" "\\1\n" hex "${hex}")
        string(APPEND cuerpo "extern const unsigned char ${simbolo}[];\nextern const int ${simbolo}Tam;\n"
                             "const unsigned char ${simbolo}[] = {\n${hex}\n};\n"
                             "const int ${simbolo}Tam = ${bytes};\n")
    endforeach()
    string(APPEND cuerpo "}  // namespace ${espacio}\n")
    file(CONFIGURE OUTPUT "${salida}" CONTENT "${cuerpo}" @ONLY)
endfunction()
