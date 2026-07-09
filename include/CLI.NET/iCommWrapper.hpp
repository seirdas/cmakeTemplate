#pragma once


#ifdef _MSC_VER

#using <system.dll>
//#using <.\dlls_32bits\iComm.dll>
//#using <.\dlls_32bits\iComm.iATC.dll>


/**
 * @class iCommWrapper
 * @brief Managed wrapper (C++/CLI) for .NET iComm dll
 *   Clase para compatibilidad con .NET de iComm
 *  especial para manejar los eventos y las funciones delegadas de la librer�a iComm
 *   Compatibilidad con iComm de .NET, clase autogestionada (managed) con el _ref class_
 *   Administrada con gc (garbage collector)
 *   Le pasa los datos necesarios a la logica del TTS.
 */
ref class iCommWrapper {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor
     */
    iCommWrapper();                            // Constructor

    /**
     * @brief Destructor
     */
    ~iCommWrapper();

private:

/************ Variables ********************************************************/

    // iComm::iCommManager^ iCommMgr = gcnew iComm::iCommManager();    // icomm manager pointer instance (.NET)
};

#endif