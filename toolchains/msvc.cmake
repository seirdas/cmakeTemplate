	#
    # Configuraciones específicas para Visual Studio
    #

    # Especifica el proyecto principal
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

	# Allow for using folders
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Set runtime library to Multi-threaded DLL for MSVC
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Agrupar archivos de cabecera, fuente y recursos en carpetas
    source_group("Include"  FILES ${HEADERS})
    source_group("Source"   FILES ${SOURCES})
    source_group("Resources" FILES ${RESOURCES})
    