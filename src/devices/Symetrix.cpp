#include "devices/Symetrix.hpp"
#include "system/SystemMgr.hpp"

// Symetrix Composer solo se puede descargar para Windows/Mac. Se usan las funciones de socket de Windows directamente.
#ifdef WIN32

    // Definiciones por si no están definidos SYS_INFO ni SYS_WARN de SystemMgr
    #ifdef SYS_INFO
        #include <iostream>
        #define SYS_INFO(module,msg) std::cout << module << msg << std::endl;
    #endif
    #ifndef SYS_WARN
        #include <iostream>
        #define SYS_WARN(module,msg) std::cerr << module << msg << std::endl;
    #endif

    // Forzar definiciones de compilación necesarias si no están activas
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #include <winsock2.h>       ///< Proporciona la funcionalidad base de Windows Sockets API (Winsock 2).
    #include <ws2tcpip.h>       ///< Extensiones de Winsock para protocolos TCP/IP, incluye funciones como inet_pton.

    #include <charconv>         ///< Conversiones primitivas de bajo nivel entre números y cadenas (desde C++17).
    #include <cmath>            ///< Funciones matemáticas comunes (seno, coseno, potencia, etc.).
    #include <cstdlib>          ///< Utilidades generales de C (gestión de memoria, conversión de tipos, aleatorios).
    #include <cstring>          ///< Funciones para manipulación de cadenas y bloques de memoria al estilo C.

    #include <string>           ///< Clase std::string para manipulación de cadenas de texto.
    #include <vector>           ///< Contenedor de secuencia dinámica (std::vector).
    #include <algorithm>        ///< std::clamp


    struct Symetrix::Impl {
        SOCKET  socket     = INVALID_SOCKET;   ///< Socket UDP para el envío de paquetes a Symetrix.
    };


    // General ------------------------------------------------------------------------------

    Symetrix::Symetrix() :
        pimpl_(std::make_unique<Impl>()),
        wsaStarted_(false),
        connected_(false),
        initialized_(false),
        connection_ping_timeout_ms_(500),
        ComposerPort_(48631),
        tolerance_percent_(2),            // Por defecto tolerancias de un 2% sobre el rango
        dBcurve_gamma_(0.415f),
        supermatrix_ins_(20),             // Por defecto para tener 20 entradas (se puede cambiar por método)
        supermatrix_outs_(20),            // Por defecto para tener 20 salidas (se puede cambiar por método)
        kBootPreset_(1)
    {

    }

    Symetrix::~Symetrix() {
        Destroy();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool Symetrix::Init(std::wstring const& SymetrixIP, bool force) {

        // No hacer nada si ya está inicializado y no se fuerza el init
        if (initialized_ && !force) return true;

        // Si entra aquí, se ha forzado la inicialización. Limpiar lo que hubiera
        if (initialized_) {
            initialized_ = false;
            net_cleanup();
        }

        // Inicializar y probar conexión de red con Symetrix
        if(!initConnection(SymetrixIP))
            return false;

        // Cargar preset de boot por defecto
        if (kBootPreset_ > 0)
            LoadPreset(kBootPreset_);

        initialized_ = true;
        return true;
    }

    void Symetrix::Destroy() {

        // Si no está inicializado no hay que destruir nada
        if (!initialized_) return;

        // Cierra Socket y WSA
        net_cleanup();

        initialized_ = false;
    }

    bool Symetrix::isConnected() const {
        return connected_;
    }


    // Parámetros ---------------------------------------------------------------------------

    void Symetrix::setSupermatrixINs(unsigned int num) {
        supermatrix_ins_ = num;
    }

    void Symetrix::setSupermatrixOUTs(unsigned int num) {
        supermatrix_outs_ = num;
    }


    // Comandos -----------------------------------------------------------------------------

    bool Symetrix::LoadPreset(unsigned int preset) {

        // Comprueba si está inicializado
        if (!initialized_) {
            return false;
        }

        // Comprueba si el socket a symetrix está correcto
        if (pimpl_->socket == INVALID_SOCKET) {
            return false;
        }

        // El preset no puede ser mayor a 1000
        if (preset > 1000) {
            return false;
        }

        // Comando oficial: LP <PRESET><CR>. Puedo usar $q para evitar tráfico.
        // Ref: (LP) Load Preset — responde ACK/NAK. Nosotros no leemos (UDP/Quiet).
        char cmd[32];
        int len = std::snprintf(cmd, sizeof(cmd), "$q LP %u\r", preset);

        // Verificación de seguridad del buffer (snprintf devuelve el número de caracteres requeridos)
        if (len <= 0 || len >= static_cast<int>(sizeof(cmd))) {
            return false;
        }

        // Envío por red
        int sent = send(pimpl_->socket, cmd, len, 0);
        if (sent == SOCKET_ERROR || sent != len) {
            return false;
        }
        return true;

    }


    // Componentes: Controles generales -----------------------------------------------------

    bool Symetrix::setValue(unsigned int id, float value, float minValue, float maxValue) {

        // Comprobar si se pueden mandar los datos
        if (!connected_ || id == 0) return false;

        // Comprobaciones de seguridad
        if (minValue > maxValue) {
            SYS_WARN("Symetrix","Max value lower than min value");
            return false;
        }
        if (maxValue < minValue) {
            SYS_WARN("Symetrix","Min value higher than max value");
            return false;
        }
        if (maxValue == minValue) {
            SYS_WARN("Symetrix","Min value equal to max value");
            return false;
        }

        // Truncamiento de valores máximo/mínimo
        value = std::clamp(value, minValue, maxValue);

        // Obtener el valor en la escala de ticks de Symetrix
        const int ticks = ValueToTicks(value, minValue, maxValue);

        // Comprobar variación respecto a tolerancia para mandar o no
        if (!shouldSendCSQ(id, ticks))
            return true;    // Considera enviado

        // Mandar comando por socket
        if (!sendCSQ(id, ticks))
            return false;

        // Si la entrada de cache no existe, guardar primero el valor de tolerancia por defecto
        if (!isCached(id)) {
            unsigned int tolerance = 
                static_cast<unsigned int>(std::round((maxTickValue_ - minTickValue_) * (tolerance_percent_ / 100.0f)));
            setTolerance(id, tolerance);
        }

        // Guardar el valor nuevo en la cache
        cacheValue(id, ticks);
        return true;
    }

    bool Symetrix::setValue_dB(unsigned int id, float value, float minValue, float maxValue) {

        // Comprobar si se pueden mandar los datos
        if (!connected_ || id == 0) return false;

        // Comprobaciones de seguridad
        if (minValue > maxValue) {
            SYS_WARN("Symetrix","Max value lower than min value");
            return false;
        }
        if (maxValue < minValue) {
            SYS_WARN("Symetrix","Min value higher than max value");
            return false;
        }
        if (maxValue == minValue) {
            SYS_WARN("Symetrix","Min value equal to max value");
            return false;
        }

        // Truncamiento de valores máximo/mínimo
        value = std::clamp(value, minValue, maxValue);

        // Normalización del valor de entrada a ratio 0 a 1
        float ratio = (value - minValue) / (maxValue - minValue);
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        // Interpolación usando la posición en la curva logarítmica
        float dbValue = SYM_GAIN_MIN + (std::pow(ratio, dBcurve_gamma_) * (SYM_GAIN_MAX - SYM_GAIN_MIN));

        // Obtener el valor en la escala de ticks de Symetrix con escala de dBs de Symetrix
        const int ticks = ValueToTicks(dbValue, SYM_GAIN_MIN, SYM_GAIN_MAX);

        // Comprobar variación respecto a tolerancia para mandar o no
        if (!shouldSendCSQ(id, ticks))
            return true;    // Considera enviado

        // Mandar comando por socket
        if (!sendCSQ(id, ticks))
            return false;

        // Si la entrada de cache no existe, guardar primero el valor de tolerancia por defecto
        if (!isCached(id)) {
            unsigned int tolerances = 
                static_cast<unsigned int>(std::round((maxTickValue_ - minTickValue_) * (tolerance_percent_ / 100.0f)));
            setTolerance(id, tolerances);
        }

        // Guardar el valor nuevo en la cache
        cacheValue(id, ticks);
        return true;
    }

    bool Symetrix::setButton(unsigned int id, bool selection) {

        // Comprobar si se pueden mandar los datos
        if (!connected_ || id == 0) return false;

        // Obtener el valor en la escala de ticks de Symetrix, desde escala [0,1] (disable,enable)
        const int ticks = ValueToTicks( selection ? 1 : 0 , 0, 1);

        /* No se comprueba tolerancia ni guarda caché, tolerancia a 0 para valores discretos */

        // Mandar comando por socket
        if (!sendCSQ(id, ticks))
            return false;

        // Guardar el valor nuevo en la cache
        cacheValue(id, ticks);
        return true;
    }


    // Supermatrix --------------------------------------------------------------------------

    bool Symetrix::setSupermatrixValue(unsigned int in, unsigned int out, float volume, bool real_scale) {
        if (!connected_) return false;

        // Comprobación de límites de supermatrix
        if (in > supermatrix_ins_) {
            SYS_WARN("Symetrix","setSupermatrixValue: IN value out of limits");
            return false;
        }
        if (out > supermatrix_outs_) {
            SYS_WARN("Symetrix","setSupermatrixValue: OUT value out of limits");
            return false;
        }

        // Obtener el valor en dB
        float dbValue = 0.0f;

        // Cálculo de valor en db si está dentro de los límites (y aplica)
        float minVal = real_scale ? SYM_GAIN_MIN : 0;
        float maxVal = real_scale ? SYM_GAIN_MAX : 100;
        if (volume < maxVal && volume > minVal)
            dbValue = real_scale ? volume : pct_to_dB(volume);
        else    // Truncamiento de valor real si sobrepasa los límites (se queda con el extremo)
            dbValue = std::clamp(volume, static_cast<float>(SYM_GAIN_MIN), static_cast<float>(SYM_GAIN_MAX));

        // Obtiene un valor de id para la caché a partir de la entrada-salida
        unsigned int id = (out << 16) | (in & 0xFFFF);

        // No se manda el valor si no supera la tolerancia
        if (!shouldSendCMV(id, dbValue)) 
            return true;    // Considera enviado

        // Mandar comando por socket
        if(!sendCMV(in, out, dbValue))
            return false;

        // Guardar el valor en la cache
        cacheValue(id, dbValue);
        return true;
    }


    // Tolerancias --------------------------------------------------------------------------

    void Symetrix::updateTolerancePct(unsigned int newTolerancePercent, unsigned int id) {

        // Comprobar rango
        if (newTolerancePercent > 100) {
            SYS_WARN("Symetrix", "New tolerance percent must be in the interval [0-100]");
            return;
        }

        // Calcular nuevos ticks de tolerancia
        unsigned int tickTol = static_cast<unsigned int>(std::round((maxTickValue_ - minTickValue_) * (newTolerancePercent / 100.0f)));

        // Cuando se selecciona el cambio de tolerancia para un id concreto
        if (id != 0) {
            // Si el valor no está cacheado, avisar de que va a cambiar la tolerancia en un valor no cacheado
            auto it = cache_.find(id);
            if (it == cache_.end()) {
                SYS_WARN("Symetrix","Updating tolerance to not cached value");
                cache_[id].tolerance = tickTol;
            }
            cache_[id].tolerance = tickTol;
        }
        else {      // Aplica a todos los componentes antiguos y nuevos (TAMBIÉN A VALORES CACHEADOS EN DB)
            tolerance_percent_ = newTolerancePercent;
            // Establecer el nuevo valor de tolerancia en todos los elementos de la cache
            for (auto & it : cache_)
                it.second.tolerance = tickTol;
        }

    }


    // Inicialización privada ---------------------------------------------------------------

    bool Symetrix::initConnection(std::wstring const& hostIp) {

        // Si ya está conectado no hacer nada
        if (connected_)
            return true;

        // Iniciar WSA (Contexto de red Windows)
        if (!wsaStarted_) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                SYS_WARN("Symetrix", "init: WSAStartup failed.");
                return false;
            }
            wsaStarted_ = true;
        }

        // Crear socket de envío UDP
        pimpl_->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (pimpl_->socket == INVALID_SOCKET) {
            SYS_WARN("Symetrix", "init: Socket binding failed.");
            net_cleanup();
            return false;
        }

        // Establecer timeout de recepción de ping al socket
        if (setsockopt(pimpl_->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&connection_ping_timeout_ms_, sizeof(connection_ping_timeout_ms_)) == SOCKET_ERROR) {
            SYS_WARN("Symetrix", "Failed to set timeout socket option.");
            net_cleanup();
            return false;
        }

        // Prepara la dirección de destino en formato big-endian
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(ComposerPort_);

        // Probar conexión desde IP literal (si hostIP es una IP)
        if (InetPtonW(AF_INET, hostIp.c_str(), &dest.sin_addr) != 1) {
            SYS_WARN("Symetrix","Cannot bind with hostIP: " + std::string(hostIp.begin(),hostIp.end()) );
            SYS_INFO("Symetrix","Trying to connect by IP alias...");
            // Intenta conectar si hostIP es un alias (como por ejemplo DNS)
            ADDRINFOW hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            ADDRINFOW* res = nullptr;
            if (GetAddrInfoW(hostIp.c_str(), nullptr, &hints, &res) != 0 || !res) {
                SYS_WARN("Symetrix","Cannot connect to Symetrix: hostIP not recognized.");
                net_cleanup();
                return false;
            }
            dest = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
            dest.sin_port = htons(ComposerPort_);
            SYS_SOLVED("Symetrix","hostIP alias found and bind correctly.");
            FreeAddrInfoW(res);
        }

        // Asociación "estática" del socket UDP: Ahora "send" y "recv" no necesitan especificar destino
        if (connect(pimpl_->socket, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest)) == SOCKET_ERROR) {
            net_cleanup();
            return false;
        }

        // --- PING CON MANEJO DE ERRORES MEJORADO ---

        // Probar a mandar un ping
        char pingCmd[] = "CSQ 1\r";
        if (send(pimpl_->socket, pingCmd, (int)strlen(pingCmd), 0) == SOCKET_ERROR) {
            SYS_WARN("Symetrix", "Cannot send ping to symetrix: Socket fail.");
            return false;
        }

        // Esperamos respuesta (el socket tiene 500ms de timeout configurado arriba)
        char rxBuf[64];
        if (recv(pimpl_->socket, rxBuf, sizeof(rxBuf), 0) <= 0) {
            SYS_WARN("Symetrix","Cannot receive ACK from Symetrix. Check connection or IP");
            return false;
        }

        connected_ = true;
        return true;
    }

    void Symetrix::net_cleanup() {

        if (pimpl_->socket != INVALID_SOCKET) {
            closesocket(pimpl_->socket);
            pimpl_->socket = INVALID_SOCKET;
        }
        if (wsaStarted_) {
            WSACleanup();
            wsaStarted_ = false;
        }
        initialized_ = false;
    }


    // Conversión de datos ------------------------------------------------------------------

    unsigned int Symetrix::ValueToTicks(float value, float minValue, float maxValue) {
        
        // Comprobación de errores
        if (minValue >= maxValue) {
            SYS_WARN("Symetrix","ValueToTicks: Min value greater or equal than max value.");
            return 0;   // Cuidado con esto que no es el valor esperado
        }
        if (maxValue <= minValue) {
            SYS_WARN("Symetrix","ValueToTicks: Max value lower or equal than min value.");
            return 0;   // Cuidado con esto que no es el valor esperado
        }
        
        // Asignación directa sin calcular
        if (value <= minValue) return minTickValue_;
        if (value >= maxValue) return maxTickValue_;

        // Devolver el valor convertido escalado a la escala de destino (ticks)
        float fraction = (value - minValue) / (maxValue - minValue);
        return minTickValue_ + static_cast<unsigned int>(std::round(fraction * (maxTickValue_ - minTickValue_)));
    }

    float Symetrix::pct_to_dB(float pct) {        
        // Convertir el porcentaje a un ratio (0.0 a 1.0)
        float ratio = std::clamp(pct, 0.0f, 100.0f) / 100.0f;
        
        // Aplicar la curva de ponderación: db = Min + ((pct/100)^gamma * (Max - Min))
        float curve = std::pow(ratio, dBcurve_gamma_);
        
        // Calcular el valor en dB dentro del rango del DSP
        return SYM_GAIN_MIN + (curve * static_cast<float>(SYM_GAIN_MAX - SYM_GAIN_MIN));
    }

    float Symetrix::dB_to_pct(float dbValue) {
        // Asegurar que el dB esté dentro de los límites del DSP
        float clampedDb = std::clamp(dbValue, static_cast<float>(SYM_GAIN_MIN), static_cast<float>(SYM_GAIN_MAX));
        
        // Normalizar: obtener el valor de la curva (de 0.0 a 1.0)
        float curve = (clampedDb - SYM_GAIN_MIN) / static_cast<float>(SYM_GAIN_MAX - SYM_GAIN_MIN);
        
        // Invertir la curva: ratio = curve ^ (1 / gamma)
        return std::pow(curve, 1.0f / dBcurve_gamma_) * 100;
    }


    // Caché de datos de envío --------------------------------------------------------------

    bool Symetrix::isCached(unsigned int id) const {
        return cache_.find(id) != cache_.end();
    }

    void Symetrix::cacheValue(unsigned int id, float value) {
        cache_[id].cachedValue = value;
    }

    float Symetrix::getCachedValue(unsigned int id) const {
        auto it = cache_.find(id);
        return it != cache_.end() ? it->second.cachedValue : 0.0f;
    }
    
    int Symetrix::getCachedTolerance(unsigned int id) const {
        auto it = cache_.find(id);
        return it != cache_.end() ? it->second.tolerance : 0;
    }


    // Tolerancias (privado) ----------------------------------------------------------------

    void Symetrix::setTolerance(unsigned int id, unsigned int newTolerance) {
        cache_[id].tolerance = newTolerance;
    }


    // Envío de datos -----------------------------------------------------------------------

    bool Symetrix::shouldSendCSQ(unsigned int id, unsigned int newTicks) {

        // Si el valor no está cacheado, se debería enviar
        if (!isCached(id))
            return true;

        // Comprobar si el cambio de valor supera la tolerancia
        return std::abs(static_cast<int>(newTicks) - static_cast<int>(getCachedValue(id)) ) >= getCachedTolerance(id);
    }

    bool Symetrix::sendCSQ(unsigned int id, unsigned int ticks) {

        // Calcular tamaño de buffer redondeado a la siguiente potencia de 2 a partir del comando
        const std::size_t cmdSize = std::formatted_size("CSQ {} {}\r", id, ticks);
        const std::size_t bufsize = std::bit_ceil(cmdSize);

        // Buffer para construir el comando
        std::vector<char> buf(bufsize);

        // Comando formateado con el valor de los ticks + id (e.g., "CSQ 302 1124")
        auto [iter, written] = std::format_to_n(buf.data(), cmdSize, "CSQ {} {}\r", id, ticks);

        // Envío
        const int len = static_cast<int>(written);
        int sent = send(pimpl_->socket, buf.data(), len, 0);
        if (sent == SOCKET_ERROR || sent != len) {
            SYS_WARN("Symetrix", "sendCSQ Error");
            return false;
        }

        return true;
    }

    bool Symetrix::shouldSendCMV(unsigned int id, float dbValue) {

        // Si el valor no está cacheado, se debería enviar
        if (!isCached(id))
            return true;

        // Calcular la tolerancia en dB
        float valuedB_menos_tolerancia = pct_to_dB(std::max(0.0f, dB_to_pct(dbValue) - tolerance_percent_));
        float toleranceDb = std::abs(dbValue - valuedB_menos_tolerancia);

        // Comprobar si el valor está dentro del rango
        return std::abs(dbValue - getCachedValue(id)) > toleranceDb;
    }

    bool Symetrix::sendCMV(unsigned int in, unsigned int out, float dbValue) {
        
        // Calcular tamaño de buffer redondeado a la siguiente potencia de 2 a partir del comando
        constexpr std::string_view componentName = "0.1.CPGain";
        const std::size_t cmdSize = std::formatted_size("$q CMV Set {}.{{I{}O{}}} {:.2f}\r",
            componentName, in, out, dbValue);
        const std::size_t bufsize = std::bit_ceil(cmdSize);
        
        // Buffer para construir el comando
        std::vector<char> buf(bufsize);
        
        // Construcción del comando. Formato: $q CMV Set [Nombre].{I<in>O<out>} <value>
        auto [iter, written] = std::format_to_n(buf.data(), cmdSize, "$q CMV Set {}.{{I{}O{}}} {:.2f}\r",
            componentName, in, out, dbValue);

        // Envío
        const int len = static_cast<int>(written);
        const int sent = send(pimpl_->socket, buf.data(), len, 0);
        if (sent == SOCKET_ERROR || sent != len) {
            SYS_WARN("Symetrix", "sendCMV: Failed sending command by socket.");
            return false;
        }

        return true;
    }


