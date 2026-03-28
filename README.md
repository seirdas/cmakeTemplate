# Descripción
Este proyecto es una plantilla base diseñada para agilizar el inicio de proyectos en C++ moderno. Su objetivo principal es evitar que el desarrollador tenga que configurar manualmente el sistema de construcción, las pruebas y la gestión de dependencias cada vez que empieza un programa nuevo, y agilizar el desarrollo a partir de un entorno preparado para integrar librerías y usar diferentes configuraciones/compiladores según interés.

# Arquitectura de proyecto

- **.vscode**: Archivos de configuración para Visual Studio Code (o similares).  Permiten configurar y compilar los proyectos con distintas herramientas de compilación.
  * **settings.json**: Ajustes específicos del proyecto para el entorno Visual Studio Code (y *forks*).
  * **tasks.json**: Equivale a las opciones de *build* que se ejecutarán, al pulsar el icono de *build* o el shortcut `Ctrl+Shift+B`.
  * **launch.json**: Ejecuta el programa. Las configuraciones aparecen en el panel lateral izquierdo por defecto, que también aparece con el shortcut `Ctrl+Shift+D`. 
    Desde ahí, se puede configurar el depurador a lanzar, con el botón de símbolo de *Play* o `F5`. Dentro del *launch* hay un `preLaunchTask`, que es un task que se ejecutará antes de lanzar el programa.
- **_build**: Se genera automáticamente al configurar el proyecto con un preset específico (puede borrarse si hay fallos).
    Almacenan el proyecto generado desde la configuración (_.sln_ de visual studio o Makefiles)
- **dependencies**: Archivos de dependencias que serán copiados en la misma ruta del ejecutable al compilar (como `.json`, `.ini`, etc.).
- **executable**: Binarios ejecutables (`.exe`) que se generar al hacer un _build_ del proyecto.
- **_external**: Librerías externas descargadas a partir de CMake. Se genera automáticamente al configurar un proyecto (puede borrarse si hay fallos).
- **include**: Archivos de cabecera (`.h`).
- **resources**: Imágenes, iconos, etc. para usar en el proyecto. 
    Incluye el `resources.h` y el `resources.rc` que se compilan con el proyecto.
- **src**: Archivos de código fuente (`.cpp`).
- **toolchains**: Archivos de cmake específicos para distinguir tipos de compilaciones (MSVC, MinGW).
- **Archivos cMake**: Incluye `CMakeLists.txt` para cmake y `CMakePresets.json` para presets de cmake. 

# Dependencias

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
Se pueden descargar las herramientas de compilación MSVC de Visual Studio a partir de los siguientes enlaces/comandos:

| Herramienta           | Enlace web                                       | Comando cmd                                              |
| --------------------  | ------------------------------------------------ | -------------------------------------------------------- |
| Build Tools v16 2019  | https://aka.ms/vs/16/release/vs_BuildTools.exe   | `winget install Microsoft.VisualStudio.2019.BuildTools`  |
| Build Tools v17 2022  | https://aka.ms/vs/17/release/vs_BuildTools.exe   | `winget install Microsoft.VisualStudio.2022.BuildTools`  |
| Build Tools v16 2019  | https://aka.ms/vs/stable/vs_BuildTools.exe       | `winget install Microsoft.VisualStudio.BuildTools`       |

También se descargarán las herramientas de compilación de Visual Studio al instalar el IDE de Visual Studio. La versión instalada corresponderá con las herramientas de compilación corresponderá con la versión instalada (VS2019, VS2022, VS2026...). 

En este caso, principalmente habría que seleccionar **Desarrollo de aplicaciones de escritorio en C++** en las cargas de trabajo de Visual Studio Installer.

## MinGW & Clang

Una de las herramientas que se pueden utilizar para descargar el entorno de compilación de MinGW & Clang es **MSYS2**. Para instalarlo hay varias alternativas:

