#pragma once

#include <memory>
#include <chrono>           ///< Herramientas para medición de tiempo, duraciones y relojes de alta resolución.
#include <string>
#include <unordered_map>    ///< Mapa para valores cacheados
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


#define SYM_THRESHOLD_MIN   -48.0    ///< Valor mínimo de threshold soportado por los componentes de Symetrix
#define SYM_THRESHOLD_MAX   0.0      ///< Valor máximo de threshold soportado por los componentes de Symetrix
#define SYM_GAIN_MIN        -72.0    ///< Valor mínimo de ganancia soportado por los componentes de Symetrix
#define SYM_GAIN_MAX        +12.0    ///< Valor máximo de ganancia soportado por los componentes de Symetrix


/**
 * @class Symetrix
 * @brief Control de dispositivos Symetrix Composer mediante comandos nativos CSQ por UDP.
 * @details Convierte rangos lógicos dinámicos a la escala nativa del DSP (0-65535 ticks).
 * Filtra envíos redundantes mediante una caché interna y control de tolerancias.
 *
 * ### Ejemplo de uso:
 * @code
 * Symetrix sym;
 * if (sym.Init(L"192.168.1.50")) {
 * // Rango dinámico: admite cambiar los límites sobre la marcha para un mismo ID
 * sym.setValue(306, 1.0f, 1.0f, 4.0f);  // Escala [1, 4] -> 0 ticks
 * sym.setValue(306, 6.0f, 1.0f, 8.0f);  // Escala [1, 8] -> Proporcional
 * * // Control por tolerancia (Porcentaje del rango de ticks)
 * sym.updateTolerance(20); 
 * sym.setValue(301, 40.0f, 0.0f, 100.0f);
 * sym.setValue(301, -20.0f, SYM_THRESHOLD_MIN, SYM_THRESHOLD_MAX); // Valores reales dB
 * * // Botones binarios (Fuerzan automáticamente tolerancia cero)
 * sym.setButton(307, false);
 * sym.setButton(307, true);
 * }
 * @endcode
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
     *  Libera recursos y cierra socket
     */
    ~Symetrix();

    // Deshabilitar copia explícitamente (elimina warnings C4625 y C4626)
    Symetrix(Symetrix const&) = delete;
    Symetrix& operator=(Symetrix const&) = delete;

    // (Opcional) Si necesitas mover la instancia, habilita o elimina el movimiento:
    Symetrix(Symetrix&&) = delete;
    Symetrix& operator=(Symetrix&&) = delete;


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicialización de la conexión a Symetrix por socket UDP.
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config = nullptr);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config = nullptr);

    /**
     * @brief Cierra y limpia todos los componentes de la clase.
     * @details Esto incluye la liberación de la capa de red WSA de Windows y el socket abierto.
     * @return @c true Si se cierra correctamente, @c false en caso contrario
     */
    bool close();

    /**
     * @brief Devuelve si la Symetrix está conectada y respondiendo.
     * @return @c true si está conectada, @c false en caso contrario.
     */
    bool isConnected() const;


// Parámetros ---------------------------------------------------------------------------

    /**
     * @brief Establece el número de entradas de la supermatrix.
     * @param num Número de entradas de la supermatrix.
     */
    void setSupermatrixINs(unsigned int num);

    /**
     * @brief Establece el número de salidas de la supermatrix.
     * @param num Número de salidas de la supermatrix.
     */
    void setSupermatrixOUTs(unsigned int num);


