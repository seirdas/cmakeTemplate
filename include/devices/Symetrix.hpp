#pragma once

#include <memory>
#include <vector>
#include <chrono>           ///< Herramientas para medición de tiempo, duraciones y relojes de alta resolución.
#include <string>
#include <unordered_map>    ///< Mapa para valores cacheados


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


// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicialización de la conexión a Symetrix por socket UDP.
     * @param hostIp IP o alias DNS del dispositivo de destino.
     * @param force Fuerza la reinicialización de la red si ya estaba activo.
     * @return @c true cuando se ha inicializado y respondido al ping correctamente, @c false en caso contrario.
     */
    bool Init(const std::wstring& hostIp, bool force = false);

    /**
     * @brief Cierra y limpia todos los componentes de la clase.
     * @details Esto incluye la liberación de la capa de red WSA de Windows y el socket abierto.
     */
    void Destroy();

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


// Comandos -----------------------------------------------------------------------------

    /**
     * @brief Carga un preset de Symetrix Composer.
     * @param preset Número de preset (rango válido de 1 a 1000).
     * @return @c true si se ha mandado el comando con éxito, @c false en caso de error o parámetros inválidos.
     */
    bool LoadPreset(unsigned int preset);


// Componentes: Controles generales -----------------------------------------------------

    /**
     * @brief Establece un valor en un componente de Symetrix.
     * @details Convierte el valor actual a la escala interna de ticks del dispositivo, 
     *  comprueba la tolerancia para evitar saturar la red y actualiza la caché si se envía.
     * @param id Identificador único del componente/controlador remoto en Symetrix.
     * @param value Valor actual que se desea procesar y enviar.
     * @param minValue Límite inferior de la escala del parámetro original.
     * @param maxValue Límite superior de la escala del parámetro original.
     * @return @c true si el valor superó la tolerancia y se envió correctamente por red, @c false en caso contrario.
     */
    bool setValue(unsigned int id, float value, float minValue, float maxValue);

    /**
     * @brief Activa/Desactiva el estado binario (botón) de un componente.
     * @details Es equivalente a mandar un estado binario mapeado a la escala nativa de ticks.
     *  Configura automáticamente una tolerancia de 0 ticks en la caché al ser un valor discreto.
     * @param id Identificador único del componente/controlador remoto en Symetrix.
     * @param selection @c true para activar/habilitar, @c false para desactivar/mutear.
     * @return @c true si el comando CSQ se transmitió con éxito, @c false en caso contrario.
     */
    bool setButton(unsigned int id, bool selection);


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
    void updateTolerance(unsigned int newTolerance, unsigned int id = 0);


private:

// Inicialización privada ---------------------------------------------------------------

    /**
     * @brief Inicializar y comprobar la conexión de red con Symetrix mediante un intercambio de Ping.
     * @param hostIp Dirección IP o nombre de host remoto.
     * @return @c true si la conexión e intercambio inicial de buffers fue correcto, @c false en caso contrario.
     */
    bool initConnection(std::wstring const& hostIp);

    /**
     * @brief Inicializar la Tabla de búsqueda (LUT) de la SuperMatrix.
     * @details Genera de forma matemática la curva Gamma logarítmica para la conversión a centi-dB.
     */
    void initSupermatrix();

    /**
     * @brief Limpia todos los elementos inicializados de la red (WSA, socket).
     */
    void net_cleanup();


// Conversión de datos a ticks --------------------------------------------------------------

    /**
     * @brief Convierte un valor a su equivalente numérico en "ticks" (0-65535).
     *  Realiza una interpolación lineal basada en el rango proporcionado
     *  para garantizar que se mantenga dentro de los límites de 16 bits de Symetrix.
     * @param value Valor actual a convertir.
     * @param minValue Límite inferior del rango del parámetro.
     * @param maxValue Límite superior del rango del parámetro.
     * @return int El valor mapeado en "ticks" (entre 0 y 65535).
     */
    unsigned int ValueToTicks(float value, float minValue, float maxValue);


// Caché de datos de envío --------------------------------------------------------------

    /**
     * @brief Comprueba si un componente específico ya tiene una entrada registrada en la caché local.
     * @param id Identificador del componente a buscar.
     * @return @c true si existe en la caché, @c false en caso contrario.
     */
    bool isCached(unsigned int id) const;

    /**
     * @brief Actualiza el valor de "ticks" almacenado en una entrada específica de la caché.
     * @details Sincroniza el estado local de la aplicación con el último valor 
     *  conocido o enviado al dispositivo.
     * @param id Identificador único del componente en la caché.
     * @param currentTick El nuevo valor en "ticks" a persistir en la caché.
     */
    void cacheValue(unsigned int id, unsigned int currentTick);

    
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
    void setToleranceTicks(unsigned int id, unsigned int newToleranceTicks);


