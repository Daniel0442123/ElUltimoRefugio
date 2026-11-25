#include "dominio/nucleo/Json.hpp"

#include <cmath>
#include <cstdlib>

namespace refugio::nucleo {
namespace {

std::string tipoDe(const ValorJson& v) {
    if (v.esNulo()) return "null";
    if (v.esBool()) return "booleano";
    if (v.esNumero()) return "numero";
    if (v.esCadena()) return "cadena";
    if (v.esArreglo()) return "arreglo";
    return "objeto";
}

[[noreturn]] void tipoIncorrecto(std::string_view contexto, const char* esperado, const ValorJson& v) {
    throw ErrorJson(std::string(contexto) + ": se esperaba " + esperado + " y hay " + tipoDe(v));
}

/// Analizador descendente recursivo. La profundidad se limita para no agotar la pila con
/// archivos maliciosos o corruptos (RNF de robustez, igual que el resto de recursiones).
class Analizador {
public:
    explicit Analizador(std::string_view t) : t_(t) {}

    ValorJson documento() {
        ValorJson v = valor(0);
        espacios();
        if (i_ != t_.size()) error("contenido sobrante tras el valor");
        return v;
    }

private:
    static constexpr int kProfundidadMax = 64;

    [[noreturn]] void error(const std::string& msg) const {
        std::size_t linea = 1;
        std::size_t col = 1;
        for (std::size_t k = 0; k < i_ && k < t_.size(); ++k) {
            if (t_[k] == '\n') {
                ++linea;
                col = 1;
            } else {
                ++col;
            }
        }
        throw ErrorJson("JSON invalido (linea " + std::to_string(linea) + ", columna " + std::to_string(col) +
                        "): " + msg);
    }

    void espacios() noexcept {
        while (i_ < t_.size() && (t_[i_] == ' ' || t_[i_] == '\t' || t_[i_] == '\n' || t_[i_] == '\r')) ++i_;
    }

    bool literal(std::string_view palabra) noexcept {
        if (t_.substr(i_, palabra.size()) != palabra) return false;
        i_ += palabra.size();
        return true;
    }

    ValorJson valor(int profundidad) {
        if (profundidad > kProfundidadMax) error("anidamiento demasiado profundo");
        espacios();
        if (i_ >= t_.size()) error("fin inesperado");
        const char c = t_[i_];
        if (c == '{') return objeto(profundidad);
        if (c == '[') return arreglo(profundidad);
        if (c == '"') return ValorJson(cadena());
        if (literal("true")) return ValorJson(true);
        if (literal("false")) return ValorJson(false);
        if (literal("null")) return ValorJson();
        if (c == '-' || (c >= '0' && c <= '9')) return ValorJson(numero());
        error(std::string("caracter inesperado '") + c + "'");
    }

    ValorJson objeto(int profundidad) {
        ++i_;  // {
        ValorJson::Objeto o;
        espacios();
        if (i_ < t_.size() && t_[i_] == '}') {
            ++i_;
            return ValorJson(std::move(o));
        }
        for (;;) {
            espacios();
            if (i_ >= t_.size() || t_[i_] != '"') error("se esperaba una clave entre comillas");
            std::string clave = cadena();
            espacios();
            if (i_ >= t_.size() || t_[i_] != ':') error("se esperaba ':' tras la clave \"" + clave + "\"");
            ++i_;
            ValorJson v = valor(profundidad + 1);
            if (!o.emplace(clave, std::move(v)).second) error("clave repetida \"" + clave + "\"");
            espacios();
            if (i_ < t_.size() && t_[i_] == ',') {
                ++i_;
                continue;
            }
            if (i_ < t_.size() && t_[i_] == '}') {
                ++i_;
                return ValorJson(std::move(o));
            }
            error("se esperaba ',' o '}'");
        }
    }

    ValorJson arreglo(int profundidad) {
        ++i_;  // [
        ValorJson::Arreglo a;
        espacios();
        if (i_ < t_.size() && t_[i_] == ']') {
            ++i_;
            return ValorJson(std::move(a));
        }
        for (;;) {
            a.push_back(valor(profundidad + 1));
            espacios();
            if (i_ < t_.size() && t_[i_] == ',') {
                ++i_;
                continue;
            }
            if (i_ < t_.size() && t_[i_] == ']') {
                ++i_;
                return ValorJson(std::move(a));
            }
            error("se esperaba ',' o ']'");
        }
    }