// Control de componentes Symetrix Composer ---------------------------------------------

    /**
     * @brief Carga un preset de Symetrix Composer.
     * @param preset Número de preset (rango válido de 1 a 1000).
     * @return @c true si se ha mandado el comando con éxito, @c false en caso de error o parámetros inválidos.
     */
    bool LoadPreset(unsigned int preset);

    /**
     * @brief Establece un valor en un componente de Symetrix.
     * @details Convierte el valor actual a la escala interna de ticks del dispositivo, 
     *  comprueba la tolerancia para evitar saturar la red y actualiza la caché si se envía.
     * @param id Identificador único del componente/controlador remoto en Symetrix.
     * @param value Valor a establecer (dentro de escala de usuario).
     * @param minValue Límite inferior de la escala del parámetro original.
     * @param maxValue Límite superior de la escala del parámetro original.
     * @return @c true si el valor superó la tolerancia y se envió correctamente por red, @c false en caso contrario.
     */
    bool setValue(unsigned short id, float value, float minValue, float maxValue);

    /**
     * @brief Establece un valor para un componente Symetrix aplicando una curva de ponderación logarítmica.
     * @details Esta función mapea un valor de entrada (dentro del rango definido por minValue y maxValue)
     *  a la escala de ganancia en dB del DSP, utilizando una curva de corrección gamma
     *  para emular el comportamiento natural de los faders de audio.
     * @param id Identificador único del componente/controlador remoto en Symetrix.
     * @param value Valor a establecer (dentro de escala de usuario).
     * @param minValue Límite inferior de la escala del parámetro original.
     * @param maxValue Límite superior de la escala del parámetro original.
     * @return true si el valor fue enviado (o no requirió envío por estar dentro de la tolerancia), false en caso de error.
     */
    bool setValue_dB(unsigned short id, float value, float minValue, float maxValue);

    /**
     * @brief Activa/Desactiva el estado binario (botón) de un componente.
     * @details Es equivalente a mandar un estado binario mapeado a la escala nativa de ticks.
     *  Configura automáticamente una tolerancia de 0 ticks en la caché al ser un valor discreto.
     * @param id Identificador único del componente/controlador remoto en Symetrix.
     * @param selection @c true para activar/habilitar, @c false para desactivar/mutear.
     * @return @c true si el comando CSQ se transmitió con éxito, @c false en caso contrario.
     */
    bool setButton(unsigned short id, bool selection);

    /**
     * @brief Establece el volumen de un punto específico de la supermatriz (Crosspoint).
     * @details Permite enviar el valor de ganancia ya sea como un valor absoluto en dB o como un
     *  porcentaje (0-100) que se traduce mediante una curva logarítmica de audio.
     * @param in Índice de la entrada (1-indexed).
     * @param out Índice de la salida (1-indexed).
     * @param volume Valor de volumen: puede ser dB exactos o porcentaje (0-100).
     * @param real_scale Si es true, trata 'volume' como dB directos. Si es false (por defecto), 
     *  aplica una curva de ponderación logarítmica sobre el porcentaje.
     * @return true si el comando fue procesado y enviado con éxito, false en caso de error.
     */
    bool setSupermatrixValue(unsigned int in, unsigned int out, float volume, bool real_scale = false);


// Tolerancias --------------------------------------------------------------------------

    /**
     * @brief Actualiza el porcentaje de valor de tolerancia para el filtrado de envíos de [0,100]
     * @details Si se especifica un @p id distinto de cero, la tolerancia se modifica de forma exclusiva 
     * para ese componente dentro de la caché. Si @p id es cero (valor por defecto), se actualiza la 
     * tolerancia global por defecto y se aplica de forma masiva a todos los componentes 
     * actualmente registrados en la caché, afectando también a los que se añadan en el futuro.
     * @param newTolerance Nuevo valor o porcentaje de tolerancia a aplicar.
     * @param id Identificador único del componente en Symetrix. Si es @c 0, se aplica a todo el sistema.
     */
    void updateTolerancePct_CSQ(unsigned char newTolerance, unsigned short id = 0);

    /**
     * @brief Actualiza el porcentaje de valor de tolerancia para 
     *  el filtrado de envíos de valores de crosspoints Supermatrix
     * @param newTolerancePercent Nuevo porcentaje de tolerancia a aplicar.
     */
    void updateTolerancePct_Supermatrix(unsigned short newTolerancePercent);


private:

// Inicialización privada ---------------------------------------------------------------

    /**
     * @brief Inicializar y comprobar la conexión de red con Symetrix mediante un intercambio de Ping.
     * @return @c true si la conexión e intercambio inicial de buffers fue correcto, @c false en caso contrario.
     */
    bool init_connection();

    /**
     * @brief Comprueba la conexión con Symetrix mandando ping y esperando ACK
     * @note Preparado para lanzarse en un hilo independiente
     */
    void t_connection_checker();

    /**
     * @brief Limpia todos los elementos inicializados de la red (WSA, socket).
     */
    void net_cleanup();


