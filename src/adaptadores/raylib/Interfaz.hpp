#pragma once
/// @file Interfaz.hpp
/// @brief Tipografía y escala de la interfaz: textos nítidos en cualquier tamaño de ventana.
///
/// Los textos usan Inter (sans-serif, licencia SIL OFL 1.1), incrustada en el ejecutable, en
/// vez de la fuente pixelada por defecto de raylib. Incluye los caracteres del español (á é í
/// ó ú ü ñ ¿ ¡ « ») y del Latin-1.
///
/// La interfaz se dibuja en unidades (vista::Disposicion) bajo una Camera2D cuyo zoom es la
/// escala. Para que el texto no se vea borroso al ampliarse, cada tamaño se rasteriza a su
/// tamaño final en píxeles (tamaño en unidades x escala) y se guarda en una caché: el glifo
/// acaba ocupando exactamente sus píxeles de pantalla. Dentro del mundo (rótulos EXT, ALM, «!»…)
/// la escala es el zoom de la cámara del mapa (AmbitoEscalaTexto).
///
/// Las firmas de dibujarTexto y medirTexto son las de DrawText y MeasureText para poder
/// sustituirlas sin cambiar el diseño de los paneles.

#include <raylib.h>

namespace refugio::adaptadores {

enum class Peso : unsigned char { Normal, Negrita };

/// Prepara las fuentes; requiere la ventana abierta. Sin ella se usa la fuente de raylib.
void cargarFuentes();
/// Libera todas las fuentes rasterizadas (antes de CloseWindow).
void liberarFuentes();

/// Píxeles de pantalla por unidad de interfaz en este frame (vista::Disposicion::escala).
void fijarEscalaInterfaz(float escala) noexcept;
[[nodiscard]] float escalaInterfaz() noexcept;
/// Posición del ratón en unidades de interfaz.
[[nodiscard]] Vector2 ratonInterfaz();

/// Mientras vive, los textos se rasterizan para @p escala píxeles por unidad (p. ej. el zoom de
/// la cámara del mapa). Al destruirse vuelve la escala anterior.
class AmbitoEscalaTexto {
public:
    explicit AmbitoEscalaTexto(float escala) noexcept;
    ~AmbitoEscalaTexto();
    AmbitoEscalaTexto(const AmbitoEscalaTexto&) = delete;
    AmbitoEscalaTexto& operator=(const AmbitoEscalaTexto&) = delete;

private:
    float previa_;
};

/// Como DrawText, con Inter. @p tam es la altura de línea en unidades del dibujo actual.
void dibujarTexto(const char* texto, int x, int y, int tam, Color color, Peso peso = Peso::Normal);
void dibujarTexto(const char* texto, Vector2 pos, float tam, Color color, Peso peso = Peso::Normal);
/// Como MeasureText: ancho en unidades del dibujo actual.
[[nodiscard]] int medirTexto(const char* texto, int tam, Peso peso = Peso::Normal);
[[nodiscard]] float medirTextoF(const char* texto, float tam, Peso peso = Peso::Normal);

}  // namespace refugio::adaptadores
