#include "dominio/experimentos/Optimizacion.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <initializer_list>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>

#include "dominio/experimentos/ExperimentoCoordinacion.hpp"
#include "dominio/juego/Partida.hpp"
#include "dominio/nucleo/Json.hpp"
#include "dominio/nucleo/Pcg32.hpp"

namespace refugio::experimentos {

using nucleo::ErrorJson;
using nucleo::Pcg32;
using nucleo::ValorJson;

namespace {

/// Normal estándar por Box-Muller con el PCG32 (las distribuciones de <random> no son iguales en
/// todas las bibliotecas, ADR-0012).
double normal(Pcg32& rng) noexcept {
    constexpr double kDosPi = 6.283185307179586476925;
    const double u1 = 1.0 - rng.real01();  // (0, 1]: evita log(0)
    const double u2 = rng.real01();
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(kDosPi * u2);
}

double recortar(double v) noexcept { return std::clamp(v, 0.0, 1.0); }

std::vector<double> aleatorio(Pcg32& rng, std::size_t n) {
    std::vector<double> x(n);
    for (auto& v : x) v = rng.real01();
    return x;
}

std::vector<double> ajustar(const std::vector<double>& inicial, std::size_t n, Pcg32& rng) {
    if (inicial.empty()) return aleatorio(rng, n);
    if (inicial.size() != n) throw std::invalid_argument("el individuo inicial no tiene el número de genes indicado");
    std::vector<double> x(inicial);
    for (auto& v : x) v = recortar(v);
    return x;
}

/// Índice del ganador de un torneo de @p k (con reemplazo); en empate gana el índice menor.
std::size_t torneo(const std::vector<double>& aptitud, int k, Pcg32& rng) noexcept {
    const auto n = static_cast<std::uint32_t>(aptitud.size());
    std::size_t mejor = rng.enteroMenorQue(n);
    for (int i = 1; i < k; ++i) {
        const std::size_t c = rng.enteroMenorQue(n);
        if (aptitud[c] > aptitud[mejor] || (aptitud[c] == aptitud[mejor] && c < mejor)) mejor = c;
    }
    return mejor;
}

GeneracionGa resumir(int g, const std::vector<std::vector<double>>& poblacion, const std::vector<double>& aptitud) {
    GeneracionGa r;
    r.generacion = g;
    std::size_t mejor = 0, peor = 0;
    double suma = 0.0;
    for (std::size_t i = 0; i < aptitud.size(); ++i) {
        suma += aptitud[i];
        if (aptitud[i] > aptitud[mejor]) mejor = i;
        if (aptitud[i] < aptitud[peor]) peor = i;
    }
    r.mejor = aptitud[mejor];
    r.peor = aptitud[peor];
    r.media = suma / static_cast<double>(aptitud.size());
    r.mejorIndividuo = poblacion[mejor];
    return r;
}

// ----------------------------------------------------------------------------- Lectura JSON
void soloClaves(const ValorJson& v, std::string_view ctx, std::initializer_list<std::string_view> claves) {
    for (const auto& miembro : v.comoObjeto(ctx))
        if (std::find(claves.begin(), claves.end(), miembro.first) == claves.end())
            throw ErrorJson(std::string(ctx) + ": clave desconocida \"" + miembro.first + "\"");
}

double real(const ValorJson& obj, std::string_view clave, std::string_view ctx, double min, double max, double actual,
            bool minimoAbierto = false) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) return actual;
    const std::string ruta = std::string(ctx) + "." + std::string(clave);
    const double x = v->comoNumero(ruta);
    if (!std::isfinite(x) || x > max || (minimoAbierto ? x <= min : x < min))
        throw ErrorJson(ruta + ": fuera de rango (" + (minimoAbierto ? "(" : "[") + std::to_string(min) + ", " +
                        std::to_string(max) + "])");
    return x;
}

int entero(const ValorJson& obj, std::string_view clave, std::string_view ctx, long long min, long long max, int actual) {
    const ValorJson* v = obj.buscar(clave);
    if (v == nullptr) return actual;
    return static_cast<int>(v->comoEntero(std::string(ctx) + "." + std::string(clave), min, max));
}

// ------------------------------------------------------------------------------ Escritura
void escribirVector(std::ostream& o, const std::vector<double>& x) {
    o << '[';
    for (std::size_t i = 0; i < x.size(); ++i) o << (i ? ", " : "") << x[i];
    o << ']';
}