// Conversión de datos ------------------------------------------------------------------

    /**
     * @brief Convierte un valor a su equivalente numérico en "ticks" (0-65535).
     *  Realiza una interpolación lineal basada en el rango proporcionado
     *  para garantizar que se mantenga dentro de los límites de 16 bits de Symetrix.
     * @param value Valor actual a convertir.
     * @param minValue Límite inferior del rango del parámetro.
     * @param maxValue Límite superior del rango del parámetro.
     * @return int El valor mapeado en "ticks" (entre 0 y 65535).
     */
    unsigned int value_to_ticks(float value, float minValue, float maxValue);

    /**
     * @brief Convierte un valor porcentual (0-100) a decibelios (dB) aplicando la curva gamma.
     * @param pct Valor de entrada en porcentaje [0, 100].
     * @return El valor equivalente en dB dentro del rango [SYM_GAIN_MIN, SYM_GAIN_MAX].
     */
    float pct_to_dB(float pct) const;

    /**
     * @brief Convierte un valor en decibelios (dB) a su equivalente porcentual (0-100).
     * @details Utiliza la operación inversa de la curva gamma para normalizar el valor.
     * @param dbValue Valor en dB.
     * @return Valor porcentual equivalente [0, 100].
     */
    float dB_to_pct(float dbValue) const;

// Caché de datos de envío (CSQ) --------------------------------------------------------

    /**
     * @brief Comprueba si un componente específico ya tiene una entrada registrada en la caché local.
     * @param id Identificador del componente a buscar.
     * @return @c true si existe en la caché, @c false en caso contrario.
     */
    bool is_cached_CSQ(unsigned short id) const;

    /**
     * @brief Guarda el valor de "ticks" o "dB" almacenado en una entrada específica de la caché.
     * @details Sincroniza el estado local de la aplicación con el último valor 
     *  conocido o enviado al dispositivo.
     * @param id Identificador único del componente en la caché.
     * @param currentTick El nuevo valor en "ticks" a persistir en la caché.
     */
    void cache_value_CSQ(unsigned short id, float value);

    /**
     * @brief Obtiene el valor bruto almacenado en la caché para un ID específico.
     * @note Esta función no verifica si el ID existe en el mapa de caché. Acceder a un ID 
     * no existente provocará una inserción por defecto (default construction) en el std::map.
     * @param id ID del componente.
     * @return El valor almacenado como entero.
     */
    float get_cached_value_CSQ(unsigned short id) const;

    /**
     * @brief Obtiene la tolerancia configurada para un ID específico en la caché.
     * @note Esta función no verifica si el ID existe en la caché. Se recomienda llamar 
     * a is_cached_CSQ() previamente si no se tiene certeza de la existencia del ID.
     * @param id ID del componente.
     * @return El valor de tolerancia en la escala correspondiente (ticks).
     */
    unsigned short get_cached_tolerance_CSQ(unsigned short id) const;


