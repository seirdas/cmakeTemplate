#pragma once

#if defined CYCLONEDDSCXX || defined CYCLONEDDSCXX_VERSION

    #include "dds/dds.hpp"
    #include <functional>
    #include <iostream>
    #include "system/SystemMgr.hpp"

    // ==============================================================================
    // Declaración e Implementación con CycloneDDS Activo
    // ==============================================================================

    /**
     * @brief Listener genérico y asíncrono para la recepción de mensajes DDS.
     * @note Todas las funciones se definen en el hpp porque la mayoría usan el template
     * @details Hereda de la interfaz de CycloneDDS (`dds::sub::NoOpDataReaderListener`) 
     *  para gestionar la recepción de datos orientada a eventos mediante un patrón de callback.
     * 
     *  - Recepción asíncrona: Se vincula a un `DataReader` para escuchar el evento de 
     *    llegada de datos (`data_available`) sin necesidad de utilizar bucles de sondeo activos (`polling`).
     *  - Desacoplamiento mediante Callbacks: Extrae las muestras válidas de la red 
     *    y delega su procesamiento a una función o lambda externa (`std::function`), permitiendo 
     *    inyectar lógica personalizada desde cualquier otra capa de la aplicación.
     * 
     * @tparam T Tipo de dato IDL gestionado por el tópico
     */
    template <typename T>
    class CycloneListener : public dds::sub::NoOpDataReaderListener<T> {
    public:
        using DataCallback = std::function<void(const T& data)>;

    // General ------------------------------------------------------------------------------

        /**
         * @brief Constructor de clase 
         * @param cb 
         */
        explicit CycloneListener(DataCallback cb = nullptr) 
            : cb_(std::move(cb)) 
        {
            
        }

        /**
         * @brief Destructor de la clase
         */
        ~CycloneListener() override {
            clear_callback();
        }

    // Callbacks ----------------------------------------------------------------------------

        /**
         * @brief Registra un callback externo para procesar los datos de un topic específico.
         * * Este método permite inyectar una función o lambda desde el exterior
         * para que sea ejecutada cada vez que el hilo de audio tenga nuevos datos disponibles.
         * * @param cb La función (o functor) que se ejecutará al recibir nuevos frames.
         */
        void set_callback(DataCallback cb)  {
            std::lock_guard<std::mutex> lk(cb_mtx_);
            cb_ = std::move(cb);
        }

        void clear_callback() {
            
            if (!cb_) 
                return;

            std::lock_guard<std::mutex> lk(cb_mtx_);
            cb_ = nullptr;
        }


    // Listener -----------------------------------------------------------------------------

        /**
         * @brief Método que llama CycloneDDS asíncronamente cuando llega un paquete
         * @param reader 
         */
        void on_data_available(dds::sub::DataReader<T>& reader) override {
            try {
                auto samples = reader.take();
                for (const auto& sample : samples) 
                    if (sample.info().valid()) {
                        if (cb_) {
                            std::lock_guard<std::mutex> lk(cb_mtx_);
                            // Procesar el dato con la función inyectada
                            cb_(sample.data());
                        } else
                            SYS_WARN("CycloneListener","Data received, null callback");
                    }

            } catch (const dds::core::Exception& e) {
                SYS_WARN("DDSListener","Listener exception: " std::string(e.what));
            }
        }

    private:
        DataCallback    cb_;        ///< Función callback inyectada para ejecutar cuando llega un paquete
        std::mutex      cb_mtx_;    ///< Mutex para función callback
    };


#else

// ==============================================================================
// Dummy / Stub de la Clase cuando CycloneDDS está DESACTIVADO
// ==============================================================================

    /**
     * @brief Clase dummy del Listener de CycloneDDS (cuando la librería está desactivada)
     */
    template <typename T>
    class CycloneListener {
    public:
    // General ------------------------------------------------------------------------------
        using DataCallback = std::function<void(const T& data)>;
        explicit CycloneListener(DataCallback cb = nullptr) { return; }
    // Callbacks ----------------------------------------------------------------------------
        void set_callback(DataCallback cb)                  { return; }
    // Listener -----------------------------------------------------------------------------
        template <typename DummyReader>
        void on_data_available(DummyReader& reader)         { return; }
    };

#endif
