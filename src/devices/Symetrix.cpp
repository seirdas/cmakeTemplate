#include "devices/Symetrix.hpp"
#include "system/SystemMgr.hpp"
#include "files/JsonMgr.hpp"


// General ------------------------------------------------------------------------------

Symetrix::Symetrix() :
    pimpl_(std::make_unique<Impl>()),
    initialized_(false),
    running_(false),
    wsaStarted_(false),
    connected_(false),
    m_waitingPingResponse_(false),
    connection_ping_timeout_ms_(500),
    ComposerPort_(48631),
    SymetrixIP_("192.168.7.21"),
    connection_check_seconds_(5),
    dBcurve_gamma_(0.415f),
    minTickValue_(0),
    maxTickValue_(65535),
    tolerance_percent_(2),            // Por defecto tolerancias de un 2% sobre el rango
    supermatrix_ins_(20),             // Por defecto para tener 20 entradas (se puede cambiar por método)
    supermatrix_outs_(20),            // Por defecto para tener 20 salidas (se puede cambiar por método)
    kBootPreset_(1)
{

}

void Symetrix::loadConfig(void* config) {
    if (!config)
        return;

    // Se considera que la configuración se pasa como json
    json* cfg = static_cast<json*>(config);
    JsonMgr& jsonMgr = JsonMgr::instance();

    jsonMgr.get_or_set(cfg, "SymetrixIP",                   SymetrixIP_);
    jsonMgr.get_or_set(cfg, "connection_ping_timeout_ms",   connection_ping_timeout_ms_);
    jsonMgr.get_or_set(cfg, "ComposerPort",                 ComposerPort_);
    jsonMgr.get_or_set(cfg, "tolerance_percent",            tolerance_percent_);            // Por defecto tolerancias de un 2% sobre el rango
    jsonMgr.get_or_set(cfg, "dBcurve_gamma",                dBcurve_gamma_);
    jsonMgr.get_or_set(cfg, "supermatrix_ins",              supermatrix_ins_);             // Por defecto para tener 20 entradas (se puede cambiar por método)
    jsonMgr.get_or_set(cfg, "supermatrix_outs",             supermatrix_outs_);            // Por defecto para tener 20 salidas (se puede cambiar por método)
    jsonMgr.get_or_set(cfg, "kBootPreset",                  kBootPreset_);
    jsonMgr.get_or_set(cfg, "connection_check_seconds",     connection_check_seconds_);

}


