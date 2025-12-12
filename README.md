
# TODO
- [ ] README Documentar compatibilidades con Visual Studio, vscode y vscodium


---

# Generación de proyectos

## Generación manual
1. Se necesita descargar e instalar **cmake**. Se puede hacer:
- [Desde aquí](https://cmake.org/download/)
- Desde `winget`
```bash
winget install Kitware.CMake
```

El proyecto está diseñado para permitir la generación de soluciones o "entornos" _Makefile_ a partir de presets de CMake.
Lo habitual es configurar los proyectos en la carpeta `build`, que habrá que crear si no está creada:
```bash
mkdir build
cd build
```

Dentro de la carpeta build se pueden generar las configuraciones del proyecto a partir de los presets definidos en `CMakePresets.json`.
Estas son algunas de las opciones posibles:
```bash
cmake .. --preset vs2019 # Generar solución para Visual Studio 2019
cmake .. --preset vs2022 # Generar solución para Visual Studio 2022
cmake .. --preset vs2026 # Generar solución para Visual Studio 2026
cmake .. --preset mingw64 # Generar archivos Makefile para MinGW-w64 de MSYS
# etc...
```

# Configuraciones
## Configuraciones Visual Studio MSVC
Abrir la carpeta en Visual Studio y se generará automáticamente.
- [ ] Pendiente

## Configuración MSYS2 MinGW-x64
1. Se necesita descargar MSYS2. Se puede hacer:
- [Desde aquí](https://www.msys2.org) 
- Desde winget
    ```bash
    winget install MSYS.MSYS
    ```
Se descargará MSYS en `C:/msys64` por defecto. 
La mayoría de rutas del proyecto orientado a la compilación con esta herramienta apuntan a este directorio.

2. Abrir MSYS2 MinGW 64-bit shell e instalar el _toolchain_ de MinGW-w64:
    ```
    pacman -Syu
    pacman -S mingw-w64-x86_64-toolchain
    ```
Se descargará MinGW en `C:/msys64/mingw64` por defecto (tarda un rato).
Los binarios como `gcc.exe` o `gdb.exe` estarán en la ruta `C:/msys64/mingw64/bin`.


> Los comandos de la extensión CMake Tools están [aquí](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-settings.md)