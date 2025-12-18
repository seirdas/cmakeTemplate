
# TODO
- [ ] README Documentar compatibilidades con Visual Studio, vscode y vscodium
- [ ] README: NO AÑADIR ARCHIVOS/CLASES (.cpp, .h) DESDE VISUAL STUDIO DIRECTAMENTE
- [ ] En Mingw al cambiar el main.cpp no funciona bien la recompilación.
- [x] El nombre de proyecto lo toma directamente del nombre de la carpeta
- [ ] Compilación Ninja / Ninja Multi-config
---



# Generación de proyectos

## Generación manual
1. Se necesita descargar e instalar **cmake**. Se puede hacer:
- Desde [aquí](https://cmake.org/download/).
- Desde `winget` en cmd:
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
Estas son las opciones posibles, desde los presets de `CMakePresets.json`:
```bash
cmake .. --preset vs2019 # Generar solución para Visual Studio 2019
cmake .. --preset vs2022 # Generar solución para Visual Studio 2022
cmake .. --preset vs2026 # Generar solución para Visual Studio 2026
cmake .. --preset mingw64 # Generar archivos Makefile para MinGW-w64 de MSYS [WIP]
# etc...
```

# Configuraciones
## Configuraciones Visual Studio MSVC
Abrir la carpeta en Visual Studio y se generará automáticamente.
- [ ] Pendiente

## Configuración MSYS2 MinGW-x64
1. Se necesita descargar MSYS2. Se puede hacer:
	- [Desde aquí](https://www.msys2.org) 
	- Desde `winget` en cmd:
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

La ruta de mingw64 para los proyectos está definida en el preset de CMakePresets.json:
```json
"environment": {
    "MINGW_PATH": "C:/msys64/mingw64"
    }
```
Esta ruta sirve para configurar, construir y compilar el proyecto.
Pero en Visual Studio Code (o VSCodium) hay que establecer la ruta de `Run` y `Build` desde la variable de  `.vs/settings.json`:
```json
    // Custom paths for visual studio code launch
    "mingw.binPath": "C:/msys64/mingw64/bin",
```
Que se leerá en el `miDebuggerPath` del `launch.json` (no hay necesidad de modificar este archivo).

> Los comandos de la extensión CMake Tools están [aquí](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-settings.md)