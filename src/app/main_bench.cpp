/// Ejecutable sin ventana para benchmark y pruebas de estrés (no enlaza raylib).
#include <iostream>

#include "adaptadores/sistema/CarpetaEjecutable.hpp"
#include "app/Cli.hpp"

int main(int argc, char** argv) {
    refugio::adaptadores::usarCarpetaDelEjecutable();  // versión portable: datos junto al .exe
    int codigo = 0;
    if (!refugio::app::ejecutarModoSinVentana(argc, argv, codigo)) {
        std::cout << "Indica un modo. Usa --ayuda para ver las opciones.\n";
        return 1;
    }
    return codigo;
}
