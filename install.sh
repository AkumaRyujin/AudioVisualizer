#!/usr/bin/env bash
# ==============================================================================
# AudioViewer - Automated Installation Script
# Proof of Concept (Alpha)
# ==============================================================================

set -euo pipefail

# Configuration
PREFIX="${PREFIX:-$HOME/.local}"
BUILD_DIR="build"
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Color definitions for output formatting
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    printf "${BLUE}[INFO]${NC} %s\n" "$1"
}

log_success() {
    printf "${GREEN}[SUCCESS]${NC} %s\n" "$1"
}

log_warn() {
    printf "${YELLOW}[WARN]${NC} %s\n" "$1"
}

log_error() {
    printf "${RED}[ERROR]${NC} %s\n" "$1" >&2
}

print_usage() {
    cat <<EOF
Usage: ./install.sh [OPTIONS]

Options:
    --install-deps, -d  Install all prerequisites using the system package manager
                        (Official support: Arch Linux with pacman)
    --prefix <path>     Specify installation prefix (default: \$HOME/.local)
    --uninstall         Uninstall AudioViewer and desktop configurations
    --no-autostart      Do not configure Hyprland autostart or keybindings
    --help              Show this help message
EOF
}

# Parse command line options
AUTOSTART=true
DO_UNINSTALL=false
INSTALL_DEPS=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --install-deps|-d)
            INSTALL_DEPS=true
            shift
            ;;
        --prefix)
            PREFIX="$2"
            shift 2
            ;;
        --uninstall)
            DO_UNINSTALL=true
            shift
            ;;
        --no-autostart)
            AUTOSTART=false
            shift
            ;;
        --help|-h)
            print_usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

uninstall_audioviewer() {
    log_info "Uninstalling AudioViewer from prefix: $PREFIX"

    # Kill running instances
    pkill -f "quickshell.*desktop.qml" 2>/dev/null || true
    pkill -x "AudioViewer" 2>/dev/null || true

    # Remove installed files
    rm -f "$PREFIX/bin/AudioViewer"
    rm -f "$PREFIX/bin/audioviewer-bg"
    rm -f "$PREFIX/bin/audioviewer-toggle"
    rm -f "$PREFIX/share/applications/audioviewer.desktop"
    rm -f "$PREFIX/share/icons/hicolor/scalable/apps/audioviewer.svg"
    rm -rf "$PREFIX/share/audioviewer"
    rm -f "$PREFIX/lib/libAudioViewerPlugin.so"* 2>/dev/null || true

    # Remove Hyprland autostart & bindings if present
    HYPR_AUTOSTART="$HOME/.config/hypr/autostart.lua"
    if [ -f "$HYPR_AUTOSTART" ]; then
        sed -i '/audioviewer-bg/d' "$HYPR_AUTOSTART"
        log_info "Removed autostart entry from $HYPR_AUTOSTART"
    fi

    HYPR_BINDINGS="$HOME/.config/hypr/bindings.lua"
    if [ -f "$HYPR_BINDINGS" ]; then
        sed -i '/Audio visualizer toggle/d' "$HYPR_BINDINGS"
        sed -i '/audioviewer-toggle/d' "$HYPR_BINDINGS"
        log_info "Removed keybinding entry from $HYPR_BINDINGS"
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
    fi

    log_success "AudioViewer has been uninstalled."
}

if [ "$DO_UNINSTALL" = true ]; then
    uninstall_audioviewer
    exit 0
fi

install_system_dependencies() {
    log_info "Installing system prerequisites..."
    if command -v pacman >/dev/null 2>&1; then
        log_info "Arch Linux detected. Installing official packages..."
        sudo pacman -S --needed --noconfirm \
            base-devel cmake ninja qt6-base qt6-declarative fftw libpulse hyprpicker quickshell
    elif command -v apt-get >/dev/null 2>&1; then
        log_warn "Debian/Ubuntu detected. AudioViewer is tested exclusively on Arch Linux with Hyprland. Proceeding at your own risk..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev \
            qml6-module-qtquick-controls libfftw3-dev libpulse-dev
    elif command -v dnf >/dev/null 2>&1; then
        log_warn "Fedora detected. AudioViewer is tested exclusively on Arch Linux with Hyprland. Proceeding at your own risk..."
        sudo dnf install -y \
            gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel \
            fftw-devel pulseaudio-libs-devel
    else
        log_error "Unsupported package manager. Please install dependencies manually."
        exit 1
    fi
    log_success "Prerequisites installed successfully."
}

if [ "$INSTALL_DEPS" = true ]; then
    install_system_dependencies
fi

# 1. Verification of build and runtime dependencies
log_warn "Compatibility Notice: AudioViewer is tested exclusively on Arch Linux with Hyprland. Usage on other distributions or window managers is at your own risk."
log_info "Checking dependencies..."

check_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        log_error "Required dependency '$1' not found."
        log_error "Tip: run './install.sh --install-deps' to install prerequisites automatically (Arch Linux)."
        exit 1
    fi
}

check_cmd cmake
check_cmd pkg-config

# Check C++ compiler
if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    log_error "No compatible C++ compiler found (g++ or clang++)."
    log_error "Tip: run './install.sh --install-deps' to install prerequisites automatically (Arch Linux)."
    exit 1
fi

