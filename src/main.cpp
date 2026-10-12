#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <string>
#include <csignal>

#include "audio/AudioSpectrumSource.h"
#include "graphics/CartesianVisualizer.h"
#include "graphics/ColorPalette.h"
#include "config/ConfigManager.h"
#include <QQmlContext>

namespace {
volatile std::sig_atomic_t g_keepRunning = 1;

void signalHandler(int) {
    g_keepRunning = 0;
}

void renderAsciiVisualizer(const QList<float>& spectrum, const QString& device) {
    static const char* blocks[] = {" ", " ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    constexpr int blockLevels = 8;
    constexpr int vizHeight = 10;
    const int numBars = spectrum.size();

    std::cout << "\033[H\033[?25l";
    std::cout << "=== AudioViewer - Modo Consola (Fase 1) ===\n";
    std::cout << "Dispositivo capturado: " << device.toStdString() << "\n";
    std::cout << "Barras activas: " << numBars << " | Presiona Ctrl+C para salir\n";
    std::cout << "--------------------------------------------------------\n";

    for (int row = vizHeight; row >= 1; --row) {
        float rowThreshold = static_cast<float>(row) / static_cast<float>(vizHeight);
        float prevThreshold = static_cast<float>(row - 1) / static_cast<float>(vizHeight);

        for (int i = 0; i < numBars; ++i) {
            float val = spectrum.at(i);
            if (val >= rowThreshold) {
                std::cout << blocks[blockLevels];
            } else if (val > prevThreshold) {
                int subLevel = static_cast<int>((val - prevThreshold) / (rowThreshold - prevThreshold) * blockLevels);
                subLevel = std::clamp(subLevel, 0, blockLevels);
                std::cout << blocks[subLevel];
            } else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }

    std::cout << "--------------------------------------------------------\n";
    std::cout << "Graves (20Hz)           Medios (1kHz)       Agudos (20kHz)\n";
    std::cout.flush();
}

} // namespace

int main(int argc, char* argv[]) {
    bool runCli = false;
    bool requestBackground = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--cli") {
            runCli = true;
        } else if (arg == "--background" || arg == "-b" || arg == "--bg") {
            requestBackground = true;
        } else if (arg == "--list-presets") {
            AudioViewer::ConfigManager cfgMgr;
            std::cout << "Presets disponibles en AudioViewer:\n";
            for (const QString& name : cfgMgr.availablePresets()) {
                std::cout << "  - " << name.toStdString();
                if (cfgMgr.isFactoryPreset(name)) std::cout << " (Fábrica)";
                std::cout << "\n";
            }
            return 0;
        } else if (arg == "--apply-preset" && i + 1 < argc) {
            QString presetName = QString::fromStdString(argv[++i]);
            AudioViewer::ConfigManager cfgMgr;
            QJsonObject presetCfg = cfgMgr.loadPreset(presetName);
            if (presetCfg.isEmpty()) {
                std::cerr << "Error: No se encontró el preset '" << presetName.toStdString() << "'.\n";
                return 1;
            }
            if (cfgMgr.hasSavedConfig()) {
                QJsonObject currentCfg = cfgMgr.loadConfig();
                if (currentCfg.contains("visualStyle")) {
                    presetCfg["visualStyle"] = currentCfg["visualStyle"];
                }
                if (currentCfg.contains("reachPercent")) {
                    presetCfg["reachPercent"] = currentCfg["reachPercent"];
                }
            }
            presetCfg["presetName"] = presetName;
            cfgMgr.saveConfig(presetCfg);
            std::cout << "Preset '" << presetName.toStdString() << "' aplicado exitosamente al escritorio.\n";
            return 0;
        } else if (arg == "--set-style" && i + 1 < argc) {
            std::string styleStr = argv[++i];
            int styleIdx = 0;
            if (styleStr == "spikes" || styleStr == "1" || styleStr == "agujas") styleIdx = 1;
            else if (styleStr == "peaks" || styleStr == "2" || styleStr == "picos") styleIdx = 2;
            else if (styleStr == "waves" || styleStr == "3" || styleStr == "ondas") styleIdx = 3;
            else if (styleStr == "blocks" || styleStr == "4" || styleStr == "bloques") styleIdx = 4;
            else if (styleStr == "dots" || styleStr == "5" || styleStr == "puntos" || styleStr == "cuentas") styleIdx = 5;
            else if (styleStr == "zigzag" || styleStr == "6") styleIdx = 6;
            else if (styleStr == "strings" || styleStr == "7" || styleStr == "cuerdas" || styleStr == "hilos") styleIdx = 7;
            else if (styleStr == "grid" || styleStr == "8" || styleStr == "vugrid" || styleStr == "matriz") styleIdx = 8;
            else styleIdx = 0;

            AudioViewer::ConfigManager cfgMgr;
            QJsonObject cfg = cfgMgr.hasSavedConfig() ? cfgMgr.loadConfig() : QJsonObject();
            cfg["visualStyle"] = styleIdx;
            cfgMgr.saveConfig(cfg);
            std::cout << "Estilo visual establecido a: " << styleStr << " (" << styleIdx << ")\n";
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Uso: AudioViewer [opciones]\n\n"
                      << "Opciones:\n"
                      << "  --background, -b         Ejecuta como ventana de fondo completa\n"
                      << "  --list-presets           Lista los perfiles y presets disponibles\n"
                      << "  --apply-preset <nombre>  Aplica un preset en caliente a la configuración\n"
                      << "  --set-style <estilo>     Cambia el estilo visual (bars, spikes, peaks, waves, blocks, dots, zigzag, strings, grid)\n"
                      << "  --cli                    Modo visualizador en consola/terminal\n"
                      << "  --help, -h               Muestra esta ayuda\n";
            return 0;
        }
    }

