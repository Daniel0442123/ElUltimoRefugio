#pragma once
/// @file ArteMundo.hpp
/// @brief Arte pixel del mundo: calles, edificios, sprites animados, sangre y luz con sombras.
///
/// Todo el arte es original y se genera por código al abrir la ventana: no hay archivos de
/// imagen que cargar ni licencias de terceros. Los sprites son de 16 x 16 px y se dibujan al
/// doble (una celda de 32 px). El fondo se pinta una vez por mapa en una textura y la luz se
/// acumula cada frame en otra textura a media resolución que multiplica la escena.
///
/// Solo lee la Partida: la decoración sale de un hash de la semilla (vista::hashCelda) y la
/// animación del tiempo de dibujo, así que nada de esto cambia la simulación (ADR-0005, ADR-0012).

#include <raylib.h>

#include <array>
#include <cstdint>
#include <vector>

#include "adaptadores/raylib/VistaDepuracion.hpp"
#include "adaptadores/vista/Escenario.hpp"
#include "dominio/juego/Partida.hpp"

namespace refugio::adaptadores {

class ArteMundo {
public:
    ArteMundo();  ///< Requiere la ventana abierta (crea texturas).
    ~ArteMundo();
    ArteMundo(const ArteMundo&) = delete;
    ArteMundo& operator=(const ArteMundo&) = delete;

    /// Rehace el fondo si cambió el mapa, avanza las animaciones y las manchas de sangre y, con
    /// @p iluminacion, pinta la textura de luz. Debe llamarse fuera de BeginMode2D.
    void preparar(const juego::Partida& p, bool iluminacion);

    /// Suelo, edificios y manchas, a escala de @p e.
    void dibujarFondo(const EstiloVista& e) const;
    /// Objetos, almacén, radio, barricadas, señuelos, infectados, superviviente y Mara.
    void dibujarActores(const juego::Partida& p, const EstiloVista& e) const;
    /// Oscuridad: multiplica lo dibujado por la luz (no hace nada si preparar() fue sin luz).
    void dibujarLuz(const EstiloVista& e) const;
    /// Lo que brilla en la oscuridad: ojos de los infectados, ventanas, pantalla de la radio y
    /// destellos de los objetos que faltan por recoger.
    void dibujarEmisivos(const juego::Partida& p, const EstiloVista& e) const;

private:
    struct Mancha {
        float x{}, y{};  ///< En celdas.
        float angulo{};
        std::uint8_t variante{};
    };
    struct Ventana {
        float x{}, y{};  ///< En celdas, esquina superior izquierda.
    };

    void rehacerFondo(const juego::Partida& p);
    void pintarLuz(const juego::Partida& p);
    void sprite(int indice, Vector2 centro, float lado, float grados, Color tinte = WHITE) const;

    Texture2D atlas_{};
    Texture2D fondo_{};
    RenderTexture2D luz_{};
    vista::Escenario escenario_;
    std::uint64_t firma_{0};
    std::uint64_t semilla_{0};
    bool hayFondo_{false};
    bool conLuz_{false};
    std::vector<Ventana> ventanas_;

    // Animación: distancia recorrida por cada infectado, Mara y el superviviente.
    std::vector<navegacion::Vec2> previas_;
    std::vector<float> recorrido_;
    std::uint64_t ultimoTick_{0};

    std::array<Mancha, 96> manchas_{};
    std::size_t numManchas_{0};
    std::size_t siguienteMancha_{0};
    std::uint64_t tickUltimaMancha_{0};
};

}  // namespace refugio::adaptadores
