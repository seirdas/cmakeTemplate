#pragma once

#include <memory>
#include <vector>
#include <chrono>           ///< Herramientas para medición de tiempo, duraciones y relojes de alta resolución.
#include <string>
#include <unordered_map>    ///< Mapa para valores cacheados


/**
 * @class Symetrix
 * Gestiona el dispositivo Symetrix a través de comandos nativos CSQ 
 *  (Change Controller Setting Quiet) a Symetrix Composer por UDP.
 */
class Symetrix {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar 
     */
    Symetrix();

    /**
     * @brief Destructor estándar 
     */
    ~Symetrix();


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicialización de la conexión a Symetrix por socket UDP
     * @param hostIp    IP local
     * @param force     Fuerza la inicialización 
     * @return 
     */
    bool Init(const std::wstring& hostIp, bool force = false);

    /**
     * @brief Cierra y limpia todos los componentes de la clase
     *  Esto incluye el WSA de Windows y el socket abierto
     */
    void Destroy();

    /**
     * @brief Devuelve si la Symetrix está conectada
     * @return @c true si está conectada, false en caso contrario
     */
    bool isConnected();


// Parámetros ---------------------------------------------------------------------------

    /**
     * @brief Establece el número de entradas de la supermatrix
     * @param num número de entradas de la supermatrix
     */
    void setSupermatrixINs(unsigned int num);

    /**
     * @brief Establece el número de salidas de la supermatrix
     * @param num número de salidas de la supermatrix
     */
    void setSupermatrixOUTs(unsigned int num);


// Comandos -----------------------------------------------------------------------------

    /**
     * @brief Carga un preset de Symetrix Composer
     * @param preset Número de preset
     * @return @c true Si se ha mandado el comando, false en caso contrario
     */
    bool LoadPreset(unsigned int preset);

    
// Componentes: Controles generales -----------------------------------------------------

    /**
     * @brief  Activa/Desactiva el botón de Mute de un componente
     * @note El id debe estar vinculado en Symetrix Composer al botón de mute de ese componente
     * @param id id del botón de mute del componente
     * @param mute true: Mute activado / false: Mute desactivado
     * @return @c true Si se ha mandado el comando, false en caso contrario
     */
    bool setMute(unsigned int id, bool mute);

    /**
     * @brief Controla el threshold de un componente
     * @note El id debe estar vinculado en Symetrix Composer al botón de threshold de ese componente
     * @param id id de los valores de threshold del componente
     * @param mute true: Mute activado / false: Mute desactivado
     * @param true_scale true para valor en rango real de symetrix [thresholdmin, thresholdMax], false para rango porcentual [0,100]
     * @return @c true Si se ha mandado el comando, false en caso contrario
     */
    bool setThreshold(unsigned int id, double value, bool true_scale = false);


// Componentes: Routers & Selectors -----------------------------------------------------

    // #TODO
    bool set_InputSelector_Selection(unsigned int id, unsigned int sel);


private:

// Inicialización privada ---------------------------------------------------------------

    /**
     * @brief Inicializar y comprobar la conexión red con Symetrix
     * @return @c true si la conexión ha sido correcta, false en caso contrario
     */
    bool initConnection(std::wstring const& hostIp);

    /**
     * @brief Inicializar la Tabla de búsqueda (LUT) de la SuperMatrix
     */
    void initSupermatrix();

    /**
     * @brief Limpia todos los elementos inicializados de la red (WSA, socket)
     */
    void net_cleanup();


// Conversión de datos ------------------------------------------------------------------

    /**
     * @brief Mapea un valor de la escala 1-100 a un rango personalizado [minVal, maxVal].
     * * @param value Valor en la escala de entrada (1.0 a 100.0).
     * @param minVal Límite inferior del rango de destino.
     * @param maxVal Límite superior del rango de destino.
     * @return float El valor convertido dentro del rango deseado.
     */
    float MapScale(float value, float minVal, float maxVal);