    if (runCli) {
        QCoreApplication app(argc, argv);
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        std::cout << "\033[2J";

        AudioViewer::AudioSpectrumSource audioSource;
        audioSource.setBarCount(64);
        audioSource.setSensitivity(1.2f);
        audioSource.setGravity(0.86f);
        audioSource.setTargetFps(30);

        QObject::connect(&audioSource, &AudioViewer::AudioSpectrumSource::spectrumUpdated,
                         [&audioSource](const QList<float>& spectrum) {
            if (!g_keepRunning) {
                QCoreApplication::quit();
                return;
            }
            renderAsciiVisualizer(spectrum, audioSource.currentDevice());
        });

        if (!audioSource.start()) {
            std::cerr << "Error al iniciar la captura de audio en PipeWire/PulseAudio.\n";
            return 1;
        }

        QTimer exitCheckTimer;
        QObject::connect(&exitCheckTimer, &QTimer::timeout, [&app]() {
            if (!g_keepRunning) {
                app.quit();
            }
        });
        exitCheckTimer.start(100);

        int result = app.exec();
        std::cout << "\033[?25h\n";
        return result;
    }

    // Modo GUI con Qt Quick y QML
    QGuiApplication app(argc, argv);
    app.setApplicationName("AudioViewer");
    app.setOrganizationName("AudioViewer");

    QQuickStyle::setStyle("Basic");

    // Registro de tipos para QML
    qmlRegisterType<AudioViewer::AudioSpectrumSource>("AudioViewer", 1, 0, "AudioSpectrumSource");
    qmlRegisterType<AudioViewer::CartesianVisualizer>("AudioViewer", 1, 0, "CartesianVisualizer");
    qmlRegisterType<AudioViewer::ColorPalette>("AudioViewer", 1, 0, "ColorPalette");
    qmlRegisterType<AudioViewer::ConfigManager>("AudioViewer", 1, 0, "ConfigManager");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("cliBackgroundRequested", requestBackground);
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject* obj, const QUrl& objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
