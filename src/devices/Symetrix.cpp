#include "devices/Symetrix.hpp"
#include "system/SystemMgr.hpp"

// Symetrix Composer solo se puede descargar para Windows/Mac. Se usan las funciones de socket de Windows directamente.
#ifdef WIN32

    // Definiciones por si no están definidos SYS_INFO ni SYS_WARN de SystemMgr
    #ifndef SYS_INFO
        #define SYS_INFO(module,msg) std::cout << module << msg << std::endl;
    #endif
    #ifndef SYS_WARN
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
        m_lastPingTime_(std::chrono::steady_clock::now()),
        m_waitingPingResponse_(false),
        wsaStarted_(false),
        connected_(false),
        initialized_(false),
        connection_ping_timeout_ms_(500),
        ComposerPort_(48631),
        tolerance_percent_(2),            // Por defecto tolerancias de un 2% sobre el rango
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

        // Inicialización de variables de supermatrix
        initSupermatrix();

        // Cargar preset de boot
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

        if (value < minValue) {
            SYS_WARN("Symetrix","Value lower than min value range");
            return false;
        }
        if (value > maxValue) {
            SYS_WARN("Symetrix","Value higher than max value range");
            return false;
        }

        // Obtener el valor en la escala de ticks de Symetrix
        const int ticks = ValueToTicks(value, minValue, maxValue);

        // Comprobar variación respecto a tolerancia para mandar o no
        if (shouldSend(id, ticks) && sendCSQ(id, ticks)) {

            // Si la entrada de cache no existe, guardar primero el valor de tolerancia por defecto
            if (!isCached(id)) {
                unsigned int toleranceTicks = 
                    static_cast<unsigned int>(std::round((maxTickValue_ - minTickValue_) * (tolerance_percent_ / 100.0f)));
                setToleranceTicks(id, toleranceTicks);
            }

            // Guardar el valor nuevo en la cache
            cacheValue(id, ticks);
            return true;
        }

        /* else */      // No se ha mandado el valor
        return false;
    }

    bool Symetrix::setButton(unsigned int id, bool selection) {

        // Comprobar si se pueden mandar los datos
        if (!connected_ || id == 0) return false;

        // Obtener el valor en la escala de ticks de Symetrix, desde escala [0,1] (disable,enable)
        const int ticks = ValueToTicks( selection ? 1 : 0 , 0, 1);

        // Comprobar variación respecto a tolerancia para mandar o no
        if (sendCSQ(id, ticks)) {

            // Si la entrada de cache no existe, guardar primero el valor de tolerancia por defecto
            if (!isCached(id))
                setToleranceTicks(id, 0);   // Tolerancia a 0 en valores binarios

            // Guardar el valor nuevo en la cache
            cacheValue(id, ticks);
            return true;
        }

        /* else */      // No se ha mandado el valor
        return false;
    }


    // Tolerancias --------------------------------------------------------------------------

    void Symetrix::updateTolerance(unsigned int newTolerancePercent, unsigned int id) {

        // Comprobar rango
        if (newTolerancePercent > 100 || newTolerancePercent < 0) {
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
                cache_[id].toleranceTick = tickTol;
            }
            cache_[id].toleranceTick = tickTol;
        }
        else {      // Aplica a todos los componentes antiguos y nuevos
            tolerance_percent_ = newTolerancePercent;
            // Establecer el nuevo valor de tolerancia en todos los elementos de la cache
            for (auto & it : cache_)
                it.second.toleranceTick = tickTol;
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

    void Symetrix::initSupermatrix() {
        const size_t total = size_t(supermatrix_outs_ + 1) * size_t(supermatrix_ins_ + 1);

        constexpr double gamma = 0.415;
        constexpr int kOffCenti = -7200;
        constexpr int kMinCenti = -7200;
        constexpr int kMaxCenti = +1200;

        stepToCentidB_[0] = kOffCenti;
        for (int s = 1; s <= 100; ++s) {
            const double x = static_cast<double>(s) / 100.0;
            const double dB = -60.0 + 72.0 * std::pow(x, gamma);
            long v = static_cast<long>(std::llround(dB * 100.0));
            stepToCentidB_[s] = std::clamp(static_cast<int>(v), kMinCenti, kMaxCenti);
        }
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


    // Caché de datos de envío --------------------------------------------------------------

    unsigned int Symetrix::ValueToTicks(float value, float minValue, float maxValue) {
        
        // Comprobación de errores
        if (minValue >= maxValue) {
            SYS_WARN("Symetrix","ValueToTicks: Min value greater or equal than max value.");
            return 0;   // Cuidado con esto que no es el valor esperado
        }
        if (minValue >= maxValue) {
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

    bool Symetrix::isCached(unsigned int id) const {
        return cache_.find(id) != cache_.end();
    }

    void Symetrix::cacheValue(unsigned int id, unsigned int currentTick) {
        cache_[id].cachedTicks = currentTick;
    }

    
    // Tolerancias (privado) ----------------------------------------------------------------

    void Symetrix::setToleranceTicks(unsigned int id, unsigned int newToleranceTicks) {
        cache_[id].toleranceTick = newToleranceTicks;
    }


    // Envío de datos -----------------------------------------------------------------------

    bool Symetrix::shouldSend(unsigned int id, unsigned int newTicks) {

        auto it = cache_.find(id);

        // Si el valor no está cacheado, se debería enviar
        if (it == cache_.end())
        return true;

        // Comprobar si el cambio de valor supera la tolerancia
        return std::abs(static_cast<int>(newTicks) - it->second.cachedTicks) >= it->second.toleranceTick;
    }

    bool Symetrix::sendCSQ(unsigned int id, unsigned int ticks) {
        char buf[32];
        char* p = buf;
        char* const end = buf + sizeof(buf);

        // Añadir prefijo "CSQ "
        std::memcpy(p, "CSQ ", 4);
        p += 4;

        // Añadir ID
        auto res_id = std::to_chars(p, end, id);
        if (res_id.ec != std::errc{}) return false;
        p = res_id.ptr;

        // Añadir espacio
        if (p >= end) return false;
        *p++ = ' ';

        // Añadir Ticks
        auto res_ticks = std::to_chars(p, end, ticks);
        if (res_ticks.ec != std::errc{}) return false;
        p = res_ticks.ptr;

        // Añadir retorno de carro
        if (p >= end) return false;
        *p++ = '\r';

        // Envío
        const int len = static_cast<int>(p - buf);
        int sent = send(pimpl_->socket, buf, len, 0);
        if (sent == SOCKET_ERROR || sent != len) {
            SYS_WARN("Symetrix", "sendCSQ Error");
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
    m_lastPingTime_(),
    m_waitingPingResponse_(false),
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
    SYS_WARN("Symetrix", "Symetrix class not compatible in non-Windows OS. Network bypassed.");
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
void Symetrix::updateTolerance(unsigned int, unsigned int) {}

// Inicialización privada ---------------------------------------------------------------
bool Symetrix::initConnection(std::wstring const&) { return false; }
void Symetrix::initSupermatrix() {}
void Symetrix::net_cleanup() {}

// Conversión de datos a ticks ----------------------------------------------------------
unsigned int Symetrix::ValueToTicks(float, float, float) { return 0; }

// Caché de datos de envío --------------------------------------------------------------
bool Symetrix::isCached(unsigned int) const { return false; }
void Symetrix::cacheValue(unsigned int, unsigned int) {}

// Tolerancias (privado) ----------------------------------------------------------------
void Symetrix::setToleranceTicks(unsigned int, unsigned int) {}

// Envío de datos -----------------------------------------------------------------------
bool Symetrix::shouldSend(unsigned int, unsigned int) { return false; }
bool Symetrix::sendCSQ(unsigned int, unsigned int) { return false; }

#endif