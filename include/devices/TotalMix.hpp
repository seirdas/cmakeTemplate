#pragma once

/**
 * @file TotalMix.hpp
 * @brief Control de RME TotalMix FX mediante protocolo OSC sobre UDP.
 * @note Como la aplicación TotalMix es exclusiva de Windows, se utilizarán
 *  las utilidades de gestión de sockets del propio Windows.
 *
 * @details
 * Configuración requerida en TotalMix FX antes de usar esta clase:
 *   - Options → Disable "Enable MIDI Control"
 *   - Options → Enable  "Enable OSC Control"
 *   - Options → Disable "Submix linked to OSC Control"
 *   - Options → Settings → OSC1 → TotalMix FX OSC Service:
 *       comprobar IP y port incoming (deben coincidir con localIP y localPort).
 *
 * @par Canales y banks
 * TotalMix agrupa los canales en banks de 8. El número de canales NO tiene
 * que ser múltiplo de 8: si el último bank está incompleto, sus canales se
 * numeran desde arriba (ej. 5 canales → posiciones 4,5,6,7,8 dentro del bank).
 * Puedes inicializar con cualquier valor, por ejemplo 82 entradas.
 *
 * @par Ejemplo de uso
 * @code
 *   TotalMix tm;
 *   tm.Init(7001, "127.0.0.1", 7002, "127.0.0.1", 18, 18, 18);
 *   tm.SetOutputVolume(1, -6.0f);       // Salida 1 a -6 dB
 *   tm.SetInputVolume(1, 1, -6.0f);     // Entrada 1 → Salida 1 a -6 dB
 *   tm.SetInputVolumePct(1, 1, 75.0f);  // Entrada 1 → Salida 1 al 75 %
 *   tm.SetMuteOutput(2, true);          // Mutear salida 2
 *   tm.SetSnapshot(1);                  // Cargar snapshot 1
 * @endcode
 */

#include <string>
#include <memory>
#include <array>


/**
 * @brief Controlador de RME TotalMix FX vía OSC/UDP.
 *
 * @details Encapsula la construcción de paquetes OSC y su envío por UDP
 * para controlar volúmenes, mutes, snapshots y parámetros de dinámica
 * de una interfaz de audio RME.
 */
class TotalMix
{
public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor.
     */
    TotalMix();

    /**
     * @brief Destructor. Cierra el socket UDP y llama a WSACleanup().
     */
    ~TotalMix();

    /**
     * @brief Constructor de copia eliminado para evitar copias.
     */
    TotalMix(const TotalMix&)            = delete;

    /**
     * @brief Operador de asignación de copia eliminado evitando sobreescrituras de clase instanciada.
     */
    TotalMix& operator=(const TotalMix&) = delete;


// Ejecución ------------------------------------------------------------------

    /**
     * @brief Inicializa Winsock, crea el socket UDP y lo enlaza a la dirección local.
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     * @return @c true cuando se ha inicializado correctamente, @c false en caso contrario.
     */
    bool init(void* config = nullptr);

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config = nullptr);