/// Valores reales (decodificados) de un individuo.
std::vector<double> valores(const std::vector<double>& x) {
    const auto p = decodificar(x);
    std::vector<double> v;
    v.reserve(genes().size());
    for (const auto& g : genes()) v.push_back(static_cast<double>(p.*(g.campo->campo)));
    return v;
}

void escribirMetricas(std::ostream& o, const MetricasAptitud& m) {
    o << "{\"aptitud\": " << m.aptitud << ", \"simulaciones\": " << m.simulaciones << ", \"victorias\": " << m.victorias
      << ", \"tasa_victorias\": " << m.tasaVictorias << ", \"contactos\": " << m.contactos
      << ", \"presion\": " << m.presion << ", \"t_contacto_medio_s\": " << m.tContactoMedio
      << ", \"cobertura\": " << m.cobertura << ", \"sobrecargadas_por_tick_ia\": " << m.sobrecargadasPorTick
      << ", \"duracion_media_s\": " << m.duracionMedia << "}";
}

MetricasAptitud agregar(std::span<const MetricasSimulacion> sims, const ConfigEvaluacion& e, const PesosAptitud& w,
                        int pasosIa) {
    MetricasAptitud m;
    m.simulaciones = static_cast<int>(sims.size());
    if (sims.empty()) return m;
    double sumaContacto = 0.0;
    for (const auto& s : sims) {
        if (s.estado == juego::EstadoPartida::Victoria) ++m.victorias;
        if (s.tContacto >= 0.0) {
            ++m.contactos;
            sumaContacto += s.tContacto;
            m.presion += std::max(0.0, 1.0 - s.tContacto / e.segundos);
        }
        m.cobertura += s.cobertura;
        const double ticksIa = std::max(1.0, static_cast<double>(s.ticks) / pasosIa);
        m.sobrecargadasPorTick += static_cast<double>(s.celdasSobrecargadas) / ticksIa;
        m.duracionMedia += s.duracion;
    }
    const auto n = static_cast<double>(sims.size());
    m.tasaVictorias = m.victorias / n;
    m.presion /= n;
    m.cobertura /= n;
    m.sobrecargadasPorTick /= n;
    m.duracionMedia /= n;
    m.tContactoMedio = m.contactos > 0 ? sumaContacto / m.contactos : 0.0;
    m.aptitud = w.contacto * m.presion + w.cobertura * m.cobertura - w.sobrecarga * m.sobrecargadasPorTick -
                w.dificultad * (std::max(0.0, w.victoriasMinimas - m.tasaVictorias) +
                                std::max(0.0, m.tasaVictorias - w.victoriasMaximas));
    return m;
}

ConfigCoordinacion configSimulacion(const ConfigEvaluacion& e, std::uint64_t semilla, const config::PesosIa& pesos) {
    ConfigCoordinacion c;
    c.semilla = semilla;
    c.repeticiones = e.simulaciones;
    c.ancho = e.ancho;
    c.alto = e.alto;
    c.infectados = e.infectados;
    c.segundos = e.segundos;
    config::aplicar(pesos, c.partida);
    return c;
}

int pasosIa(const ConfigCoordinacion& c) noexcept {
    return std::max(1, c.partida.tickSimHz / std::max(1, c.partida.tickIaHz));
}

/// Ejecuta tarea(i) para i en [0, n) con @p hilos hilos (0 = núcleos lógicos). Cada tarea escribe
/// en su propia posición, así que el resultado no depende del reparto.
template <class F>
void enParalelo(std::size_t n, int hilos, F&& tarea) {
    std::size_t h = hilos > 0 ? static_cast<std::size_t>(hilos) : std::max(1u, std::thread::hardware_concurrency());
    h = std::min(h, n);
    if (h <= 1) {
        for (std::size_t i = 0; i < n; ++i) tarea(i);
        return;
    }
    std::atomic<std::size_t> siguiente{0};
    std::exception_ptr error;
    std::mutex mError;
    auto trabajar = [&] {
        for (std::size_t i = siguiente.fetch_add(1); i < n; i = siguiente.fetch_add(1)) {
            try {
                tarea(i);
            } catch (...) {
                const std::lock_guard<std::mutex> l(mError);
                if (!error) error = std::current_exception();
            }
        }
    };
    std::vector<std::thread> trabajadores;
    trabajadores.reserve(h - 1);
    for (std::size_t i = 1; i < h; ++i) trabajadores.emplace_back(trabajar);
    trabajar();
    for (auto& t : trabajadores) t.join();
    if (error) std::rethrow_exception(error);
}

}  // namespace