#else
// ============================================================
//  (Stubs para SO No-Windows)
// ============================================================

// Definición del struct de pimpl vacío
struct Symetrix::Impl {};

Symetrix::Symetrix() :
    pimpl_(std::make_unique<Impl>()),
    wsaStarted_(false),
    connected_(false),
    initialized_(false),
    connection_ping_timeout_ms_(0),
    ComposerPort_(48631),
    tolerance_percent_(2),
    supermatrix_outs_(20),
    supermatrix_ins_(20),
    kBootPreset_(0)
{
    SYS_WARN("Symetrix", "Symetrix not compatible in non-Windows OS. Network bypassed.");
}

Symetrix::~Symetrix() {}

// Ejecución ----------------------------------------------------------------------------
bool Symetrix::Init(const std::wstring&, bool) { return false; }
void Symetrix::Destroy() {}
bool Symetrix::isConnected() const { return false; }

// Parámetros ---------------------------------------------------------------------------
void Symetrix::setSupermatrixINs(unsigned int) {}
void Symetrix::setSupermatrixOUTs(unsigned int) {}

// Comandos -----------------------------------------------------------------------------
bool Symetrix::LoadPreset(unsigned int) { return false; }

// Componentes: Controles generales -----------------------------------------------------
bool Symetrix::setValue(unsigned int, float, float, float) { return false; }
bool Symetrix::setButton(unsigned int, bool) { return false; }