// Envío de datos -----------------------------------------------------------------------

    /**
     * @brief Determina si la diferencia entre el nuevo valor y el último valor enviado supera el umbral.
     * @details Sirve como filtro de transmisión para evitar saturar la red UDP si la variación no es
     * significativa. Si el elemento no existe en la caché, siempre autoriza el envío.
     * @param id Identificador único del componente.
     * @param newTicks El nuevo valor en "ticks" que se pretende evaluar.
     * @return @c true si se ha superado el umbral de tolerancia o no había registro previo, @c false si debe descartarse.
     */
    bool shouldSend(unsigned int id, unsigned int newTicks);

    /**
     * @brief Envía un comando nativo CSQ (Change Controller Setting Quiet) al dispositivo Symetrix por UDP.
     * @details El comando formatea la cadena de texto de manera eficiente en la pila (`CSQ <id> <ticks>\r`)
     * sin generar tráfico de retorno (ACK/NAK) por parte del hardware de audio.
     * @param id Identificador del controlador remoto en el ecosistema Symetrix.
     * @param ticks El valor numérico final que se le va a asignar en el dispositivo (0-65535).
     * @return @c true si todo el buffer del comando se transmitió correctamente por el socket, @c false en caso de fallo.
     */
    bool sendCSQ(unsigned int id, unsigned int ticks);


/************ Variables ********************************************************/

    struct Impl;                        ///< Estructura PIMPL para el socket, para no depender de Windows en el header
    std::unique_ptr<Impl> pimpl_;       ///< Miembros dependientes de Windows (socket)

    /** @brief Entrada de cache */
    struct CacheEntry {
        int cachedTicks = 99999;        ///< Último valor mandado a Symetrix (valor por defecto fuera de rango)
        int toleranceTick;              ///< Valor de tolerancia 
    };

    /** @brief Conexión de socket */
    using TimePoint = std::chrono::steady_clock::time_point;
    TimePoint           m_lastPingTime_;                ///< Registro temporal del último comando de control o ping enviado al hardware.
    bool                m_waitingPingResponse_;         ///< Bandera que indica si estamos esperando que el socket reciba el ACK del ping pendiente.
    bool                wsaStarted_;                    ///< Indica si la capa de red del sistema Windows (WSAStartup) se inicializó correctamente.
    bool                connected_;                     ///< Estado actual de la comunicación (true = conectado y respondiendo pings).
    bool                initialized_;                   ///< Bandera que indica si el sistema está inicializado.
    unsigned long const connection_ping_timeout_ms_;    ///< Tiempo de espera para recibir el ping de conexión con Symetrix
    unsigned short      ComposerPort_;                  ///< Puerto de conexión para el socket UDP


    // --- Conversión de datos a ticks ---
    unsigned int    minTickValue_    = 0;               ///< Valor mínimo de parámetro mapeado en "ticks" de 16 bits (2^16-1 = 65535)
    unsigned int    maxTickValue_    = 65535;           ///< Valor máximo de parámetro mapeado en "ticks" de 16 bits (2^16-1 = 65535)
    unsigned int    tolerance_percent_;                 ///< Porcentaje de tolerancia, si un valor cambia menos de este porcentaje respecto a su escala, no se mandará
    
    // --- Caché de Comandos de Control Único ---
    using CacheMap = std::unordered_map<unsigned int, CacheEntry>;
    CacheMap        cache_;                         ///< Vector de caché local para evitar saturar el bus UDP con valores idénticos o dentro de tolerancia

    // --- Configuración y Estado de la SuperMatrix ---
    int             supermatrix_ins_;               ///< Número de entradas lógicas de la SuperMatrix.
    int             supermatrix_outs_;              ///< Número de salidas lógicas de la SuperMatrix.
    int             stepToCentidB_[101];            ///< Tabla de búsqueda (LUT) para convertir pasos 0..100 a centi-dB.
    std::vector<int>            PrevCenti_;         ///< Buffer histórico de ganancias de los cruces de la matriz guardado en centi-dB para control de cambios.
    std::vector<unsigned short> ToleranceCenti_;    ///< Tolerancia de cambio guardada por celda expresada en centi-dB (extraída de m_offsets).

    // --- Buffers Temporales de Optimización (Zero-Allocation en Ejecución) ---
    std::vector<int>  m_tmpNewCenti;                ///< Array de almacenamiento temporal de valores objetivo en centi-dB durante el procesado por filas.
    std::vector<char> m_tmpChanged;                 ///< Máscara booleana temporal para marcar qué entradas de una fila han cambiado de valor.
    std::vector<int>  m_tmpUniq;                    ///< Lista temporal utilizada para agrupar y deduplicar valores de dB antes de empaquetar comandos agrupados.

    // --- Configuración Fija del Dispositivo ---
    unsigned int const kBootPreset_;                ///< Número de preset de hardware (1..1000) que se invocará automáticamente al arrancar. Si es 0 se omite.
};