# Check libraries via pkg-config
if ! pkg-config --exists libpulse; then
    log_error "PulseAudio development libraries (libpulse) not found."
    log_error "Tip: run './install.sh --install-deps' to install prerequisites automatically (Arch Linux)."
    exit 1
fi

if ! pkg-config --exists fftw3; then
    log_error "FFTW3 development libraries (fftw3) not found."
    log_error "Tip: run './install.sh --install-deps' to install prerequisites automatically (Arch Linux)."
    exit 1
fi

# Check for quickshell (required for native Hyprland wallpaper integration)
if ! command -v quickshell >/dev/null 2>&1; then
    log_warn "Quickshell is not installed. Layer-shell wallpaper visualizer requires quickshell."
    log_warn "Tip: run './install.sh --install-deps' to install quickshell (Arch Linux)."
else
    log_info "Found Quickshell: $(quickshell --version 2>/dev/null || echo 'installed')"
fi

# 2. Build configuration
log_info "Configuring build with CMake (prefix: $PREFIX)..."
cd "$REPO_DIR"

GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    PREV_GEN=$(grep "CMAKE_GENERATOR:INTERNAL=" "$BUILD_DIR/CMakeCache.txt" | cut -d= -f2 || true)
    if [ -n "$PREV_GEN" ] && [ "$PREV_GEN" != "$GENERATOR" ]; then
        log_info "Generator changed ($PREV_GEN -> $GENERATOR). Recreating build directory..."
        rm -rf "$BUILD_DIR"
    fi
fi

cmake -B "$BUILD_DIR" \
    -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX"

# 3. Compilation
log_info "Compiling AudioViewer..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

# 4. Installation
log_info "Installing to $PREFIX..."
cmake --install "$BUILD_DIR"

# Ensure execution permissions on scripts
chmod +x "$PREFIX/bin/AudioViewer"
chmod +x "$PREFIX/bin/audioviewer-bg"
chmod +x "$PREFIX/bin/audioviewer-toggle"

# 5. Desktop database & icon cache update
if command -v update-desktop-database >/dev/null 2>&1; then
    log_info "Updating desktop database..."
    update-desktop-database "$PREFIX/share/applications" 2>/dev/null || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    log_info "Updating icon cache..."
    gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" 2>/dev/null || true
fi

# 6. Hyprland autostart and keybinding configuration
if [ "$AUTOSTART" = true ]; then
    HYPR_CONFIG_DIR="$HOME/.config/hypr"
    
    if [ -d "$HYPR_CONFIG_DIR" ]; then
        # Check for Lua-based configuration (Omarchy)
        AUTOSTART_LUA="$HYPR_CONFIG_DIR/autostart.lua"
        if [ -f "$AUTOSTART_LUA" ]; then
            if ! grep -q "audioviewer-bg" "$AUTOSTART_LUA"; then
                log_info "Configuring autostart in $AUTOSTART_LUA..."
                printf '\no.launch_on_start("%s/bin/audioviewer-bg")\n' "$PREFIX" >> "$AUTOSTART_LUA"
            else
                log_info "Autostart already registered in $AUTOSTART_LUA"
            fi
        fi

        BINDINGS_LUA="$HYPR_CONFIG_DIR/bindings.lua"
        if [ -f "$BINDINGS_LUA" ]; then
            if ! grep -q "audioviewer-toggle" "$BINDINGS_LUA"; then
                log_info "Configuring SUPER + M keybinding in $BINDINGS_LUA..."
                printf '\n-- Desktop audio visualizer (AudioViewer)\no.bind("SUPER + M", "Audio visualizer toggle", "%s/bin/audioviewer-toggle")\n' "$PREFIX" >> "$BINDINGS_LUA"
            else
                log_info "Keybinding already registered in $BINDINGS_LUA"
            fi
        fi

        # Standard hyprland.conf support
        HYPR_CONF="$HYPR_CONFIG_DIR/hyprland.conf"
        if [ -f "$HYPR_CONF" ] && [ ! -f "$AUTOSTART_LUA" ]; then
            if ! grep -q "audioviewer-bg" "$HYPR_CONF"; then
                log_info "Configuring autostart in $HYPR_CONF..."
                printf '\n# AudioViewer background layer\nexec-once = %s/bin/audioviewer-bg\nbind = SUPER, M, exec, %s/bin/audioviewer-toggle\n' "$PREFIX" "$PREFIX" >> "$HYPR_CONF"
            fi
        fi
    fi
fi

# 7. Restart background layer if already running
if command -v quickshell >/dev/null 2>&1; then
    log_info "Starting AudioViewer desktop wallpaper visualizer layer..."
    pkill -f "quickshell.*desktop.qml" 2>/dev/null || true
    sleep 0.5
    setsid -f "$PREFIX/bin/audioviewer-bg" >/dev/null 2>&1 || true
fi

log_success "AudioViewer installation finished successfully!"
printf "\n"
printf "Usage Information:\n"
printf "  - Standalone GUI:     %s/bin/AudioViewer\n" "$PREFIX"
printf "  - Background Layer:   %s/bin/audioviewer-bg\n" "$PREFIX"
printf "  - Toggle Shortcut:    SUPER + M (or run '%s/bin/audioviewer-toggle')\n" "$PREFIX"
printf "\n"
