# AudioViewer

Visualizador cartesiano de espectro de audio de alto rendimiento diseñado para entornos Linux y compositores Wayland (optimizado para Hyprland), construido sobre C++20, Qt 6 / QML, FFTW3 y la infraestructura de audio de PipeWire / PulseAudio.

> [!WARNING]
> **Aviso de Estado del Proyecto: Prueba de Concepto (Estado Alfa)**  
> Este software es actualmente una **prueba de concepto (Proof of Concept - PoC)** y se encuentra en **fase alfa de desarrollo**. Está sujeto a cambios significativos de arquitectura, parámetros de configuración y puede presentar comportamientos no contemplados en configuraciones de escritorio no estándar. No se recomienda su uso en entornos de producción críticos.

---

## Características Principales

### 1. Motor de Captura y Procesamiento de Audio
- **Captura en tiempo real**: Ingesta continua y asíncrona mediante libpulse (compatible de forma nativa con PipeWire y PulseAudio) con reconexión automática ante cambios de dispositivo.
- **Transformada Rápida de Fourier (FFT)**: Procesamiento multi-banda acelerado mediante FFTW3 con distribución logarítmica inspirada en escalas melódicas y corrección psicoacústica.
- **Física balística y suavizado orgánico**:
  - Tasa de ataque progresiva (`attackRate`) con amortiguación de aceleración para evitar saltos caóticos en transitorios bruscos.
  - Suavizado espacial binomial de 3 tomas (`spatialSmoothing`) entre frecuencias contiguas para erradicar el ruido errático de bandas.
  - Disipación de gravedad (`gravity`) con decaimiento natural y mezcla ponderada (60% cuerpo espectral / 40% picos).
  - Modo reposo automático (`autoSleep`): suspensión de cálculo a 0% de CPU tras periodos prolongados de silencio.

### 2. Motor Gráfico Cartesiano 2D y 3D / 2.5D
- **Proyección matemática**: Soporte para orientación de origen en ejes X o Y, distribución de frecuencias lineal o desde el centro hacia los extremos (`CenterOut`), espejo simétrico (`mirror`) e inversión de sentido.
- **Transformación tridimensional**: Proyección con ángulos de inclinación (pitch), guiñada (yaw) y profundidad de perspectiva configurable.
- **9 estilos visuales integrados**:
  - Barras Clásicas (Solid Bars)
  - Onda de Picos (Spike Waveform)
  - Barras con Marcador de Pico (Peak Caps)
  - Ondas Armónicas (Harmonic Waves)
  - Bloques Segmentados LCD (Segmented Blocks)
  - Matriz de Puntos (Dot Matrix)
  - Onda Zig-Zag Continua (ZigZag Wave)
  - Cuerdas de Seda Tensadas (Silk Strings)
  - Matriz VU Grid (VU Grid Matrix)
- **Renderizado de estela residual**: Sombra residual con retención temporal y atenuación progresiva de opacidad.

### 3. Integración con el Entorno de Escritorio y Hyprland
- **Modo Fondo de Escritorio (`--background` / `-b`)**: Ejecución transparente en la capa inferior de Wayland (layer-shell) sin marco y sin interceptar eventos de entrada (`click-through`), permitiendo operar como fondo de pantalla activo.
- **Sincronización de tema en tiempo real**: Detección automática y observación reactiva de colores de sistema (Omarchy `colors.toml`, Pywal `colors.json` y borde activo de `hyprctl`). La ventana y los paneles adaptan su fondo, bordes y superficies al tema del usuario en vivo sin necesidad de reiniciar.
- **Herramienta Cuenta Gotas (Eyedropper)**: Integración con `hyprpicker` para capturar cualquier color de la pantalla directamente hacia la paleta de la aplicación (hasta 64 colores interpolables).
- **Modo Terminal / CLI (`--cli`)**: Renderizado en consola mediante caracteres de bloques Unicode de 8 niveles sin necesidad de servidor gráfico.

---

## Requisitos del Sistema

### Requisitos Mínimos
- **Sistema Operativo**: Linux (Kernel 5.15 o superior recomendado).
- **Servidor de Audio**: PipeWire (con `pipewire-pulse`) o PulseAudio en ejecución.
- **Compositor de Ventanas**: Wayland (probado exhaustivamente en Hyprland) o servidor X11.
- **Procesador**: Arquitectura x86_64 o ARM64 con soporte de instrucciones vectoriales básicas.
- **Memoria RAM**: 150 MB disponibles.

