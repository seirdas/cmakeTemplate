// Habilita escribir en terminal sólo si se abre desde ella
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define ISATTY _isatty(_fileno(stdout))
#else
    #include <unistd.h>
    #define ISATTY isatty(STDOUT_FILENO)
#endif