    /**
     * @brief Convierte un valor de ganancia en decibelios (dB) a su equivalente numérico en "ticks" (0-65535).
     * * Realiza una interpolación lineal basada en el rango de dB proporcionado y satura el resultado 
     * para garantizar que se mantenga dentro de los límites de 16 bits de Symetrix.
     * @param dB Valor actual en decibelios a convertir.
     * @param minDb Límite inferior en decibelios del rango del parámetro.
     * @param maxDb Límite superior en decibelios del rango del parámetro.
     * @return int El valor mapeado en "ticks" (entre 0 y 65535).
     */
    int dbToTicks(double dB, double minDb, double maxDb);

    /**
     * @brief Convierte una diferencia o umbral en decibelios (offset dB) a su equivalente en "ticks".
     *  Se utiliza principalmente para calcular el umbral de cambio mínimo (deadband/threshold) 
     * requerido para notificar o actualizar el estado de un parámetro. El valor devuelto está 
     * limitado a un rango mínimo de 1 tick.
     * @param offDb El offset o diferencial en decibelios. Si es <= 0.0, retorna 1.
     * @param minDb Límite inferior en decibelios del rango completo.
     * @param maxDb Límite superior en decibelios del rango completo.
     * @return int El valor del offset convertido a "ticks" (entre 1 y 65535).
     */
    int dbOffsetToTicks(double offDb, double minDb, double maxDb);


// Búsqueda de parámetros ---------------------------------------------------------------

    /**
     * @brief Busca un parámetro por su ID en la caché o le asigna una nueva entrada si no existe.
     * * Si el parámetro no está registrado y queda espacio libre en la caché (`kMaxEntries`), 
     * se inicializa una nueva entrada con valores por defecto y el tipo especificado.
     * @param id Identificador único del parámetro de Symetrix.
     * @param t Tipo de parámetro (e.g., volumen, mute, fader).
     * @param defaultOffsetTicks Umbral por defecto en ticks para filtrar cambios. Si es <= 0, se fuerza a 1.
     * @return int El índice de la entrada encontrada o asignada en la caché, o -1 si la caché está llena.
     */
    int findOrAlloc(int id, int defaultOffsetTicks);


// Envío de datos -----------------------------------------------------------------------

    /**
     * @brief Determina si la diferencia entre el nuevo valor y el último valor enviado supera el umbral.
     *  Sirve como filtro de transmisión para evitar saturar la red UDP si la variación no es 
     * significativa. Si es la primera vez que se evalúa (valor previo < 0), siempre autoriza el envío.
     * @param idx Índice del parámetro dentro de la caché.
     * @param newTicks El nuevo valor en "ticks" que se pretende enviar.
     * @return true Si se ha superado el umbral (offsetTicks) o no había valor previo en caché.
     * @return false Si el cambio es demasiado pequeño y no requiere envío.
     */
    bool shouldSend(int idx, int newTicks) const;

    /**
     * @brief Actualiza el valor de "ticks" almacenado en una entrada específica de la caché.
     *  Sincroniza el estado local de la aplicación con el último valor conocido o enviado al dispositivo.
     * @param idx Índice del parámetro dentro de la caché.
     * @param ticks El nuevo valor en "ticks" a persistir en la caché.
     * @return true Operación realizada con éxito.
     */
    bool setCached(int idx, int ticks);

    /**
     * @brief Envía un comando nativo CSQ (Change Controller Setting Quiet) al dispositivo Symetrix por UDP.
     *  El comando formatea la cadena de texto de manera eficiente en la pila (`CSQ <id> <ticks>\r`) 
     * sin generar tráfico de retorno (ACK/NAK) por parte del hardware de audio.
     * @param id Identificador del controlador remoto en el ecosistema Symetrix.
     * @param ticks El valor numérico final que se le va a asignar en el dispositivo (0-65535).
     * @return true Si todo el buffer del comando se transmitió correctamente por el socket.
     * @return false Si hubo un fallo en el formateo o en la llamada del sistema `send`.
     */
    bool sendCSQ(int id, int ticks);


/************ Variables ********************************************************/

    struct Impl;                        ///< Estructura PIMPL para el socket, para no depender de Windows en el header
    std::unique_ptr<Impl> pimpl_;       ///< Miembros dependientes de Windows (socket)

