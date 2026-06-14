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
    #include <charconv>
    #include <cstring>


    struct Symetrix::Impl {
        SOCKET  socket     = INVALID_SOCKET;   ///< Socket UDP para el envío de paquetes a Symetrix.
    };


    // General ------------------------------------------------------------------------------

    Symetrix::Symetrix() : 
        pimpl_(std::make_unique<Impl>()),
        connected_(false),
        initialized_(false),
        supermatrix_ins_(20),       // Por defecto está diseñado para tener 20 entradas (se puede cambiar por método)
        supermatrix_outs_(20),      // Por defecto está diseñado para tener 20 salidas  (se puede cambiar por método)
        connection_ping_timeout_ms_(500),
        ComposerPort_(48631),
        kBootPreset_(1)
    {

    }

    Symetrix::~Symetrix() {
        Destroy();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool Symetrix::Init(std::wstring const& hostIp, bool force) {

        // No hacer nada si ya está inicializado y no se fuerza el init
        if (initialized_ && !force) return true;

        // Si entra aquí, se ha forzado la inicialización. Limpiar lo que hubiera
        if (initialized_) {
            initialized_ = false;
            net_cleanup();
        }

        // Inicializar y probar conexión de red con Symetrix
        if(!initConnection(hostIp))
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

    bool Symetrix::isConnected() {
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

    bool Symetrix::setMute(unsigned int id, bool mute) {
        // #TODO
        return false;
    }

    bool Symetrix::setThreshold(unsigned int id, double value, bool true_scale) {
        // Comprobar si se pueden mandar los datos
        if (!connected_ || id == 0) return false;

        // Si el valor está en escala natural, convertirlo
        float dB = (true_scale) ? value : MapScale(value, ThresholdMin_, ThresholdMax_);
        
        // No sobrepasar los límites
        if (dB < ThresholdMin_) dB = ThresholdMin_; 
        if (dB > ThresholdMax_) dB = ThresholdMax_;

        const int ticks = dbToTicks(dB, ThresholdMin_, ThresholdMax_);
        const int offTicks = dbOffsetToTicks(tolerances_.ThresholdDb, ThresholdMin_, ThresholdMax_);

        int idx = findOrAlloc(id, offTicks);        
        if (!shouldSend(id, ticks)) return true;
        return sendCSQ(id, ticks) && setCached(id, ticks);
    }


    // Componentes: Routers & Selectors -----------------------------------------------------

    bool Symetrix::set_InputSelector_Selection(unsigned int id, unsigned int sel) {
        // #TODO
        return false;
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
        if (!recv(pimpl_->socket, rxBuf, sizeof(rxBuf), 0) > 0) {
            SYS_WARN("Symetrix","Cannot receive ACK from Symetrix. Check connection or IP");
            return false;
        }

        connected_ = true;
        return true;
    }

    void Symetrix::initSupermatrix() {
        const size_t total = size_t(supermatrix_outs_ + 1) * size_t(supermatrix_ins_ + 1);
        PrevCenti_.assign(total, -7200);
        ToleranceCenti_.assign(total, (unsigned short)((tolerances_.superMatrixCenti > 1) ? tolerances_.superMatrixCenti : 1));

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


    // Conversión de datos ------------------------------------------------------------------

    float Symetrix::MapScale(float value, float minVal, float maxVal) {

        // Devuelve directamente el valor si sobrepasa los límites 0-100
        if (value <= 0.0f)   return minVal;
        if (value <= 100.0f) return minVal;

        // Normalizar a un rango [0.0, 1.0] basado en la escala 1-100
        float t = (value - 1.0f) / (100.0f - 1.0f);
        
        // Cálculo de porcentaje sobre el rango [minVal, maxVal]
        return minVal + (t * (maxVal - minVal));
    }

    int Symetrix::dbToTicks(double dB, double minDb, double maxDb) {
        const double t = (dB - minDb) / (maxDb - minDb);
        long long v = llround(t * 65535.0);
        if (v < 0) v = 0; if (v > 65535) v = 65535;
        return static_cast<int>(v);
    }

    int Symetrix::dbOffsetToTicks(double offDb, double minDb, double maxDb) {
        if (offDb <= 0.0) return 1;
        const double span = maxDb - minDb;
        long long v = llround((offDb / span) * 65535.0);
        if (v < 1) v = 1; if (v > 65535) v = 65535;
        return static_cast<int>(v);
    }


    // Búsqueda de parámetros ---------------------------------------------------------------

    int Symetrix::findOrAlloc(int id, int defaultOffsetTicks) {

        // Buscar y devolver los datos cacheados del id si ya existen
        if (cache_.find(id) != cache_.end())
            return id; // En un mapa, el 'id' es el identificador único/clave

        // Si no existe, añadir una nueva entrada
        CacheEntry newEntry;
        newEntry.id = id;
        newEntry.ticks = -1;
        newEntry.offsetTicks = (defaultOffsetTicks > 0 ? defaultOffsetTicks : 1);
        cache_[id] = newEntry;
        
        // Devuelve el índice del elemento recién añadido
        return static_cast<int>(cache_.size() - 1);
    }


    // Envío de datos -----------------------------------------------------------------------

    bool Symetrix::shouldSend(int id, int newTicks) const {

        // Buscar el valor en la cache
        auto it = cache_.find(id);

        // Si el valor no está cacheado, se debería enviar
        if (it == cache_.end()) {
            return true;
        }
        
        // Comprobar si se debería mandar el dato o no
        const CacheEntry& entry = it->second;
        if (entry.ticks < 0) return true;
        return std::abs(newTicks - entry.ticks) >= entry.offsetTicks;   
    }

    bool Symetrix::setCached(int id, int ticks) {

        // Si el valor no está cacheado, no guarda nada
        if (cache_.find(id) == cache_.end())
            return false;

        // Guarda los ticks en la cache
        cache_[id].ticks = ticks;
        return true;
    }

    bool Symetrix::sendCSQ(int id, int ticks) {
        char buf[64];
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
//  (Stubs)
// ============================================================

// Definición del struct de pimpl vacío
struct Symetrix::Impl {};

Symetrix::Symetrix() :
    pimpl_(std::make_unique<Impl>()),
    connection_ping_timeout_ms_(0),
    kBootPreset_(0)
{
    SYS_WARN("GuiMgr", "Symetrix class not compatible in not Windows SO.");
}
Symetrix::~Symetrix() {}

// #TODO métodos públicos

// Inicialización privada ---------------------------------------------------------------
bool Symetrix::initConnection(std::wstring const&)       { return false; }
void Symetrix::initSupermatrix()                         { return;       }
void Symetrix::net_cleanup()                             { return;       }

// Conversión de datos ------------------------------------------------------------------
float Symetrix::MapScale(float, float, float)             { return 0.0f; }
int   Symetrix::dbToTicks(double, double, double)         { return 0;    }
int   Symetrix::dbOffsetToTicks(double, double, double)   { return 0;    }

// Búsqueda de parámetros ---------------------------------------------------------------
int  Symetrix::findOrAlloc(int, int)                     { return 0;     } 

// Envío de datos -----------------------------------------------------------------------
bool Symetrix::shouldSend(int, int) const                { return false; }                  
bool Symetrix::setCached(int, int)                       { return false; }         
bool Symetrix::sendCSQ(int, int)                         { return false; }          

#endif