std::string_view nombreAlgoritmo(AlgoritmoOptimizacion a) noexcept {
    return a == AlgoritmoOptimizacion::Genetico ? "ga" : "sa";
}

// ---------------------------------------------------------------------------------- Genes
const std::vector<Gen>& genes() {
    static const std::vector<Gen> g = [] {
        // Rangos de búsqueda por clave; cada uno contiene el valor por defecto.
        struct Rango {
            std::string_view clave;
            float minimo, maximo;
        };
        constexpr Rango kRangos[] = {
            {"seguir", 4.0f, 12.0f},           {"separacion", 4.0f, 24.0f},     {"alineacion", 0.0f, 1.5f},
            {"cohesion", 0.0f, 1.5f},          {"anticipacion", 5.0f, 50.0f},   {"utilidad_explorar", 0.05f, 0.5f},
            {"peso_interceptar", 0.3f, 1.5f},  {"histeresis", 1.0f, 1.5f},      {"vision_radio", 2.5f, 8.0f},
            {"vel_perseguir", 1.6f, 4.4f},     {"danio_por_segundo", 4.0f, 16.0f},
        };
        std::vector<Gen> v;
        for (const auto& c : config::camposPesos())
            for (const auto& r : kRangos)
                if (r.clave == c.clave) v.push_back({&c, r.minimo, r.maximo});
        return v;
    }();
    return g;
}

std::string nombreGen(const Gen& g) { return std::string(g.campo->seccion) + "." + std::string(g.campo->clave); }

std::vector<double> codificar(const config::PesosIa& p) {
    std::vector<double> x;
    x.reserve(genes().size());
    for (const auto& g : genes()) {
        const double v = static_cast<double>(p.*(g.campo->campo));
        x.push_back(recortar((v - g.minimo) / (static_cast<double>(g.maximo) - g.minimo)));
    }
    return x;
}

config::PesosIa decodificar(std::span<const double> x) {
    config::PesosIa p;
    const auto& gs = genes();
    for (std::size_t i = 0; i < gs.size() && i < x.size(); ++i) {
        const auto& g = gs[i];
        p.*(g.campo->campo) = static_cast<float>(g.minimo + recortar(x[i]) * (static_cast<double>(g.maximo) - g.minimo));
    }
    return p;
}

// --------------------------------------------------------------------------------- GA
ResultadoGa ejecutarGa(const ParametrosGa& p, std::size_t numGenes, std::uint64_t semilla, const Evaluador& evaluar,
                       const std::vector<double>& inicial) {
    if (p.poblacion < 2 || p.generaciones < 1 || p.torneo < 1 || numGenes == 0)
        throw std::invalid_argument("parámetros del algoritmo genético inválidos");
    Pcg32 rng(semilla, kFlujoOptimizacion);
    const auto n = static_cast<std::size_t>(p.poblacion);
    const auto elites = static_cast<std::size_t>(std::clamp(p.elites, 0, p.poblacion - 1));

    std::vector<std::vector<double>> poblacion;
    poblacion.reserve(n);
    poblacion.push_back(ajustar(inicial, numGenes, rng));
    while (poblacion.size() < n) poblacion.push_back(aleatorio(rng, numGenes));
    std::vector<double> aptitud = evaluar(poblacion);

    ResultadoGa r;
    r.evaluaciones = static_cast<int>(n);
    r.generaciones.push_back(resumir(0, poblacion, aptitud));

    std::vector<std::size_t> orden(n);
    for (int g = 1; g < p.generaciones; ++g) {
        std::iota(orden.begin(), orden.end(), std::size_t{0});
        std::stable_sort(orden.begin(), orden.end(), [&](std::size_t a, std::size_t b) { return aptitud[a] > aptitud[b]; });

        std::vector<std::vector<double>> nueva;
        std::vector<double> aptNueva;
        nueva.reserve(n);
        for (std::size_t e = 0; e < elites; ++e) {
            nueva.push_back(poblacion[orden[e]]);
            aptNueva.push_back(aptitud[orden[e]]);
        }
        std::vector<std::vector<double>> hijos;
        while (nueva.size() + hijos.size() < n) {
            const auto& a = poblacion[torneo(aptitud, p.torneo, rng)];
            const auto& b = poblacion[torneo(aptitud, p.torneo, rng)];
            std::vector<double> h(a);
            if (rng.real01() < p.probCruce)
                for (std::size_t i = 0; i < numGenes; ++i)
                    if (rng.real01() < 0.5) h[i] = b[i];
            for (auto& v : h)
                if (rng.real01() < p.probMutacion) v = recortar(v + p.sigma * normal(rng));
            hijos.push_back(std::move(h));
        }
        const auto aptHijos = evaluar(hijos);
        r.evaluaciones += static_cast<int>(hijos.size());
        for (std::size_t i = 0; i < hijos.size(); ++i) {
            nueva.push_back(std::move(hijos[i]));
            aptNueva.push_back(aptHijos[i]);
        }
        poblacion = std::move(nueva);
        aptitud = std::move(aptNueva);
        r.generaciones.push_back(resumir(g, poblacion, aptitud));
    }
    // Mejor de toda la ejecución (con elitismo es el de la última generación si elites > 0).
    const auto mejor = std::max_element(r.generaciones.begin(), r.generaciones.end(),
                                        [](const GeneracionGa& a, const GeneracionGa& b) { return a.mejor < b.mejor; });
    r.mejor = mejor->mejorIndividuo;
    r.aptitudMejor = mejor->mejor;
    return r;
}