### Dependencias de Compilación
- Compilador con soporte C++20 (`gcc` 11+ o `clang` 13+)
- CMake 3.16 o superior
- Qt 6 (módulos `Core`, `Gui`, `Quick`, `Qml`, `QuickControls2`, `Widgets`)
- FFTW3 (`libfftw3-dev` en Debian/Ubuntu, `fftw` en Arch Linux)
- libpulse (`libpulse-dev` en Debian/Ubuntu, `libpulse` en Arch Linux)
- `hyprpicker` (opcional, para el selector de color de pantalla en Hyprland)

---

### Instalación Automática (Recomendada):
El repositorio incluye un script de instalación desatendido que detecta dependencias, compila, instala los binarios, iconos, integración de escritorio y configura el inicio automático y atajos en Hyprland / Omarchy:

```bash
# Otorgar permisos de ejecución si es necesario y ejecutar
./install.sh

# Opciones disponibles:
./install.sh --prefix ~/.local     # Prefijo personalizado (por defecto ~/.local)
./install.sh --uninstall           # Desinstalación limpia de binarios y configuraciones
./install.sh --no-autostart        # No registrar autostart ni atajos en Hyprland
```

### Compilación Manual con CMake:
```bash
# Configurar en modo optimizado Release
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"

# Compilar
cmake --build build -j$(nproc)

# Instalar
cmake --install build
```

El ejecutable principal se instalará en `~/.local/bin/AudioViewer`.

---

## Integración con Hyprland y Fondo de Escritorio

AudioViewer incluye integración nativa con la capa inferior del compositor Wayland (`zwlr_layer_shell_v1`, capa `bottom`) para mostrar las barras de espectro directamente sobre el fondo de pantalla en todos los monitores activos, con aceleración gráfica por hardware y sin depender de herramientas externas como `cava-bg`.

### Componentes de Integración:
- `audioviewer-bg`: Inicia el visualizador en la capa inferior de Wayland sobre el fondo de pantalla.
- `audioviewer-toggle`: Alterna (enciende/apaga) la visibilidad de las barras en el escritorio.
- **Atajo por defecto en Hyprland**: `SUPER + M` para alternar la visualización.
- **Inicio de sesión**: Configurado automáticamente en `~/.config/hypr/autostart.lua` o `hyprland.conf`.
- **Adaptación cromática reactiva**: Las barras adoptan inmediatamente la paleta del tema activo en Hyprland/Omarchy y se actualizan en caliente cuando el tema del sistema cambia.

---

## Uso y Opciones de Línea de Comandos

```bash
Uso: AudioViewer [opciones]

Opciones:
  --background, -b         Ejecuta en modo fondo de escritorio completo (Wayland layer-shell)
  --list-presets           Lista los perfiles y presets disponibles en el sistema
  --apply-preset <nombre>  Aplica un perfil guardado en caliente a la configuración activa
  --set-style <estilo>     Cambia el estilo visual (bars, spikes, peaks, waves, blocks, dots, zigzag, strings, grid)
  --cli                    Ejecuta el visualizador en consola/terminal sin interfaz gráfica
  --help, -h               Muestra el mensaje de ayuda
```

### Atajos de Teclado (Modo Ventana Interactiva)
| Tecla | Acción |
| :--- | :--- |
| `H` | Alternar visibilidad del panel lateral de ajustes |
| `B` | Alternar entre modo ventana y modo fondo de pantalla completo |
| `Esc` | Salir del modo pantalla completa o cerrar la ventana |

---

## Arquitectura del Código

- `src/audio/`: Gestor de captura asíncrona de muestras (`AudioSpectrumSource`) y procesamiento FFT / balística de movimiento (`FftProcessor`).
- `src/graphics/`: Lienzo de renderizado cartesiano 2D/3D (`CartesianVisualizer`) y gestión de paletas / temas dinámicos (`ColorPalette`).
- `src/config/`: Serialización y persistencia de perfiles y ajustes en disco (`ConfigManager`).
- `src/cli/`: Motor de visualización en terminal de comandos (`VisualizerCli`).
- `qml/`: Interfaz declarativa en Qt Quick / QML (`Main.qml`).

---

## Licencia

Este proyecto se distribuye bajo los términos de la licencia MIT. Consulta el archivo de licencia correspondiente para más detalles.
