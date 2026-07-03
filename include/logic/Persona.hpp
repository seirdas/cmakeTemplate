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
    }tm_;                       ///< Instancia de metadatos relacionados con entradas/salidas de totalmix


// Datos de entradas/salidas (symetrix)

    /**
     * @struct st_sym
     * @brief Estructura que agrupa los metadatos de entradas/salidas relacionados con Symetrix (Dante).
     */
    struct st_sym {
        short in;           ///< Número de entrada dante asociada
        short out;          ///< Número de salida dante asociada
    }sym_;                  ///< Instancia de metadatos relacionados con entradas/salidas dante asociadas a symetrix


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