// --------------------------------------------------------------------------------- SA
ResultadoSa ejecutarSa(const ParametrosSa& p, std::size_t numGenes, std::uint64_t semilla, const Evaluador& evaluar,
                       const std::vector<double>& inicial) {
    if (p.iteraciones < 1 || !(p.temperaturaInicial > 0.0) || !(p.alfa > 0.0 && p.alfa <= 1.0) || numGenes == 0)
        throw std::invalid_argument("parámetros del recocido simulado inválidos");
    Pcg32 rng(semilla, kFlujoOptimizacion);
    std::vector<double> actual = ajustar(inicial, numGenes, rng);
    double fActual = evaluar({actual}).at(0);

    ResultadoSa r;
    r.evaluaciones = 1;
    r.mejor = actual;
    r.aptitudMejor = fActual;
    r.iteraciones.reserve(static_cast<std::size_t>(p.iteraciones) + 1);
    r.iteraciones.push_back({0, p.temperaturaInicial, fActual, fActual, fActual, true, actual});

    double t = p.temperaturaInicial;
    for (int i = 1; i <= p.iteraciones; ++i) {
        std::vector<double> vecino(actual);
        for (auto& v : vecino) v = recortar(v + p.sigma * normal(rng));
        const double fVecino = evaluar({vecino}).at(0);
        ++r.evaluaciones;
        const double delta = fVecino - fActual;
        const double u = rng.real01();  // siempre se extrae: la secuencia no depende de las aptitudes
        const bool acepta = delta >= 0.0 || u < std::exp(delta / t);
        if (acepta) {
            ++r.aceptadas;
            if (delta < 0.0) ++r.empeoramientosAceptados;
            actual = std::move(vecino);
            fActual = fVecino;
            if (fActual > r.aptitudMejor) {
                r.aptitudMejor = fActual;
                r.mejor = actual;
            }
        }
        r.iteraciones.push_back({i, t, fVecino, fActual, r.aptitudMejor, acepta, actual});
        t *= p.alfa;
    }
    return r;
}

// ----------------------------------------------------------------------------- Aptitud
MetricasAptitud evaluarPesos(const ConfigEvaluacion& e, const PesosAptitud& w, std::uint64_t semilla,
                             const config::PesosIa& pesos) {
    const auto c = configSimulacion(e, semilla, pesos);
    std::vector<MetricasSimulacion> sims(static_cast<std::size_t>(std::max(0, e.simulaciones)));
    enParalelo(sims.size(), e.hilos,
               [&](std::size_t i) { sims[i] = simularHorda(c, ModoHorda::Coordinado, static_cast<int>(i)); });
    return agregar(sims, e, w, pasosIa(c));
}