// Control de volumen -----------------------------------------------------------------------

    /**
     * @brief Ajusta el fader principal de una salida física en porcentaje.
     *
     * @param out Número de salida (1-based, máximo numOutputs).
     * @param value Nivel en porcentaje (0 … 100) o en dB si in_dB_units activo
     * @param in_db_units Indicador para considerar el valor como porcentaje (false) o dB (true)
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetOutputVolume(int out, float value, bool in_dB_units = false);

    /**
     * @brief Ajusta el volumen de una entrada en el submix de una salida, en porcentaje.
     *
     * @param out Número de salida destino del submix (1-based, máximo numOutputs).
     * @param in  Número de entrada (1-based, máximo numInputs).
     * @param value Nivel en porcentaje (0 … 100) o en dB si in_dB_units activo
     * @param in_db_units Indicador para considerar el valor como porcentaje (false) o dB (true)
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetInputVolume(int out, int in, float value, bool in_dB_units = false);

    /**
     * @brief Ajusta el volumen de un canal de playback en el submix de una salida, en porcentaje.
     *
     * @param out Número de salida destino del submix (1-based, máximo numOutputs).
     * @param pb  Número de canal de playback (1-based, máximo numPlaybacks).
     * @param pct Nivel en porcentaje (0 … 100) o en dB si in_dB_units activo
     * @param in_db_units Indicador para considerar el valor como porcentaje (false) o dB (true)
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetPlaybackVolume(int out, int pb, float pct, bool in_dB_units = false);


// Control de Mute ----------------------------------------------------------------------

    /**
     * @brief Activa o desactiva el mute de una salida física.
     *
     * @param out  Número de salida (1-based, máximo numOutputs).
     * @param mute @c true para mutear, @c false para desmutear.
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetMuteOutput(int out, bool mute);

    /**
     * @brief Activa o desactiva el mute de una entrada física.
     *
     * @param in   Número de entrada (1-based, máximo numInputs).
     * @param mute @c true para mutear, @c false para desmutear.
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetMuteInput(int in, bool mute);

    /**
     * @brief Activa o desactiva el mute de un canal de playback.
     *
     * @param pb   Número de canal de playback (1-based, máximo numPlaybacks).
     * @param mute @c true para mutear, @c false para desmutear.
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetMutePlayback(int pb, bool mute);


// Miscelánea ---------------------------------------------------------------------------

    /**
     * @brief Carga un snapshot de TotalMix FX.
     *
     * @param index Número de snapshot a activar (1-based).
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetSnapshot(int index);

    /**
     * @brief Ajusta el umbral (threshold) del expansor de una entrada física.
     *
     * @param in        Número de entrada (1-based, máximo numInputs).
     * @param threshold Valor de threshold en el rango que acepta TotalMix FX (0.0 … 1.0).
     * @return @c true si la inicialización fue exitosa.
     */
    bool SetInputThreshold(int in, float threshold);


private:

    // Declaración de estructuras privadas para usarlas en las funciones de abajo.
    enum class Bus;
    struct OscTimeTag;

// Envío de paquete OSC -----------------------------------------------------------------

    /**
     * @brief Construye y envía un paquete OSC de volumen para cualquier tipo de bus.
     *
     * @param bus     Bus de destino (Output, Input o Playback).
     * @param out     Salida destino del submix (ignorado si bus == Output).
     * @param channel Número de canal dentro del bus (1-based).
     * @param dB      Nivel en decibelios.
     * @return TMError correspondiente al resultado de la operación.
     */
    bool SendVolume(Bus bus, int out, int channel, float dB);

    /**
     * @brief Construye y envía un paquete OSC de mute para cualquier tipo de bus.
     *
     * @param bus     Bus de destino (Output, Input o Playback).
     * @param channel Número de canal dentro del bus (1-based).
     * @param mute    @c true para mutear, @c false para desmutear.
     * @return TMError correspondiente al resultado de la operación.
     */
    bool SendMute(Bus bus, int channel, bool mute);

    /**
     * @brief Envía el contenido actual del buffer OSC por UDP al host TotalMix FX.
     * @return TMError::OK si sendto() tuvo éxito; TMError::SocketError en caso contrario.
     */
    bool SendPacket();


// Banks --------------------------------------------------------------------------------
/*
 * TotalMix numera sus canales en grupos de 8 ("banks"), con posiciones 1–8.
 * Si el total de canales no es múltiplo de 8, el último bank incompleto
 * se rellena desde arriba:
 *   ej. 5 canales en el último bank → posiciones 4, 5, 6, 7, 8
 */

    /**
     * @brief Precalcula la posición dentro del bank OSC para cada canal de cada bus.
     * @details TotalMix divide los canales en grupos de 8 ("banks"). Esta función
     *          rellena bankPosOutput_, bankPosInput_ y bankPosPlayback_.
     */
    void BuildBankMaps();

    /**
     * @brief Devuelve el índice de inicio de bank para un canal dado.
     * @details El valor resultante se envía en el mensaje @c /setBankStart.
     * @param channel Número de canal (1-based).
     * @return Índice de inicio del bank (múltiplo de 8).
     */
    int BankStartFor(int channel) const;