// Caché de datos de envío (CMV) --------------------------------------------------------

    /**
     * @brief Comprueba si un crosspoint de supermatrix ya tiene una entrada registrada en la caché local.
     * @param id Identificador del crosspoint supermatrix a buscar.
     * @return @c true si existe en la caché, @c false en caso contrario.
     */
    bool is_cached_CMV(unsigned short id) const;

    /**
     * @brief Guarda el valor "dB" de Crosspoint Supermatrix almacenado en una entrada específica de la caché.
     * @details Sincroniza el estado local de la aplicación con el último valor 
     *  conocido o enviado al dispositivo.
     * @param id Identificador único del componente en la caché.
     * @param currentTick El nuevo valor en "ticks" a persistir en la caché.
     */
    void cache_value_CMV(unsigned short id, float value);

    /**
     * @brief Obtiene el valor "dB" de Crosspoint Supermatrix almacenado en la caché para un ID específico.
     * @warning Esta función no verifica si el ID existe en el mapa de caché. Acceder a un ID 
     * no existente provocará una inserción por defecto (default construction) en el std::map.
     * @param id ID del componente.
     * @return El valor almacenado como entero.
     */
    float get_cached_value_CMV(unsigned short id) const;

    /**
     * @brief Obtiene la tolerancia configurada para un ID CMV específico en la caché.
     * @note Esta función no verifica si el ID existe en la caché. Se recomienda llamar 
     * a is_cached_CSQ() previamente si no se tiene certeza de la existencia del ID.
     * @param id ID del componente.
     * @return El valor de tolerancia en la escala correspondiente (ticks).
     */
    unsigned short get_cached_tolerance_CMV(unsigned short id) const;
    
    /**
     * @brief Obtiene el valor de tolerancia de ganancia de un 
     *  volumen crosspoint de Supermatrix respecto a otro
     * @details Se necesita un valor para calcular la tolerancia por
     *  el valor de ponderación de escala porcentual a escala logarítmica
     *  (esto significa que la tolerancia depende del valor en dB)
     * @param value Valor de referencia (porcentual o dB)
     * @param db_scale Magnitud del valor de referencia, @c true si es en escala dB, @c false en escala porcentual
     * @return Valor de tolerancia en dB (valor que se debe superar para mandar el dato)
     */
    float get_cached_tolerance_dB_CMV(float value, bool db_scale) const;
    
// Tolerancias (privado) ----------------------------------------------------------------

    /**
     * @brief Asigna el umbral de tolerancia directamente en unidades de "ticks" a un componente en la caché.
     * @details Este método privado escribe o sobrescribe de forma directa el campo `toleranceTick` de la 
     * estructura `CacheEntry` vinculada al @p id especificado. No realiza conversiones de escala ni cálculos 
     * porcentuales; el valor proporcionado se utilizará de forma literal en las comparaciones de diferencias 
     * absolutas dentro del filtro de transmisión.
     * @param id Identificador único del componente o controlador remoto en Symetrix.
     * @param newToleranceTicks Margen de tolerancia bruto expresado directamente en la escala de ticks (0-65535).
     */
    void set_tolerance_CSQ(unsigned short id, unsigned short newTolerance);

    /**
     * @brief Asigna el umbral de tolerancia directamente a un valor crosspoint de la caché.
     * @param id Identificador único del componente o controlador remoto en Symetrix.
     * @param newToleranceTicks Margen de tolerancia porcentual (0-100)
     */
    void set_tolerance_CMV(unsigned short id, unsigned short newTolerance);


// Envío de datos -----------------------------------------------------------------------

    /**
     * @brief Determina si la diferencia entre el nuevo valor y el último valor enviado supera el umbral.
     * @details Sirve como filtro de transmisión para evitar saturar la red UDP si la variación no es
     * significativa. Si el elemento no existe en la caché, siempre autoriza el envío.
     * @param id Identificador único del componente.
     * @param newTicks El nuevo valor en "ticks" que se pretende evaluar.
     * @return @c true si se ha superado el umbral de tolerancia o no había registro previo, @c false si debe descartarse.
     */
    bool should_send_CSQ(unsigned short id, unsigned int newTicks);

    /**
     * @brief Envía un comando nativo CSQ (Change Controller Setting Quiet) al dispositivo Symetrix por UDP.
     * @details El comando formatea la cadena de texto de manera eficiente en la pila (`CSQ <id> <ticks>\r`)
     * sin generar tráfico de retorno (ACK/NAK) por parte del hardware de audio.
     * @param id Identificador del controlador remoto en el ecosistema Symetrix.
     * @param ticks El valor numérico final que se le va a asignar en el dispositivo (0-65535).
     * @return @c true si todo el buffer del comando se transmitió correctamente por el socket, @c false en caso de fallo.
     */
    bool send_CSQ(unsigned short id, unsigned int ticks);

    /**
     * @brief Determina si un cambio en el valor en dB debe enviarse al Supermatrix según la tolerancia dinámica.
     * @details Calcula la tolerancia en dB de forma dinámica basándose en la posición actual 
     * en la curva logarítmica y la compara con el valor en caché.
     * @param id ID del componente (formateado para Supermatrix).
     * @param dbValue Valor nuevo en dB que se pretende enviar.
     * @return true si la diferencia es suficiente para requerir un nuevo envío, false si está dentro del umbral.
     */
    bool should_send_CMV(unsigned short id, float dbValue);

    /**
     * @brief Envía el comando CMV (Command Matrix Value) al Supermatrix para un punto de cruce.
     * @param in Índice de la entrada (0-indexed).
     * @param out Índice de la salida (0-indexed).
     * @param dbValue Valor de ganancia en dB a aplicar.
     * @return true si el paquete se envió correctamente a través del socket, false en caso de error.
     */
    bool send_CMV(unsigned short in, unsigned short out, float dbValue);


