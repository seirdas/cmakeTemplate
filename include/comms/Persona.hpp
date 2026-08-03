#pragma once

#include <string>
#include <vector>


/**
 * @class Persona
 * @brief Gestiona la configuración de audio, información y estado de transmisión/recepción de un usuario.
 */
class Persona {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor por defecto
     */
    Persona();

    /**
     * @brief Destructor por defecto
     */
    ~Persona();


// Inicialización y ejecución -----------------------------------------------------------

    /**
     * @brief Inicializa la persona con sus datos
     * @param config Datos de configuración (diseñado para recibir un puntero a json)
     */
    bool init(void* config = nullptr);

    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la clase desde un objeto JSON.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);


// Configuración ------------------------------------------------------------------------

    /**
     * @brief Configura el índice de la entrada de micrófono.
     * @param in_index Índice de la entrada de micrófono a asignar.
     * @return true si la configuración fue exitosa, false en caso contrario.
     */
    bool set_mic_in(short in_index);

    /**
     * @brief Establece el volumen de los altavoces (speakers).
     * @param vol Nivel de volumen a asignar.
     */
    void set_spk_vol(short vol);


// Información --------------------------------------------------------------------------

    /**
     * @brief Vuelca la información de la persona en el log del sistema.
     */
    void logInfo();

    /**
     * @brief Obtiene el nombre de la persona.
     * @return Cadena de texto con el nombre.
     */
    std::string name();

    /**
     * @brief Comprueba si la persona está transmitiendo audio en el ciclo actual.
     * @return true si está transmitiendo, false en caso contrario.
     */
    bool isTransmiting();

    /**
     * @brief Comprueba si la persona está recibiendo audio en el ciclo actual.
     * @return true si está recibiendo, false en caso contrario.
     */
    bool isReceiving();


private:

/************ Variables ********************************************************/

// Inicialización y ejecución
    bool            initialized_;       ///< Bandera para indicar inicialización exitosa

// Datos de la persona
    std::string name_;              ///< Nombre de la persona
    bool        is_instructor_;     ///< flag que indica si es un instructor
    short       spk_vol;            ///< Volumen de speaker asociado


// Datos de entradas/salidas (totalmix)

    /**
     * @struct st_tm
     * @brief Estructura que agrupa los metadatos de entradas/salidas relacionados con TotalMix.
     */
    struct st_tm {
        std::vector<short> in;  ///< Número de entradas totalmix asociadas
        std::vector<short> out; ///< Número de salidas totalmix asociadas
        short fx;               ///< Número de entrada/salida de bucle de efectos
        short out_spk;          ///< Número de salida de speaker
    };
    
    st_tm   tm_;                ///< Instancia de metadatos relacionados con entradas/salidas de totalmix


// Datos de entradas/salidas (symetrix)

    /**
     * @struct st_sym
     * @brief Estructura que agrupa los ids asociados de Symetrix Composer.
     */
    struct st_sym {
        short sm_in;                ///< Número de entrada a Symetrix Supermatrix
        short sm_out;               ///< Número de salida a Symetrix Supermatrix

        short in_input_sel;         ///< ID de Input selector de entrada            (1-4)
        short in_vox_threshold;     ///< ID de VOX Gate: Threshold de entrada       (ThresholdMin-ThresholdMax)
        short in_fx_input_sel;      ///< ID de FX Input selector para fx de entrada (1-4)
        short out_gain_fader;       ///< ID de ganancia de salida                   (GAIN_MIN-GAIN_MAX)
        short out_gain_mute;        ///< ID de mute de ganancia de salida           (0-1)
    };

    st_sym    symIDs_;              ///< Instancia de metadatos de componentes de Symetrix Composer



// Identificadores de transmisión/recepción

    /**
     * @brief Metadatos de cada elemento de recepción
     */
    struct rx_data {
        unsigned long long  id;     ///< Identificador de recepción
        short               vol;    ///< Volumen de recepción
    };
    std::vector<rx_data>    RXs;    ///< Recepciones RX del ciclo actual
    
    /**
     * @brief Metadatos de cada elemento de transmisión
     */
    struct tx_data {
        unsigned long long  id;     ///< Identificador de recepción
    };
    std::vector<tx_data>    TXs;    ///< Transmisiones TX del ciclo actual

};
