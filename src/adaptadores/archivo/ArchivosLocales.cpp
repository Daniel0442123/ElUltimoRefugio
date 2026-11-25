#include "adaptadores/archivo/ArchivosLocales.hpp"

#include <cstdio>
#include <system_error>

namespace refugio::adaptadores {

namespace fs = std::filesystem;

LecturaArchivo leerArchivo(const fs::path& ruta, std::uintmax_t maxBytes) {
    LecturaArchivo r;
    std::error_code ec;
    if (!fs::is_regular_file(ruta, ec)) {
        r.existe = fs::exists(ruta, ec);
        if (r.existe) r.error = ruta.generic_string() + " no es un archivo";
        return r;
    }
    r.existe = true;
    const auto tam = fs::file_size(ruta, ec);
    if (ec) {
        r.error = "no se pudo leer el tamaño de " + ruta.generic_string();
        return r;
    }
    if (tam > maxBytes) {  // RNF-021: antes de leer nada
        r.grande = true;
        r.error = ruta.generic_string() + " ocupa más de " + std::to_string(maxBytes / (1024 * 1024)) + " MB";
        return r;
    }
    std::FILE* f = std::fopen(ruta.string().c_str(), "rb");
    if (f == nullptr) {
        r.error = "no se pudo abrir " + ruta.generic_string();
        return r;
    }
    r.datos.resize(static_cast<std::size_t>(tam));
    const std::size_t leidos = tam > 0 ? std::fread(r.datos.data(), 1, r.datos.size(), f) : 0;
    const bool masDatos = std::fgetc(f) != EOF;  // creció mientras se leía
    std::fclose(f);
    if (leidos != r.datos.size() || masDatos) {
        r.error = "lectura incompleta de " + ruta.generic_string();
        r.datos.clear();
        return r;
    }
    r.ok = true;
    return r;
}

std::string escribirAtomico(const fs::path& ruta, std::string_view datos) {
    std::error_code ec;
    if (ruta.has_parent_path()) fs::create_directories(ruta.parent_path(), ec);
    fs::path tmp = ruta;
    tmp += ".tmp";
    std::FILE* f = std::fopen(tmp.string().c_str(), "wb");
    if (f == nullptr) return "no se pudo crear " + tmp.generic_string();
    const bool escrito = std::fwrite(datos.data(), 1, datos.size(), f) == datos.size();
    const bool vaciado = std::fflush(f) == 0;
    const bool cerrado = std::fclose(f) == 0;
    if (!escrito || !vaciado || !cerrado) {
        fs::remove(tmp, ec);
        return "no se pudo escribir " + tmp.generic_string();
    }
    // Renombrar sobre el destino es atómico en el mismo volumen (rename en POSIX, MoveFileEx con
    // MOVEFILE_REPLACE_EXISTING en Windows): el destino es el viejo o el nuevo, nunca uno a medias.
    fs::rename(tmp, ruta, ec);
    if (ec) {
        fs::remove(tmp, ec);
        return "no se pudo reemplazar " + ruta.generic_string();
    }
    return {};
}

fs::path rutaRanura(const fs::path& carpeta, int ranura) { return carpeta / ("slot" + std::to_string(ranura) + ".sav"); }

std::string guardarRanura(const fs::path& carpeta, int ranura, const persistencia::PartidaGuardada& g) {
    const auto bytes = persistencia::codificar(g);
    return escribirAtomico(rutaRanura(carpeta, ranura),
                           std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
}

persistencia::PartidaRestaurada cargarRanura(const fs::path& carpeta, int ranura) {
    persistencia::PartidaRestaurada r;
    const auto l = leerArchivo(rutaRanura(carpeta, ranura));
    if (!l.existe) {
        r.error = persistencia::ErrorCarga::Vacia;
        return r;
    }
    if (!l.ok) {
        r.error = l.grande ? persistencia::ErrorCarga::Grande : persistencia::ErrorCarga::Lectura;
        r.detalle = l.error;
        return r;
    }
    return persistencia::cargar(
        std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(l.datos.data()), l.datos.size()));
}

namespace {
template <class T, class F>
T leerUno(const fs::path& ruta, F analizar, std::vector<std::string>& errores) {
    const auto l = leerArchivo(ruta);
    if (!l.existe) return T{};  // falta: valores por defecto sin queja
    if (!l.ok) {
        errores.push_back(l.error + "; se usan los valores por defecto");
        return T{};
    }
    auto r = analizar(l.datos, ruta.generic_string());
    errores.insert(errores.end(), r.errores.begin(), r.errores.end());
    return r.valor;
}
}  // namespace

ConfigLocal leerConfigLocal(const fs::path& carpeta) {
    ConfigLocal c;
    c.opciones = leerUno<config::OpcionesJuego>(
        carpeta / "juego.json", [](std::string_view t, std::string_view a) { return config::leerOpciones(t, a); }, c.errores);
    c.controles = leerUno<config::Controles>(
        carpeta / "controles.json", [](std::string_view t, std::string_view a) { return config::leerControles(t, a); },
        c.errores);
    c.ia = leerUno<config::ParametrosIa>(
        carpeta / "ia.json", [](std::string_view t, std::string_view a) { return config::leerParametrosIa(t, a); }, c.errores);
    return c;
}

std::string guardarConfigLocal(const fs::path& carpeta, const config::OpcionesJuego& o, const config::Controles& c) {
    std::string e = escribirAtomico(carpeta / "juego.json", config::escribirOpciones(o));
    if (!e.empty()) return e;
    return escribirAtomico(carpeta / "controles.json", config::escribirControles(c));
}

}  // namespace refugio::adaptadores
