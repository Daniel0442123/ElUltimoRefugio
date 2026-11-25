#pragma once
/// @file Json.hpp
/// @brief Lector JSON mínimo para archivos de configuración (RF-080, ADR-0013).
///
/// Lee null, booleanos, números, cadenas (con escapes \" \\ \/ \b \f \n \r \t y \uXXXX ASCII),
/// arreglos y objetos. Es suficiente para bench/config.json y config/*.json y evita una
/// dependencia externa en el dominio. Los errores lanzan ErrorJson con línea y columna.

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace refugio::nucleo {

class ErrorJson : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ValorJson {
public:
    using Arreglo = std::vector<ValorJson>;
    using Objeto = std::map<std::string, ValorJson, std::less<>>;

    ValorJson() = default;
    explicit ValorJson(bool b) : v_(b) {}
    explicit ValorJson(double d) : v_(d) {}
    explicit ValorJson(std::string s) : v_(std::move(s)) {}
    explicit ValorJson(Arreglo a) : v_(std::move(a)) {}
    explicit ValorJson(Objeto o) : v_(std::move(o)) {}

    [[nodiscard]] bool esNulo() const noexcept { return std::holds_alternative<std::monostate>(v_); }
    [[nodiscard]] bool esBool() const noexcept { return std::holds_alternative<bool>(v_); }
    [[nodiscard]] bool esNumero() const noexcept { return std::holds_alternative<double>(v_); }
    [[nodiscard]] bool esCadena() const noexcept { return std::holds_alternative<std::string>(v_); }
    [[nodiscard]] bool esArreglo() const noexcept { return std::holds_alternative<Arreglo>(v_); }
    [[nodiscard]] bool esObjeto() const noexcept { return std::holds_alternative<Objeto>(v_); }

    /// Accesos con tipo: lanzan ErrorJson con @p contexto si el tipo no coincide.
    [[nodiscard]] bool comoBool(std::string_view contexto = "valor") const;
    [[nodiscard]] double comoNumero(std::string_view contexto = "valor") const;
    /// Número entero exacto dentro de [minimo, maximo].
    [[nodiscard]] long long comoEntero(std::string_view contexto, long long minimo, long long maximo) const;
    [[nodiscard]] const std::string& comoCadena(std::string_view contexto = "valor") const;
    [[nodiscard]] const Arreglo& comoArreglo(std::string_view contexto = "valor") const;
    [[nodiscard]] const Objeto& comoObjeto(std::string_view contexto = "valor") const;

    /// Miembro de un objeto o nullptr si no existe (o si no es un objeto).
    [[nodiscard]] const ValorJson* buscar(std::string_view clave) const noexcept;

private:
    std::variant<std::monostate, bool, double, std::string, Arreglo, Objeto> v_;
};

/// Analiza @p texto completo. Lanza ErrorJson («línea L, columna C: …») si no es JSON válido.
[[nodiscard]] ValorJson leerJson(std::string_view texto);

}  // namespace refugio::nucleo
