
# Function to read the configuration file and set variables
function(readConfigFile FILE)
    # Check if the file exists
    if(NOT EXISTS ${FILE})
        message(FATAL_ERROR "Configuration file ${FILE} does not exist.")
    endif()

    # Read and parse the .ini file
    file(STRINGS "${FILE}" ini_contents)

    # Process each line
    foreach(line ${ini_contents})
        # Skip comments and empty lines
        if(line MATCHES "^;" OR line STREQUAL "")
            continue()
        endif()
        # Handle sections (though we won't use them directly)
        if(line MATCHES "^\\[.*\\]$")
            continue()
        endif()

        # Remove whitespace
        string(STRIP ${line} line)

        # Split key-value pairs
        if(line MATCHES "^(.*)=(.*)$")
            string(REGEX REPLACE "^(.*)=(.*)$" "\\1" key ${line})
            string(REGEX REPLACE "^(.*)=(.*)$" "\\2" value ${line})
            string(STRIP ${key} key)
            string(STRIP ${value} value)

            # Set the variable in CMake
            set(${key} "${value}" CACHE STRING "Set from config.ini")
            # message(STATUS "Set variable ${key} to ${value}")
        endif()
    endforeach()
endfunction()