// Symetrix Composer solo se puede descargar para Windows/Mac. Se usan las funciones de socket de Windows directamente.
#ifdef WIN32

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

    /* Movido fuera el encapsulado WIN32, común en ambos sistemas */
    // Symetrix::Symetrix() {...}

    Symetrix::~Symetrix() {
        close();
    }


    // Ejecución ----------------------------------------------------------------------------

    bool Symetrix::init(void* config) {

        // No hacer nada si ya está inicializado y no se fuerza el init
        if (initialized_) return true;

        // Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config)
            loadConfig(config);
        else
            SYS_WARN("Symetrix","Cannot load config. Using default values.");

        // Inicializar y probar conexión de red con Symetrix
        if(!initConnection())
            return false;

        // Cargar preset de boot por defecto
        if (kBootPreset_ > 0)
            LoadPreset(kBootPreset_);

        initialized_ = true;
        return true;
    }

    bool Symetrix::isInitialized() const {
        return initialized_;
    }
    
    /* Movido fuera el encapsulado WIN32, común en ambos sistemas */
    // Symetrix::loadConfig(void* config) {...}

    void Symetrix::close() {

        // Si no está inicializado no hay que destruir nada
        if (!initialized_) return;
        running_ = false;

        // Cierra Socket y WSA
        net_cleanup();

        // Esperar al hilo que comprueba la conexión
        connection_cv_.notify_one();
        if (connection_checker_.joinable()) {
            SYS_INFO("Symetrix","Waiting for connection checker thread...");
            connection_checker_.join();
        }

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


    // Control de componentes Symetrix Composer ---------------------------------------------

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

    bool Symetrix::setValue(unsigned short id, float value, float minValue, float maxValue) {

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
        if (!isCached_CSQ(id)) {
            unsigned int tolerance = 
                static_cast<unsigned int>(std::round((maxTickValue_ - minTickValue_) * (tolerance_percent_ / 100.0f)));
            setTolerance_CSQ(id, tolerance);
        }

        // Guardar el valor nuevo en la cache
        cacheValue_CSQ(id, ticks);
        return true;
    }

    bool Symetrix::setValue_dB(unsigned short id, float value, float minValue, float maxValue) {

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
        if (!isCached_CSQ(id)) {
            unsigned int tolerance = 
                static_cast<unsigned int>(std::round((maxTickValue_ - minTickValue_) * (tolerance_percent_ / 100.0f)));
            setTolerance_CSQ(id, tolerance);
        }

        // Guardar el valor nuevo en la cache
        cacheValue_CSQ(id, ticks);
        return true;
    }

    bool Symetrix::setButton(unsigned short id, bool selection) {

        // Comprobar si se pueden mandar los datos
        if (!connected_ || id == 0) return false;

        // Obtener el valor en la escala de ticks de Symetrix, desde escala [0,1] (disable,enable)
        const int ticks = ValueToTicks( selection ? 1 : 0 , 0, 1);

        /* No se comprueba tolerancia ni guarda caché, tolerancia a 0 para valores discretos */

        // Mandar comando por socket
        if (!sendCSQ(id, ticks))
            return false;

        // Guardar el valor nuevo en la cache
        cacheValue_CSQ(id, ticks);
        return true;
    }

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
        if (in == 0) {
            SYS_WARN("Symetrix","setSupermatrixValue: 0 is not a valid 'IN' value.");
            return false;
        }
        if (out == 0) {
            SYS_WARN("Symetrix","setSupermatrixValue: 0 is not a valid 'OUT' value.");
            return false;
        }

        // Cálculo de valor en db
        float dbValue = 0.0f;
        float minVal = real_scale ? SYM_GAIN_MIN : 0;
        float maxVal = real_scale ? SYM_GAIN_MAX : 100;
        if (volume >= maxVal)
            dbValue = SYM_GAIN_MAX;
        else if (volume <= minVal)
            dbValue = SYM_GAIN_MIN;
        else
            dbValue = real_scale ? volume : pct_to_dB(volume);

        // Obtiene un valor de id para la caché a partir de la entrada-salida
        unsigned short id = (out << 16) | (in & 0xFFFF);

        // No se manda el valor si no supera la tolerancia
        if (!shouldSendCMV(id, dbValue)) 
            return true;    // Considera enviado

        // Mandar comando por socket
        if(!sendCMV(in, out, dbValue))
            return false;

        // Si la entrada de cache no existe, guardar primero el valor de tolerancia por defecto
        if (!isCached_CMV(id)) 
            setTolerance_CMV(id, tolerance_percent_);

        // Guardar el valor en la cache
        cacheValue_CMV(id, dbValue);
        return true;
    }


    // Tolerancias --------------------------------------------------------------------------

    void Symetrix::updateTolerancePct_CSQ(unsigned char newTolerancePercent, unsigned short id) {

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
            auto it = CSQ_cache_.find(id);
            if (it == CSQ_cache_.end()) {
                SYS_WARN("Symetrix","Updating tolerance to not cached value");
                CSQ_cache_[id].tolerance = tickTol;
            }
            CSQ_cache_[id].tolerance = tickTol;
        }
        else {      // Aplica a todos los componentes antiguos y nuevos
            tolerance_percent_ = newTolerancePercent;
            // Establecer el nuevo valor de tolerancia en todos los elementos de la cache
            for (auto & it : CSQ_cache_)
                it.second.tolerance = tickTol;
        }

    }

    void Symetrix::updateTolerancePct_Supermatrix(unsigned short newTolerancePercent) {

        // Comprobar rango
        if (newTolerancePercent > 100) {
            SYS_WARN("Symetrix", "New tolerance percent must be in the interval [0-100]");
            return;
        }

        // Establecer el nuevo valor de tolerancia en todos los elementos de la cache y posteriores
        tolerance_percent_ = newTolerancePercent;
        for (auto & it : CMV_cache_)
            it.second.tolerance = newTolerancePercent;

    }


    // Inicialización privada ---------------------------------------------------------------

    bool Symetrix::initConnection() {

        // Si ya está conectado no hacer nada
        if (connected_)
            return true;

        SYS_INFO("Symetrix","Setting up async connection to Symetrix...");

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
        if (inet_pton(AF_INET, SymetrixIP_.c_str(), &dest.sin_addr) != 1) {
            SYS_WARN("Symetrix", "Cannot bind as literal IP. Trying to resolve as hostname/alias: " + SymetrixIP_);

            // Configurar las pistas (hints) para la resolución de DNS
            addrinfo hints{};
            hints.ai_family = AF_INET;       // IPv4
            hints.ai_socktype = SOCK_DGRAM;  // UDP
            hints.ai_protocol = IPPROTO_UDP;

            addrinfo* res = nullptr;

            // Utilizar getaddrinfo para resolver el hostname/alias
            if (getaddrinfo(SymetrixIP_.c_str(), nullptr, &hints, &res) != 0 || !res) {
                SYS_WARN("Symetrix", "Cannot connect to Symetrix: hostIP alias not recognized.");
                net_cleanup();
                return false;
            }

            // Copiar la dirección encontrada a nuestra estructura 'dest'
            dest = *reinterpret_cast<sockaddr_in*>(res->ai_addr);
            dest.sin_port = htons(ComposerPort_); // Asignar el puerto

            SYS_SOLVED("Symetrix", "hostIP alias found and resolved correctly.");
            
            // Liberar la memoria asignada por getaddrinfo
            freeaddrinfo(res);
        } else {
            // Si entró aquí, inet_pton funcionó. Solo falta asignar el puerto a la IP literal.
            dest.sin_family = AF_INET;
            dest.sin_port = htons(ComposerPort_);
        }

        // Asociación "estática" del socket UDP: Ahora "send" y "recv" no necesitan especificar destino
        if (connect(pimpl_->socket, reinterpret_cast<const sockaddr*>(&dest), sizeof(dest)) == SOCKET_ERROR) {
            net_cleanup();
            return false;
        }

        // Activar hilo que comprueba conexión a pings constantemente
        running_ = true;
        connection_checker_ = std::thread(&Symetrix::ConnectionChecker, this);

        return true;
    }

    void Symetrix::ConnectionChecker() {

        bool resultado_conexion;
        bool first_connection = true;

        while (running_) {

            resultado_conexion = true;

            // Probar a mandar un ping
            char pingCmd[] = "CSQ 1\r";
            if (send(pimpl_->socket, pingCmd, (int)strlen(pingCmd), 0) == SOCKET_ERROR) 
                resultado_conexion = false;

            // Esperamos respuesta (el socket tiene 'connection_ping_timeout_ms_' de tiempo de timeout)
            char rxBuf[64];
            if (recv(pimpl_->socket, rxBuf, sizeof(rxBuf), 0) <= 0) 
                resultado_conexion = false;

            // Guarda resultado en connected
            if (first_connection || connected_ != resultado_conexion) {
                if (!resultado_conexion)
                    SYS_WARN("Symetrix","Cannot receive ACK from Symetrix. Check connection or IP");
                else if (first_connection) {
                    SYS_INFO("Symetrix","ACK received from Symetrix. Connected.");
                }
                else 
                    SYS_SOLVED("Symetrix","ACK received from Symetrix. Reconnected.");
                connected_ = resultado_conexion;
                first_connection = false;
            }

            // Espera 'connection_check_seconds_' o hasta que 'running_' sea false (notificado)
            std::unique_lock<std::mutex> lock(connection_mutex_);
            connection_cv_.wait_for(lock, std::chrono::seconds(connection_check_seconds_), [this]() {
                return !running_;
            });
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

    float Symetrix::pct_to_dB(float pct) const {        
        // Convertir el porcentaje a un ratio (0.0 a 1.0)
        float ratio = std::clamp(pct, 0.0f, 100.0f) / 100.0f;
        
        // Aplicar la curva de ponderación: db = Min + ((pct/100)^gamma * (Max - Min))
        float curve = std::pow(ratio, dBcurve_gamma_);
        
        // Calcular el valor en dB dentro del rango del DSP
        return SYM_GAIN_MIN + (curve * static_cast<float>(SYM_GAIN_MAX - SYM_GAIN_MIN));
    }

    float Symetrix::dB_to_pct(float dbValue) const {
        // Asegurar que el dB esté dentro de los límites del DSP
        float clampedDb = std::clamp(dbValue, static_cast<float>(SYM_GAIN_MIN), static_cast<float>(SYM_GAIN_MAX));
        
        // Normalizar: obtener el valor de la curva (de 0.0 a 1.0)
        float curve = (clampedDb - SYM_GAIN_MIN) / static_cast<float>(SYM_GAIN_MAX - SYM_GAIN_MIN);
        
        // Invertir la curva: ratio = curve ^ (1 / gamma)
        return std::pow(curve, 1.0f / dBcurve_gamma_) * 100;
    }


    // Caché de datos de envío (CSQ) --------------------------------------------------------

    bool Symetrix::isCached_CSQ(unsigned short id) const {
        return CSQ_cache_.find(id) != CSQ_cache_.end();
    }

    void Symetrix::cacheValue_CSQ(unsigned short id, float value) {
        CSQ_cache_[id].cachedValue = value;
    }

    float Symetrix::getCachedValue_CSQ(unsigned short id) const {
        auto it = CSQ_cache_.find(id);
        return it != CSQ_cache_.end() ? it->second.cachedValue : 0.0f;
    }
    
    unsigned short Symetrix::getCachedTolerance_CSQ(unsigned short id) const {
        auto it = CSQ_cache_.find(id);
        return it != CSQ_cache_.end() ? it->second.tolerance : 0;
    }


    // Caché de datos de envío (CMV) --------------------------------------------------------

    bool Symetrix::isCached_CMV(unsigned short id) const {
        return CMV_cache_.find(id) != CMV_cache_.end();
    }

    void Symetrix::cacheValue_CMV(unsigned short id, float value) {
        CMV_cache_[id].cachedValue = value;
    }

    float Symetrix::getCachedValue_CMV(unsigned short id) const {
        auto it = CMV_cache_.find(id);
        return it != CMV_cache_.end() ? it->second.cachedValue : 0.0f;
    }
    
    unsigned short Symetrix::getCachedTolerance_CMV(unsigned short id) const {
        auto it = CMV_cache_.find(id);
        return it != CMV_cache_.end() ? it->second.tolerance : 0;
    }

    float Symetrix::getCachedTolerance_CMV_dB(float value, bool db_scale) const {
        /* Considera que cualquier valor CMV cacheado tiene la tolerancia = tolerance_percent_ */

        float dbValue = db_scale ? value : pct_to_dB(value);

        // Calcular la tolerancia en dB respecto al valor
        float valuedB_menos_tolerancia = pct_to_dB(std::max(0.0f, dbValue - tolerance_percent_));
        return std::abs(dbValue - valuedB_menos_tolerancia);
    }


    // Tolerancias (privado) ----------------------------------------------------------------

    void Symetrix::setTolerance_CSQ(unsigned short id, unsigned short newTolerance) {
        CSQ_cache_[id].tolerance = newTolerance;
    }

    void Symetrix::setTolerance_CMV(unsigned short id, unsigned short newTolerance) {
        CMV_cache_[id].tolerance = newTolerance;
    }


    // Envío de datos -----------------------------------------------------------------------

    bool Symetrix::shouldSendCSQ(unsigned short id, unsigned int newTicks) {

        // Si el valor no está cacheado, se debería enviar
        if (!isCached_CSQ(id))
            return true;

        // Comprobar si el cambio de valor supera la tolerancia evitando desbordamientos por resta
        int diff = static_cast<int>(newTicks) - static_cast<int>(getCachedValue_CSQ(id));
        return static_cast<unsigned int>(std::abs(diff)) >= getCachedTolerance_CSQ(id);
    }

    bool Symetrix::sendCSQ(unsigned short id, unsigned int ticks) {

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

    bool Symetrix::shouldSendCMV(unsigned short id, float dbValue) {

        // Si el valor no está cacheado, se debería enviar
        if (!isCached_CMV(id))
            return true;

        // Calcular la tolerancia en dB restando la tolerancia pasada a dB al valor
        float valuedB_menos_tolerancia = pct_to_dB(std::max(0.0f, dB_to_pct(dbValue) - tolerance_percent_));
        float toleranceDb = std::abs(dbValue - valuedB_menos_tolerancia);

        // Comprobar si el valor está dentro del rango
        return std::abs(dbValue - getCachedValue_CMV(id)) > toleranceDb;
    }

    bool Symetrix::sendCMV(unsigned short in, unsigned short out, float dbValue) {

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

struct Symetrix::Impl {};

// General ------------------------------------------------------------------------------
/* Común en ambos sistemas */
// Symetrix::Symetrix() {...}
Symetrix::~Symetrix() {}

// Ejecución ----------------------------------------------------------------------------
bool Symetrix::init(void* config) { 
    SYS_WARN("Symetrix", "Symetrix not compatible in non-Windows OS.");
    loadConfig(config); 
    return false; 
}
bool Symetrix::isInitialized() const    { return false; }
/* Común en ambos sistemas */
// Symetrix::loadConfig(void* config) {...}
void Symetrix::close()                { return; }
bool Symetrix::isConnected() const      { return false; }

// Parámetros ---------------------------------------------------------------------------
void Symetrix::setSupermatrixINs(unsigned int)  {}
void Symetrix::setSupermatrixOUTs(unsigned int) {}

// Comandos -----------------------------------------------------------------------------
bool Symetrix::LoadPreset(unsigned int) { return false; }

// Componentes: Controles generales -----------------------------------------------------
bool Symetrix::setValue(unsigned short, float, float, float)    { return false; }
bool Symetrix::setValue_dB(unsigned short, float, float, float) { return false; }
bool Symetrix::setButton(unsigned short, bool)                  { return false; }
bool Symetrix::setSupermatrixValue(unsigned int, unsigned int, float, bool) { return false; }

// Tolerancias --------------------------------------------------------------------------
void Symetrix::updateTolerancePct_CSQ(unsigned char, unsigned short)   {}
void Symetrix::updateTolerancePct_Supermatrix(unsigned short)         {}

// Inicialización privada ---------------------------------------------------------------
bool Symetrix::initConnection() { return false; }
void Symetrix::net_cleanup()    { return; }

// Conversión de datos a ticks ----------------------------------------------------------
unsigned int Symetrix::ValueToTicks(float, float, float)    { return 0; }
float Symetrix::pct_to_dB(float) const                      { return 0.0f; }
float Symetrix::dB_to_pct(float) const                      { return 0.0f; }

// Caché de datos de envío (CSQ) --------------------------------------------------------
bool Symetrix::isCached_CSQ(unsigned short) const             { return false; }
void Symetrix::cacheValue_CSQ(unsigned short, float)          { return; }
float Symetrix::getCachedValue_CSQ(unsigned short) const      { return 0.0f; }
unsigned short Symetrix::getCachedTolerance_CSQ(unsigned short) const    { return 0; }

// Caché de datos de envío (CMV) --------------------------------------------------------
bool Symetrix::isCached_CMV(unsigned short) const             { return false; }
void Symetrix::cacheValue_CMV(unsigned short, float)          { return; }
float Symetrix::getCachedValue_CMV(unsigned short) const      { return 0.0f; }
unsigned short Symetrix::getCachedTolerance_CMV(unsigned short) const    { return 0; }
float Symetrix::getCachedTolerance_CMV_dB(float, bool) const  { return 0.0f; } // Evita undefined reference si se declara en .hpp

// Tolerancias (privado) ----------------------------------------------------------------
void Symetrix::setTolerance_CSQ(unsigned short, unsigned short) {}
void Symetrix::setTolerance_CMV(unsigned short, unsigned short) {}

// Envío de datos -----------------------------------------------------------------------
bool Symetrix::shouldSendCSQ(unsigned short, unsigned int)  { return false; }
bool Symetrix::sendCSQ(unsigned short, unsigned int)        { return false; }
bool Symetrix::shouldSendCMV(unsigned short, float)         { return false; }
bool Symetrix::sendCMV(unsigned short, unsigned short,float){ return false; }

#endif