- [Desde aquí](https://www.msys2.org) 
- En Windows, desde _cmd_ con la herramienta `winget`:
  
  ```bash
  winget install MSYS.MSYS
  ```

Se descargará MSYS en `C:/msys64` por defecto. La mayoría de rutas del proyecto orientado a la compilación con esta herramienta apuntan a este directorio.

---

Una vez instalado, abrir `MSYS2 MinGW 64-bit shell` e instalar el _toolchain_ de MinGW-w64:

```bash
pacman -Syu         # Probablemente pida reiniciar MSYS.
pacman -S mingw-w64-x86_64-toolchain
```

 Por lo general, es más seguro instalar todas las dependencias (pulsando enter). Se descargará MinGW en `C:/msys64/mingw64` por defecto (tarda un rato).
 Los binarios como `gcc.exe` o `gdb.exe` estarán en la ruta `C:/msys64/mingw64/bin`.

La instalación de Clang es similar, pero con el _toolchain_ de Clang:

```bash
pacman -Syu         # Probablemente pida reiniciar MSYS.
pacman -S mingw-w64-clang-x86_64-toolchain
```

Se descargará Clang en `C:/msys64/clang64` por defecto (tarda un rato). Los binarios como `clang++.exe` o `llvm-windres.exe` estarán en la ruta `C:/msys64/clang64/bin`.

---

La ruta de mingw64 para los proyectos está definida en el preset de `CMakePresets.json`. Esta ruta sirve para configurar, construir y compilar el proyecto. Puede modificarse.

```json
"environment": {
    "MINGW_PATH": "C:/msys64/mingw64"
    }
```

En Visual Studio Code (o VSCodium), para usar los comandos de _build_ y _Run_ hay que establecer la ruta de `mingw64` y `clang64` desde la variable de `.vs/settings.json`. Vienen ya establecidas las rutas por defecto, pero se pueden modificar. Se leerá en el `miDebuggerPath` del `launch.json` (no hay necesidad de modificar este archivo).

```json
    // Custom paths for visual studio code launch
    "mingw.binPath": "C:/msys64/mingw64/bin",
    "clang.binPath": "C:/msys64/clang64/bin",
```

---

Para desinstalar los toolchains, desde la consola de _MSYS_:

```bash
pacman -Rs mingw-w64-x86_64-toolchain         # Desinstalar MinGW
pacman -Rs mingw-w64-clang-x86_64-toolchain   # Desinstalar Clang
```

### TL;DR

- Copia y pega este script en un cmd (con permisos de administrador) para descargar las herramientas de compilación mingw por msys [WIP, pendiente de revisar]:
  
  ```bash
  winget install msys2.msys2
  C:\msys64\msys2.exe -msys -c "pacman -Syu"
  C:\msys64\msys2.exe -msys -c "pacman -S mingw-w64-x86_64-toolchain"
  C:\msys64\msys2.exe -msys -c "pacman -S mingw-w64-clang-x86_64-toolchain"
  ```
  
## Ninja

Por defecto sólo se necesita `Ninja.exe`. El sistema está configurado para tomarlo del mismo proyecto en la carpeta _other/ninja.exe_, pero esta ruta es configurable desde los presets de cmake.
Las configuraciones de _MinGW Ninja_ y _Clang Ninja_ son más rápidas y están diseñadas para ser _multi-config_, es decir, con una sola configuración se puede generar el build de _Debug_ y _Release_.
Se puede personalizar la ruta de Ninja de MinGW y Clang en los presets:
  ```bash
  "environment": {
      "NINJA_PATH":"${sourceDir}/other"
  },
  ```

## Extensiones Visual Studio Code
- Cmake
- [ ] TODO

# Generación de proyectos

## Generación manual

El proyecto está diseñado para permitir la generación de soluciones o "entornos" _Makefile_ a partir de presets de CMake.
Lo habitual es configurar los proyectos en la carpeta `_build`, que habrá que crear si no está creada:

```bash
mkdir _build
cd _build
```

Dentro de la carpeta build se pueden generar las configuraciones del proyecto a partir de los presets definidos en `CMakePresets.json`.
Estas son algunas de las opciones posibles, desde los presets de `CMakePresets.json`:

```bash
cmake .. --preset vs2019 # Generar solución para Visual Studio 2019
cmake .. --preset vs2022 # Generar solución para Visual Studio 2022
cmake .. --preset vs2026 # Generar solución para Visual Studio 2026
cmake .. --preset mingw64 # Generar archivos Makefile para MinGW-w64 de MSYS
# etc...
```

## Generación con Visual Studio IDE

- [ ] TODO

## Generación con VSCode

- [ ] TODO

> Los comandos de la extensión CMake Tools están [aquí](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-settings.md)  
> Desinstalar ucrt:  pacman -Rs mingw-w64-ucrt-x86_64-toolchain  

> # TODO
> - [ ] README Documentar compatibilidades con Visual Studio, vscode y vscodium
> - [ ] README: NO AÑADIR ARCHIVOS/CLASES (.cpp, .h) DESDE VISUAL STUDIO DIRECTAMENTE
> - [ ] README: Documentar compilación en IDE Visual Studio.
> - [ ] README: Documentar depuración Clang LLDB en vscode con la extensión CodeLLDB.
> - [ ] Compilación Ninja Multi-Config MingW
> - [x] ~~Compilación Ninja Multi-config Clang~~
> - [x] ~~Compilación Clang `pacman -S mingw-w64-clang-x86_64-toolchain`~~
> - [x] ~~En Mingw al cambiar el main.cpp no funciona bien la recompilación.~~
> - [x] ~~El nombre de proyecto lo toma directamente del nombre de la carpeta~~