/************ Variables ********************************************************/

// Aliases
    using TimePoint = std::chrono::steady_clock::time_point;
    struct CacheEntry;
    using CacheMap = std::unordered_map<unsigned short, CacheEntry>;

// Pointer to implementation (PIMPL) para quitar includes del header
    struct Impl;                        ///< Estructura PIMPL para el socket, para no depender de Windows en el header
    std::unique_ptr<Impl> pimpl_;       ///< Miembros dependientes de Windows (socket)

// Inicialización y ejecución
    bool                initialized_;                   ///< Bandera que indica si el sistema está inicializado.
    bool                wsaStarted_;                    ///< Indica si la capa de red del sistema Windows (WSAStartup) se inicializó correctamente.
    std::atomic<bool>   running_;                       ///< flag de aplicación corriendo (para hilos)

// Conexión de socket
    bool                connected_;                     ///< Estado actual de la comunicación (true = conectado y respondiendo pings).
    TimePoint           m_lastPingTime_;                ///< Registro temporal del último comando de control o ping enviado al hardware.
    bool                m_waitingPingResponse_;         ///< Bandera que indica si estamos esperando que el socket reciba el ACK del ping pendiente.
    unsigned long       connection_ping_timeout_ms_;    ///< Tiempo de espera para recibir el ping de conexión con Symetrix
    unsigned short      ComposerPort_;                  ///< Puerto de conexión para el socket UDP
    std::string         SymetrixIP_;                    ///< IP de Symetrix
    std::thread         connection_checker_thread_;     ///< Hilo para "certificar" la conexión con Symetrix
    unsigned char       connection_check_seconds_;      ///< Intervalo de tiempo para comprobar la conexión con un ping
    std::mutex          connection_mutex_;              ///< Mutex para hilo de check conexión
    std::condition_variable connection_cv_;             ///< Condition variable para hilo de check conexión

// Conversión de datos
    float               dBcurve_gamma_;                 ///< Valor de ponderación de escala porcentual a escala logarítmica
    unsigned int const  minTickValue_;                  ///< Valor mínimo de parámetro mapeado en "ticks" de 16 bits (2^16-1 = 65535)
    unsigned int const  maxTickValue_;                  ///< Valor máximo de parámetro mapeado en "ticks" de 16 bits (2^16-1 = 65535)
    unsigned char       tolerance_percent_;             ///< Porcentaje de tolerancia, si un valor cambia menos de este porcentaje respecto a su escala, no se mandará
    
// Caché de Comandos de Control Único

    /** 
     * @brief Entrada de caché
     */
    struct CacheEntry {
        float           cachedValue = 99999;           ///< Último valor mandado a Symetrix de ticks/dB (valor por defecto fuera de rango)
        unsigned short  tolerance;                     ///< Valor de tolerancia (ticks en CSQ, pct en CMV)
    };
    CacheMap        CSQ_cache_;                         ///< Lista de caché local de elementos CSQ
    CacheMap        CMV_cache_;                         ///< Lista de caché local de elementos CMV (Supermatrix) 


// Configuración y Estado de SuperMatrix
    unsigned short  supermatrix_ins_;                   ///< Número de entradas lógicas de la SuperMatrix.
    unsigned short  supermatrix_outs_;                  ///< Número de salidas lógicas de la SuperMatrix.

// Configuración Fija del Dispositivo
    unsigned short  kBootPreset_;                       ///< Número de preset de hardware (1..1000) que se invocará automáticamente al arrancar. Si es 0 se omite.

};