// Tolerancias --------------------------------------------------------------------------
void Symetrix::updateTolerancePct(unsigned int, unsigned int) {}

// Inicialización privada ---------------------------------------------------------------
bool Symetrix::initConnection(std::wstring const&) { return false; }
void Symetrix::net_cleanup() {}

// Conversión de datos a ticks ----------------------------------------------------------
unsigned int Symetrix::ValueToTicks(float, float, float) { return 0; }

// Caché de datos de envío --------------------------------------------------------------
bool Symetrix::isCached(unsigned int) const             { return false; }
void Symetrix::cacheValue(unsigned int, float)          { return; }
float Symetrix::getCachedValue(unsigned int) const      { return 0; }
int Symetrix::getCachedTolerance(unsigned int) const    { return 0; }

// Tolerancias (privado) ----------------------------------------------------------------
void Symetrix::setTolerance(unsigned int, unsigned int) {}

// Envío de datos -----------------------------------------------------------------------
bool Symetrix::shouldSendCSQ(unsigned int, unsigned int)    { return false; }
bool Symetrix::sendCSQ(unsigned int, unsigned int)          { return false; }
bool Symetrix::shouldSendCMV(unsigned int, float)           { return false; }
bool Symetrix::sendCMV(unsigned int, unsigned int, float)   { return false; }

#endif