    std::string cadena() {
        ++i_;  // "
        std::string s;
        while (i_ < t_.size()) {
            const char c = t_[i_++];
            if (c == '"') return s;
            if (static_cast<unsigned char>(c) < 0x20) error("caracter de control dentro de una cadena");
            if (c != '\\') {
                s.push_back(c);
                continue;
            }
            if (i_ >= t_.size()) break;
            const char e = t_[i_++];
            switch (e) {
                case '"': s.push_back('"'); break;
                case '\\': s.push_back('\\'); break;
                case '/': s.push_back('/'); break;
                case 'b': s.push_back('\b'); break;
                case 'f': s.push_back('\f'); break;
                case 'n': s.push_back('\n'); break;
                case 'r': s.push_back('\r'); break;
                case 't': s.push_back('\t'); break;
                case 'u': {
                    if (i_ + 4 > t_.size()) error("escape \\u incompleto");
                    unsigned codigo = 0;
                    for (int k = 0; k < 4; ++k) {
                        const char h = t_[i_++];
                        codigo <<= 4;
                        if (h >= '0' && h <= '9') codigo |= static_cast<unsigned>(h - '0');
                        else if (h >= 'a' && h <= 'f') codigo |= static_cast<unsigned>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') codigo |= static_cast<unsigned>(h - 'A' + 10);
                        else error("escape \\u con digito no hexadecimal");
                    }
                    // UTF-8 de un punto del plano básico (sin pares sustitutos: basta para configuración).
                    if (codigo < 0x80) {
                        s.push_back(static_cast<char>(codigo));
                    } else if (codigo < 0x800) {
                        s.push_back(static_cast<char>(0xC0 | (codigo >> 6)));
                        s.push_back(static_cast<char>(0x80 | (codigo & 0x3F)));
                    } else {
                        s.push_back(static_cast<char>(0xE0 | (codigo >> 12)));
                        s.push_back(static_cast<char>(0x80 | ((codigo >> 6) & 0x3F)));
                        s.push_back(static_cast<char>(0x80 | (codigo & 0x3F)));
                    }
                    break;
                }
                default: error(std::string("escape desconocido \\") + e);
            }
        }
        error("cadena sin cerrar");
    }

    double numero() {
        const std::size_t inicio = i_;
        if (t_[i_] == '-') ++i_;
        auto digitos = [&] {
            const std::size_t d = i_;
            while (i_ < t_.size() && t_[i_] >= '0' && t_[i_] <= '9') ++i_;
            return i_ > d;
        };
        if (!digitos()) error("numero sin digitos");
        if (i_ < t_.size() && t_[i_] == '.') {
            ++i_;
            if (!digitos()) error("numero sin decimales tras el punto");
        }
        if (i_ < t_.size() && (t_[i_] == 'e' || t_[i_] == 'E')) {
            ++i_;
            if (i_ < t_.size() && (t_[i_] == '+' || t_[i_] == '-')) ++i_;
            if (!digitos()) error("exponente sin digitos");
        }
        const std::string texto(t_.substr(inicio, i_ - inicio));
        char* fin = nullptr;
        const double d = std::strtod(texto.c_str(), &fin);
        if (fin != texto.c_str() + texto.size() || !std::isfinite(d)) error("numero fuera de rango");
        return d;
    }

    std::string_view t_;
    std::size_t i_{0};
};

}  // namespace

bool ValorJson::comoBool(std::string_view contexto) const {
    if (!esBool()) tipoIncorrecto(contexto, "un booleano", *this);
    return std::get<bool>(v_);
}

double ValorJson::comoNumero(std::string_view contexto) const {
    if (!esNumero()) tipoIncorrecto(contexto, "un numero", *this);
    return std::get<double>(v_);
}

long long ValorJson::comoEntero(std::string_view contexto, long long minimo, long long maximo) const {
    const double d = comoNumero(contexto);
    if (d != std::floor(d)) throw ErrorJson(std::string(contexto) + ": se esperaba un entero");
    if (d < static_cast<double>(minimo) || d > static_cast<double>(maximo))
        throw ErrorJson(std::string(contexto) + ": debe estar entre " + std::to_string(minimo) + " y " +
                        std::to_string(maximo));
    return static_cast<long long>(d);
}

const std::string& ValorJson::comoCadena(std::string_view contexto) const {
    if (!esCadena()) tipoIncorrecto(contexto, "una cadena", *this);
    return std::get<std::string>(v_);
}

const ValorJson::Arreglo& ValorJson::comoArreglo(std::string_view contexto) const {
    if (!esArreglo()) tipoIncorrecto(contexto, "un arreglo", *this);
    return std::get<Arreglo>(v_);
}

const ValorJson::Objeto& ValorJson::comoObjeto(std::string_view contexto) const {
    if (!esObjeto()) tipoIncorrecto(contexto, "un objeto", *this);
    return std::get<Objeto>(v_);
}

const ValorJson* ValorJson::buscar(std::string_view clave) const noexcept {
    const auto* o = std::get_if<Objeto>(&v_);
    if (o == nullptr) return nullptr;
    const auto it = o->find(clave);
    return it == o->end() ? nullptr : &it->second;
}

ValorJson leerJson(std::string_view texto) { return Analizador(texto).documento(); }

}  // namespace refugio::nucleo