Evaluador evaluadorSimulacion(const ConfigEvaluacion& e, const PesosAptitud& w, std::uint64_t semilla) {
    return [e, w, semilla](const std::vector<std::vector<double>>& candidatos) {
        const auto s = static_cast<std::size_t>(std::max(0, e.simulaciones));
        std::vector<ConfigCoordinacion> cfgs;
        cfgs.reserve(candidatos.size());
        for (const auto& x : candidatos) cfgs.push_back(configSimulacion(e, semilla, decodificar(x)));
        std::vector<MetricasSimulacion> sims(candidatos.size() * s);
        enParalelo(sims.size(), e.hilos, [&](std::size_t k) {
            sims[k] = simularHorda(cfgs[k / s], ModoHorda::Coordinado, static_cast<int>(k % s));
        });
        std::vector<double> aptitud;
        aptitud.reserve(candidatos.size());
        for (std::size_t c = 0; c < candidatos.size(); ++c)
            aptitud.push_back(agregar(std::span(sims).subspan(c * s, s), e, w, pasosIa(cfgs[c])).aptitud);
        return aptitud;
    };
}

// ----------------------------------------------------------------------------- Config
ConfigOptimizacion leerConfigOptimizacion(std::string_view json) {
    const ValorJson raiz = nucleo::leerJson(json);
    soloClaves(raiz, "config", {"algoritmo", "semilla", "salida", "descripcion", "ga", "sa", "evaluacion", "aptitud"});
    ConfigOptimizacion c;
    if (const auto* v = raiz.buscar("algoritmo")) {
        const auto& a = v->comoCadena("algoritmo");
        if (a == "ga") c.algoritmo = AlgoritmoOptimizacion::Genetico;
        else if (a == "sa") c.algoritmo = AlgoritmoOptimizacion::Recocido;
        else throw ErrorJson("algoritmo: debe ser \"ga\" o \"sa\"");
    }
    if (const auto* v = raiz.buscar("semilla"))
        c.semilla = static_cast<std::uint64_t>(v->comoEntero("semilla", 0, 9007199254740991LL));
    if (const auto* v = raiz.buscar("salida")) c.salida = v->comoCadena("salida");
    if (const auto* v = raiz.buscar("descripcion")) (void)v->comoCadena("descripcion");
    if (const auto* s = raiz.buscar("ga")) {
        soloClaves(*s, "ga", {"poblacion", "generaciones", "torneo", "prob_cruce", "prob_mutacion", "sigma", "elites"});
        auto& g = c.ga;
        g.poblacion = entero(*s, "poblacion", "ga", 2, 10000, g.poblacion);
        g.generaciones = entero(*s, "generaciones", "ga", 1, 100000, g.generaciones);
        g.torneo = entero(*s, "torneo", "ga", 1, g.poblacion, g.torneo);
        g.probCruce = real(*s, "prob_cruce", "ga", 0.0, 1.0, g.probCruce);
        g.probMutacion = real(*s, "prob_mutacion", "ga", 0.0, 1.0, g.probMutacion);
        g.sigma = real(*s, "sigma", "ga", 0.0, 1.0, g.sigma, true);
        g.elites = entero(*s, "elites", "ga", 0, g.poblacion - 1, g.elites);
    }
    if (c.ga.torneo > c.ga.poblacion) throw ErrorJson("ga.torneo: no puede superar a ga.poblacion");
    if (c.ga.elites >= c.ga.poblacion) throw ErrorJson("ga.elites: debe ser menor que ga.poblacion");
    if (const auto* s = raiz.buscar("sa")) {
        soloClaves(*s, "sa", {"iteraciones", "temperatura_inicial", "alfa", "sigma"});
        auto& a = c.sa;
        a.iteraciones = entero(*s, "iteraciones", "sa", 1, 10000000, a.iteraciones);
        a.temperaturaInicial = real(*s, "temperatura_inicial", "sa", 0.0, 1e6, a.temperaturaInicial, true);
        a.alfa = real(*s, "alfa", "sa", 0.0, 1.0, a.alfa, true);
        a.sigma = real(*s, "sigma", "sa", 0.0, 1.0, a.sigma, true);
    }
    if (const auto* s = raiz.buscar("evaluacion")) {
        soloClaves(*s, "evaluacion", {"simulaciones", "segundos", "ancho", "alto", "infectados", "hilos"});
        auto& e = c.evaluacion;
        e.simulaciones = entero(*s, "simulaciones", "evaluacion", 1, 1000, e.simulaciones);
        e.segundos = real(*s, "segundos", "evaluacion", 0.0, 3600.0, e.segundos, true);
        e.ancho = entero(*s, "ancho", "evaluacion", 16, 256, e.ancho);
        e.alto = entero(*s, "alto", "evaluacion", 16, 256, e.alto);
        e.infectados = entero(*s, "infectados", "evaluacion", 1, 256, e.infectados);
        e.hilos = entero(*s, "hilos", "evaluacion", 0, 256, e.hilos);
    }
    if (const auto* s = raiz.buscar("aptitud")) {
        soloClaves(*s, "aptitud", {"contacto", "cobertura", "sobrecarga", "dificultad", "victorias_minimas", "victorias_maximas"});
        auto& w = c.aptitud;
        w.contacto = real(*s, "contacto", "aptitud", 0.0, 1000.0, w.contacto);
        w.cobertura = real(*s, "cobertura", "aptitud", 0.0, 1000.0, w.cobertura);
        w.sobrecarga = real(*s, "sobrecarga", "aptitud", 0.0, 1000.0, w.sobrecarga);
        w.dificultad = real(*s, "dificultad", "aptitud", 0.0, 1000.0, w.dificultad);
        w.victoriasMinimas = real(*s, "victorias_minimas", "aptitud", 0.0, 1.0, w.victoriasMinimas);
        w.victoriasMaximas = real(*s, "victorias_maximas", "aptitud", 0.0, 1.0, w.victoriasMaximas);
    }
    if (c.aptitud.victoriasMaximas < c.aptitud.victoriasMinimas)
        throw ErrorJson("aptitud.victorias_maximas: no puede ser menor que aptitud.victorias_minimas");
    return c;
}