// Utilidades ---------------------------------------------------------------------------

    /**
     * @brief Convierte un nivel en dB al valor de fader que espera TotalMix FX (0.0–1.0).
     * @param dB Nivel en decibelios (rango: -65 dB … +6 dB).
     * @return Valor de fader normalizado entre 0.0 y 1.0.
     */
    static float dBtoFader(float dB);

    /**
     * @brief Convierte un porcentaje de volumen a dB.
     * @details Escala logarítmica: 100 % → +6 dB, ~0 % → -64 dB.
     * @param pct Nivel en porcentaje (rango: 0.0 … 100.0).
     * @return Nivel en decibelios.
     */
    static float PctTodB(float pct);


// Métodos OSC --------------------------------------------------------------------------

    /** 
     * @brief Resetea el buffer OSC para empezar a construir un nuevo paquete. 
     */
    void OscReset();

    /**
     * @brief Devuelve los bytes libres disponibles en el buffer OSC.
     * @return Número de bytes disponibles.
     */
    int OscFreeSpace() const;

    /**
     * @brief Verifica que el próximo tipo en el type-string coincida con @p expected y avanza el cursor.
     * @param expected Carácter de tipo OSC esperado (@c 'f', @c 'i', @c 's', o @c '\\0').
     * @return @c true si el tipo coincide o no hay type-string activo; @c false en caso contrario.
     */
    bool OscCheckTag(char expected);

    /** 
     * @brief Escribe el tamaño del mensaje en curso en el campo de longitud reservado previamente. 
     */
    void OscPatchMsgSize();

    /**
     * @brief Copia un string en @p dest con padding a múltiplos de 4 bytes según el protocolo OSC.
     * @param dest Destino donde escribir el string.
     * @param str  String fuente terminado en null.
     * @return Número de bytes escritos (incluyendo padding).
     */
    int OscPadString(char* dest, const char* str);

    /**
     * @brief Calcula el número de bytes que ocupará un string en el formato OSC.
     * @param str String cuya longitud se quiere calcular.
     * @return Longitud del string más null-terminator, redondeada al siguiente múltiplo de 4.
     */
    int OscEffectiveStringLen(const char* str) const;

    /**
     * @brief Abre un bundle OSC con el timestamp indicado.
     * @param tt Timestamp OSC. Usar @c {0,1} para "ejecutar inmediatamente".
     * @return @c true si el bundle se abrió correctamente; @c false si hay desbordamiento o estado inválido.
     */
    bool OscOpenBundle(OscTimeTag tt);

    /**
     * @brief Cierra el bundle OSC abierto más recientemente.
     * @return @c true si se cerró correctamente; @c false si no había ningún bundle abierto.
     */
    bool OscCloseBundle();

    /**
     * @brief Cierra todos los bundles OSC abiertos.
     * @return @c true si al menos un bundle estaba abierto y se cerraron todos.
     */
    bool OscCloseAll();

    /**
     * @brief Escribe la dirección OSC y el string de tipos en el buffer.
     * @param name  Dirección OSC (ej. @c "/1/volume3").
     * @param types String de tipos con coma inicial (ej. @c ",f" para un float).
     * @return @c true si se escribió correctamente; @c false por desbordamiento o estado inválido.
     */
    bool OscWriteAddrAndTypes(const char* name, const char* types);

    /**
     * @brief Escribe un argumento de tipo float en el buffer OSC (big-endian).
     * @param val Valor float a escribir.
     * @return @c true si se escribió correctamente; @c false por desbordamiento o tipo incorrecto.
     */
    bool OscWriteFloat(float val);