    // Entrada de cache
    struct CacheEntry { 
        int id{ 0 }; 
        int ticks{ -1 }; 
        int offsetTicks{ 1 }; 
    };

    // Tolerancias por defecto. Valores mínimos de cambio para mandar comando a Symetrix
    struct Tolerances {
        double ThresholdDb = 1.0;       // solo puertas de ruido
        double gainDb = 0.3;            // faders/ganancias
        int    selectorTicks = 1;       // selectores 2 estados
        int    superMatrixCenti = 30;   // centi-dB por cruce (dB/100)
    };

    
    // --- Conexión de socket ---
    using TimePoint = std::chrono::steady_clock::time_point;
    TimePoint           m_lastPingTime_;                ///< Registro temporal del último comando de control o ping enviado al hardware.
    bool                m_waitingPingResponse_;         ///< Bandera que indica si estamos esperando que el socket reciba el ACK del ping pendiente.
    bool                wsaStarted_;                    ///< Indica si la capa de red del sistema Windows (WSAStartup) se inicializó correctamente.
    bool                connected_;                     ///< Estado actual de la comunicación (true = conectado y respondiendo pings).
    bool                initialized_;                   ///< Bandera que indica si el sistema está inicializado.
    unsigned long const connection_ping_timeout_ms_;    ///< Tiempo de espera para recibir el ping de conexión con Symetrix
    unsigned short      ComposerPort_;                  ///< Puerto de conexión para el socket UDP

    // --- Valores límite soportados por los componentes Symetrix ---
    static constexpr double ThresholdMin_   = -48.0;    ///< Valor mínimo de threshold soportado por los componentes de Symetrix
    static constexpr double ThresholdMax_   = 0.0;      ///< Valor máximo de threshold soportado por los componentes de Symetrix
    static constexpr double GainMin_        = -72.0;    ///< Valor mínimo de ganancia soportado por los componentes de Symetrix
    static constexpr double GainMax_        = +12.0;    ///< Valor máximo de ganancia soportado por los componentes de Symetrix

    // --- Caché de Comandos de Control Único ---
    static constexpr int    kMaxTicks    = 2^16-1;  ///< Valor máximo de parámetro mapeado en "ticks" de 16 bits (2^16-1 = 65535)
    std::unordered_map<int, CacheEntry> cache_;                 ///< Array estático que funciona como caché local para evitar saturar el bus UDP con valores idénticos.

    // --- Configuración y Estado de la SuperMatrix ---
    int supermatrix_outs_;                          ///< Número de salidas lógicas de la SuperMatrix.
    int supermatrix_ins_;                           ///< Número de entradas lógicas de la SuperMatrix.
    int stepToCentidB_[101];                        ///< Tabla de búsqueda (LUT) para convertir pasos 0..100 a centi-dB.
    std::vector<int>            PrevCenti_;         ///< Buffer histórico de ganancias de los cruces de la matriz guardado en centi-dB para control de cambios.
    std::vector<unsigned short> ToleranceCenti_;    ///< Tolerancia de cambio guardada por celda expresada en centi-dB (extraída de m_offsets).

    // --- Buffers Temporales de Optimización (Zero-Allocation en Ejecución) ---
    std::vector<int>  m_tmpNewCenti;                ///< Array de almacenamiento temporal de valores objetivo en centi-dB durante el procesado por filas.
    std::vector<char> m_tmpChanged;                 ///< Máscara booleana temporal para marcar qué entradas de una fila han cambiado de valor.
    std::vector<int>  m_tmpUniq;                    ///< Lista temporal utilizada para agrupar y deduplicar valores de dB antes de empaquetar comandos agrupados.

    // --- Configuración Fija del Dispositivo ---
    Tolerances tolerances_;                              ///< Instancia que contiene las tolerancias de variación por defecto para cada tipo de dato.
    unsigned int const kBootPreset_;                ///< Número de preset de hardware (1..1000) que se invocará automáticamente al arrancar. Si es 0 se omite.
};