// --------------------------------------------------------------------------- Ejecución
ResumenOptimizacion ejecutarOptimizacion(const ConfigOptimizacion& cfg, const std::vector<std::string>& metadatos,
                                         std::ostream& json) {
    const auto& gs = genes();
    const auto evaluar = evaluadorSimulacion(cfg.evaluacion, cfg.aptitud, cfg.semilla);
    const auto inicial = codificar(cfg.inicial);
    const bool ga = cfg.algoritmo == AlgoritmoOptimizacion::Genetico;

    ResumenOptimizacion r;
    r.base = evaluarPesos(cfg.evaluacion, cfg.aptitud, cfg.semilla, decodificar(inicial));
    ResultadoGa resGa;
    ResultadoSa resSa;
    std::vector<double> mejor;
    if (ga) {
        resGa = ejecutarGa(cfg.ga, gs.size(), cfg.semilla, evaluar, inicial);
        mejor = resGa.mejor;
        r.evaluaciones = resGa.evaluaciones;
        for (std::size_t i = 1; i < resGa.generaciones.size(); ++i)
            if (resGa.generaciones[i].mejor < resGa.generaciones[i - 1].mejor) r.mejorNoDecreciente = false;
    } else {
        resSa = ejecutarSa(cfg.sa, gs.size(), cfg.semilla, evaluar, inicial);
        mejor = resSa.mejor;
        r.evaluaciones = resSa.evaluaciones;
    }
    r.pesosMejor = decodificar(mejor);
    r.mejor = evaluarPesos(cfg.evaluacion, cfg.aptitud, cfg.semilla, r.pesosMejor);
    r.simulaciones = (r.evaluaciones + 2) * cfg.evaluacion.simulaciones;

    // ------------------------------------------------------------------- JSON (RF-085)
    json << std::setprecision(10);
    json << "{\n  \"metadatos\": [";
    for (std::size_t i = 0; i < metadatos.size(); ++i) {
        json << (i ? ", " : "") << '"';
        for (const char ch : metadatos[i]) {
            if (ch == '"' || ch == '\\') json << '\\';
            if (static_cast<unsigned char>(ch) >= 0x20) json << ch;
        }
        json << '"';
    }
    json << "],\n";
    json << "  \"algoritmo\": \"" << nombreAlgoritmo(cfg.algoritmo) << "\",\n";
    json << "  \"semilla\": " << cfg.semilla << ",\n";
    if (ga) {
        const auto& p = cfg.ga;
        json << "  \"ga\": {\"poblacion\": " << p.poblacion << ", \"generaciones\": " << p.generaciones
             << ", \"torneo\": " << p.torneo << ", \"prob_cruce\": " << p.probCruce
             << ", \"prob_mutacion\": " << p.probMutacion << ", \"sigma\": " << p.sigma << ", \"elites\": " << p.elites
             << "},\n";
    } else {
        const auto& p = cfg.sa;
        json << "  \"sa\": {\"iteraciones\": " << p.iteraciones << ", \"temperatura_inicial\": " << p.temperaturaInicial
             << ", \"alfa\": " << p.alfa << ", \"sigma\": " << p.sigma << "},\n";
    }
    const auto& e = cfg.evaluacion;
    json << "  \"evaluacion\": {\"simulaciones\": " << e.simulaciones << ", \"segundos\": " << e.segundos
         << ", \"mapa\": \"ciudad_" << e.ancho << 'x' << e.alto << "\", \"infectados\": " << e.infectados
         << ", \"modo\": \"coordinado\", \"guion\": \"objetos-radio-extraccion\", \"semillas\": [" << cfg.semilla
         << ", " << cfg.semilla + static_cast<std::uint64_t>(std::max(0, e.simulaciones - 1)) << "]},\n";
    const auto& w = cfg.aptitud;
    json << "  \"aptitud\": {\"formula\": \"contacto*presion + cobertura*cobertura - sobrecarga*sobrecargadas_por_tick_ia"
            " - dificultad*(max(0, victorias_minimas - tasa_victorias) + max(0, tasa_victorias - victorias_maximas))\","
            " \"contacto\": "
         << w.contacto << ", \"cobertura\": " << w.cobertura << ", \"sobrecarga\": " << w.sobrecarga
         << ", \"dificultad\": " << w.dificultad << ", \"victorias_minimas\": " << w.victoriasMinimas
         << ", \"victorias_maximas\": " << w.victoriasMaximas << "},\n";
    json << "  \"genes\": [";
    for (std::size_t i = 0; i < gs.size(); ++i)
        json << (i ? ", " : "") << "{\"nombre\": \"" << nombreGen(gs[i]) << "\", \"min\": " << gs[i].minimo
             << ", \"max\": " << gs[i].maximo << '}';
    json << "],\n";
    json << "  \"evaluaciones\": " << r.evaluaciones << ",\n";
    json << "  \"simulaciones\": " << r.simulaciones << ",\n";
    json << "  \"base\": {\"metricas\": ";
    escribirMetricas(json, r.base);
    json << ", \"valores\": ";
    escribirVector(json, valores(inicial));
    json << "},\n";
    json << "  \"mejor\": {\"aptitud\": " << r.mejor.aptitud << ", \"metricas\": ";
    escribirMetricas(json, r.mejor);
    json << ", \"valores\": ";
    escribirVector(json, valores(mejor));
    json << ",\n    \"pesos\": " << config::escribirPesos(r.pesosMejor, 4) << "},\n";
    if (ga) {
        json << "  \"mejor_no_decreciente\": " << (r.mejorNoDecreciente ? "true" : "false") << ",\n";
        json << "  \"generaciones\": [\n";
        for (std::size_t i = 0; i < resGa.generaciones.size(); ++i) {
            const auto& g = resGa.generaciones[i];
            json << "    {\"generacion\": " << g.generacion << ", \"mejor\": " << g.mejor << ", \"media\": " << g.media
                 << ", \"peor\": " << g.peor << ", \"mejor_individuo\": ";
            escribirVector(json, valores(g.mejorIndividuo));
            json << '}' << (i + 1 < resGa.generaciones.size() ? "," : "") << '\n';
        }
        json << "  ]\n";
    } else {
        json << "  \"aceptadas\": " << resSa.aceptadas << ",\n";
        json << "  \"empeoramientos_aceptados\": " << resSa.empeoramientosAceptados << ",\n";
        json << "  \"iteraciones\": [\n";
        for (std::size_t i = 0; i < resSa.iteraciones.size(); ++i) {
            const auto& it = resSa.iteraciones[i];
            json << "    {\"iteracion\": " << it.iteracion << ", \"temperatura\": " << it.temperatura
                 << ", \"vecino\": " << it.aptitudVecino << ", \"actual\": " << it.aptitudActual
                 << ", \"mejor\": " << it.aptitudMejor << ", \"aceptada\": " << (it.aceptada ? "true" : "false")
                 << ", \"solucion_actual\": ";
            escribirVector(json, valores(it.actual));
            json << '}' << (i + 1 < resSa.iteraciones.size() ? "," : "") << '\n';
        }
        json << "  ]\n";
    }
    json << "}\n";
    return r;
}

}  // namespace refugio::experimentos
