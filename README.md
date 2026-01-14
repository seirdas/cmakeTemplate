
# TODO
- [ ] README Documentar compatibilidades con Visual Studio, vscode y vscodium
- [ ] README: NO AÑADIR ARCHIVOS/CLASES (.cpp, .h) DESDE VISUAL STUDIO DIRECTAMENTE
- [ ] Compilación Ninja / Ninja Multi-config
- [ ] Compilación Clang `pacman -S mingw-w64-clang-x86_64-toolchain`
- [x] ~~En Mingw al cambiar el main.cpp no funciona bien la recompilación.~~
- [x] ~~El nombre de proyecto lo toma directamente del nombre de la carpeta~~
---

# Dependencias (Qué hay que instalar)
## CMake
Este proyecto utiliza CMake, una herramienta de automatización de compilación multiplataforma que simplifica el proceso de configuración y generación de archivos de construcción. Gracias a su capacidad para gestionar las dependencias y diferentes configuraciones del entorno, CMake facilita la compilación del proyecto en (y para) múltiples sistemas operativos. 

Al implementar CMake, aseguramos que todos los desarrolladores usen una configuración unificada y consistente, mejorando la portabilidad y la mantenibilidad del código.

Se necesita descargar e instalar **cmake**. Se puede hacer:
- Desde [aquí](https://cmake.org/download/).
- Desde `winget` en cmd:
```bash
winget install Kitware.CMake
```



## Build Tools de Visual Studio (MSVC)
- [ ] TODO

## MinGW
1. Se necesita descargar MSYS2. Se puede hacer:
	- [Desde aquí](https://www.msys2.org) 
	- Desde `winget` en cmd:
    ```bash
    winget install MSYS.MSYS
    ```
Se descargará MSYS en `C:/msys64` por defecto. 
La mayoría de rutas del proyecto orientado a la compilación con esta herramienta apuntan a este directorio.

2. Abrir MSYS2 MinGW 64-bit shell e instalar el _toolchain_ de MinGW-w64:
    ```bash
    pacman -Syu
    pacman -S mingw-w64-x86_64-toolchain
    ```
Se descargará MinGW en `C:/msys64/mingw64` por defecto (tarda un rato).
Los binarios como `gcc.exe` o `gdb.exe` estarán en la ruta `C:/msys64/mingw64/bin`.


# Arquitectura de proyecto
- **.vscode**: Archivos de configuración para Visual Studio Code (o similares). 
    Permiten configurar y compilar los proyectos con distintas herramientas de compilación.
- **build**: Se genera automáticamente al configurar el proyecto con un preset específico. 
    Estos archivos se generan automáticamente, por lo que no hay que tocarlos.
- **config**: Archivos de configuración que serán copiados en la misma ruta del ejecutable (como `.json`, `.ini`, etc.).
- **executable**: Binarios ejecutables (`.exe`) que se generar al hacer un _build_ del proyecto.
- **include**: Archivos de cabecera (`.h`).
- **resources**: Imágenes, iconos, etc. para usar en el proyecto. 
    Incluye el `resources.h` y el `resources.rc` que se compilan con el proyecto.
- **src**: Archivos de código fuente (`.cpp`).
- **toolchains**: Archivos de cmake específicos para distinguir tipos de compilaciones (MSVC, MinGW).
- **Archivos cMake**: Incluye `CMakeLists.txt` para cmake y `CMakePresets.json` para presets de cmake. 


# Generación de proyectos

## Generación manual
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
cmake .. --preset mingw64 # Generar archivos Makefile para MinGW-w64 de MSYS
# etc...
```


## Generación con Visual Studio
Descargar y abrir la carpeta en un entorno IDE de Visual Studio.
- [ ] TODO

## Generación con VSCode
- [ ] TODO

### Arquitectura de configuración
Visual Studio Code se apoya en los siguientes archivos que están en la carpeta `.vscode`:
- **settings.json**:
- **tasks.json**:
#### launch.json
Ejecuta el programa. Las configuraciones aparecen en el panel lateral izquierdo por defecto, que también aparece con el shortcut `Ctrl+Shift+D`. 
Desde ahí, se puede configurar el depurador a lanzar, con el botón de símbolo de _Play_ o `F5`. 
> Al lanzar, aparecerá en la terminal que se ha ejecutado algo como `cmake --build [PATH] --config [CONFIG] --parallel 4`.
Habitualmente dentro del _launch_ hay un `preLaunchTask`, que es un task que se ejecutará antes de lanzar el programa.

### Configure

### Build
#### MSVC

#### MinGW
En Visual Studio Code (o VSCodium) hay que establecer la ruta de `Run` y `Build` desde la variable de  `.vs/settings.json`:
```json
    // Custom paths for visual studio code launch
    "mingw.binPath": "C:/msys64/mingw64/bin",
```
Que se leerá en el `miDebuggerPath` del `launch.json` (no hay necesidad de modificar este archivo).

## Generación con VSCodium (FOSS)
- [ ] TODO


# Configuraciones
## Configuraciones Visual Studio MSVC
- [ ] Pendiente

## Configuración MSYS2 MinGW-x64


La ruta de mingw64 para los proyectos está definida en el preset de CMakePresets.json:
```json
"environment": {
    "MINGW_PATH": "C:/msys64/mingw64"
    }
```
Esta ruta sirve para configurar, construir y compilar el proyecto.


> Los comandos de la extensión CMake Tools están [aquí](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-settings.md)