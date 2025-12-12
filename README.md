pendiente


# Configuración MSYS2 MinGW-x64
1. Se necesita descargar MSYS2. Se puede hacer:
- [Desde aquí](https://www.msys2.org) 
- Desde winget
    ```
    winget install MSYS.MSYS
    ```
Se descargará MinGW en `C:/msys64` por defecto.

2. Abrir MSYS2 MinGW 64-bit shell e instalar el toolchain de MinGW-w64:
    ```
    pacman -Syu
    pacman -S mingw-w64-x86_64-toolchain
    ```
Se descargará MinGW en `C:/msys64/mingw64` por defecto. Los binarios como `gcc.exe` o `gdb.exe` estarán en la ruta `C:/msys64/mingw64/bin`.


> Los comandos de la extensión CMake Tools están [aquí](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-settings.md)