/************ Variables ****************************************************************/

    static constexpr int OSC_MAX_BUNDLE_NESTING = 32;   ///< Máximo nivel de anidamiento de bundles OSC.
    static constexpr int OSC_STRING_ALIGN       = 4;    ///< Alineación de strings en el protocolo OSC (4 bytes).
    static constexpr int MAX_TOTAL_CHANNELS     = 256;  ///< Para los bankPos
    static constexpr int OSC_BUF_SIZE           = 1024; ///< Aumentado para mayor seguridad en paquetes complejos

    /** @brief Timestamp OSC de 64 bits (segundos + fracción en formato NTP). */
    struct OscTimeTag {
        unsigned int seconds  = 0; ///< Parte entera en segundos.
        unsigned int fraction = 1; ///< Fracción de segundo. Valor 1 significa "inmediatamente".
    };

    /** @brief Estados internos del buffer OSC durante la construcción de un paquete. */
    enum OscBufState {
        OSC_EMPTY,        ///< Buffer vacío, sin datos escritos.
        OSC_ONE_MSG_ARGS, ///< Paquete de un solo mensaje; recibiendo argumentos.
        OSC_NEED_COUNT,   ///< Bundle recién abierto; esperando mensaje o sub-bundle.
        OSC_GET_ARGS,     ///< Dentro de un mensaje; recibiendo argumentos.
        OSC_DONE          ///< Todos los bundles cerrados; paquete finalizado.
    };
        
    /** @brief Buffer de trabajo para construir paquetes OSC. */
    struct OscBuffer {
        char*       data         = nullptr;                  ///< Puntero al array de bytes subyacente.
        int         size         = 0;                        ///< Capacidad total del buffer en bytes.
        char*       ptr          = nullptr;                  ///< Posición actual de escritura.
        OscBufState state        = OSC_EMPTY;                ///< Estado de construcción del paquete.
        int*        thisMsgSize  = nullptr;                  ///< Puntero al campo de tamaño del mensaje en curso.
        int*        prevCounts[OSC_MAX_BUNDLE_NESTING] = {}; ///< Punteros a los campos de tamaño de cada bundle abierto.
        int         bundleDepth  = 0;                        ///< Nivel de anidamiento de bundles actual.
        OscTimeTag* outerStamp   = nullptr;                  ///< Puntero al timestamp del bundle más externo.
        char*       typePtr      = nullptr;                  ///< Cursor sobre el string de tipos OSC del mensaje en curso.
        bool        firstUntyped = false;                    ///< Indica si se espera el primer argumento sin tipo.
    };

    /** @brief Identifica el tipo de bus de audio en TotalMix. */
    enum class Bus {
        Output,   ///< Bus de salidas físicas.
        Input,    ///< Bus de entradas físicas.
        Playback  ///< Bus de canales de playback (software).
    };

    // Inicialización y ejecución
    bool wsaStarted_ = false;           ///< true si WSAStartup ya fue llamado con éxito.

    // Socket
    std::string     localIP_;           ///< IP Local de socket de envío de datos a Totalmix
    unsigned int    localPort_;         ///< Puerto local de socket de envío de datos a Totalmix
    unsigned int    remotePort_;        ///< Puerto UDP de TotalMix FX (destino).
    std::string     remoteIP_;          ///< IP de TotalMix FX (destino).
    struct Impl;                        ///< Estructura PIMPL para el socket, para no depender de Windows en el header
    std::unique_ptr<Impl> pimpl_;       ///< Miembros dependientes de Windows (socket)

    // Número de canales
    int numInputs_;                     ///< Número total de entradas del dispositivo.
    int numPlaybacks_;                  ///< Número total de canales de playback del dispositivo.
    int numOutputs_;                    ///< Número total de salidas del dispositivo.

    // Banks
    std::array<int, MAX_TOTAL_CHANNELS + 1> bankPosOutput_;     ///< Posición dentro del bank OSC para cada salida (1-based).
    std::array<int, MAX_TOTAL_CHANNELS + 1> bankPosInput_;      ///< Posición dentro del bank OSC para cada entrada (1-based).
    std::array<int, MAX_TOTAL_CHANNELS + 1> bankPosPlayback_;   ///< Posición dentro del bank OSC para cada canal de playback (1-based).

    // Buffer OSC
    char      oscRaw_[OSC_BUF_SIZE];            ///< Array de bytes subyacente del buffer OSC.
    OscBuffer oscBuf_;                          ///< Estructura de estado del buffer OSC en construcción.

};
