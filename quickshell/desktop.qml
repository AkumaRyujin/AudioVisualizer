import QtQuick
import Quickshell
import Quickshell.Wayland
import AudioViewer 1.0

ShellRoot {
    id: root

    ConfigManager {
        id: cfgMgr
    }

    // Fuente de audio compartida para todos los monitores
    AudioSpectrumSource {
        id: sharedAudio
        barCount: 64
        sensitivity: 1.2
        gravity: 0.88
        targetFps: 60
        Component.onCompleted: sharedAudio.start()
    }

    // Monitorización ultra-ligera de cambios en la configuración (stat/timestamp sin I/O innecesario)
    Timer {
        id: reloadTimer
        interval: 250
        repeat: true
        running: true
        property var lastModMs: 0

        onTriggered: {
            var mod = cfgMgr.configLastModifiedMs();
            if (mod > 0 && mod !== lastModMs) {
                lastModMs = mod;
                var cfg = cfgMgr.loadConfig();
                applyConfigToAll(cfg);
            }
        }
    }

    function applyConfigToAll(cfg) {
        if (!cfg || Object.keys(cfg).length === 0) return;

        if (cfg.barCount !== undefined && cfg.barCount !== sharedAudio.barCount) {
            sharedAudio.barCount = cfg.barCount;
        }
        if (cfg.sensitivity !== undefined) sharedAudio.sensitivity = cfg.sensitivity;
        if (cfg.gravity !== undefined) sharedAudio.gravity = cfg.gravity;
        if (cfg.attackRate !== undefined) sharedAudio.attackRate = cfg.attackRate;
        if (cfg.spatialSmoothing !== undefined) sharedAudio.spatialSmoothing = cfg.spatialSmoothing;
        if (cfg.autoSleepEnabled !== undefined) sharedAudio.autoSleepEnabled = cfg.autoSleepEnabled;

        for (var i = 0; i < monitorVariants.instances.length; ++i) {
            var win = monitorVariants.instances[i];
            if (win && win.visualizer) {
                var v = win.visualizer;
                if (cfg.originAxis !== undefined) v.originAxis = cfg.originAxis;
                if (cfg.axisOffset !== undefined) v.axisOffset = cfg.axisOffset;
                if (cfg.frequencyLayout !== undefined) v.frequencyLayout = cfg.frequencyLayout;
                if (cfg.mirrorEnabled !== undefined) v.mirrorEnabled = cfg.mirrorEnabled;
                if (cfg.invertDirection !== undefined) v.invertDirection = cfg.invertDirection;
                if (cfg.showAxes !== undefined) v.showAxes = cfg.showAxes;
                if (cfg.autoFillScreen !== undefined) v.autoFillScreen = cfg.autoFillScreen;
                if (cfg.autoMaxThreshold !== undefined) v.autoMaxThreshold = cfg.autoMaxThreshold;
                if (cfg.reachPercent !== undefined) v.reachPercent = cfg.reachPercent;
                if (cfg.minThreshold !== undefined) v.minThreshold = cfg.minThreshold;
                if (cfg.maxThreshold !== undefined) v.maxThreshold = cfg.maxThreshold;
                if (cfg.showThresholdLine !== undefined) v.showThresholdLine = cfg.showThresholdLine;
                if (cfg.barWidth !== undefined) v.barWidth = cfg.barWidth;
                if (cfg.barSpacing !== undefined) v.barSpacing = cfg.barSpacing;
                if (cfg.maxBarLength !== undefined) v.maxBarLength = cfg.maxBarLength;
                if (cfg.visualStyle !== undefined) v.visualStyle = cfg.visualStyle;
                if (cfg.segmentHeight !== undefined) v.segmentHeight = cfg.segmentHeight;
                if (cfg.segmentGap !== undefined) v.segmentGap = cfg.segmentGap;
                if (cfg.showPeakCaps !== undefined) v.showPeakCaps = cfg.showPeakCaps;
                if (cfg.peakCapGap !== undefined) v.peakCapGap = cfg.peakCapGap;
                if (cfg.peakCapHeight !== undefined) v.peakCapHeight = cfg.peakCapHeight;
                if (cfg.waveLayers !== undefined) v.waveLayers = cfg.waveLayers;
                if (cfg.waveSpeed !== undefined) v.waveSpeed = cfg.waveSpeed;
                if (cfg.waveSpread !== undefined) v.waveSpread = cfg.waveSpread;
                if (cfg.dotSize !== undefined) v.dotSize = cfg.dotSize;
                if (cfg.dotGap !== undefined) v.dotGap = cfg.dotGap;
                if (cfg.gridRows !== undefined) v.gridRows = cfg.gridRows;
                if (cfg.stringGlow !== undefined) v.stringGlow = cfg.stringGlow;
                if (cfg.tiltPitch !== undefined) v.tiltPitch = cfg.tiltPitch;
                if (cfg.tiltYaw !== undefined) v.tiltYaw = cfg.tiltYaw;
                if (cfg.perspectiveDepth !== undefined) v.perspectiveDepth = cfg.perspectiveDepth;
                if (cfg.residualEnabled !== undefined) v.residualEnabled = cfg.residualEnabled;
                if (cfg.residualDecay !== undefined) v.residualDecay = cfg.residualDecay;
                if (cfg.residualOpacity !== undefined) v.residualOpacity = cfg.residualOpacity;
                if (cfg.residualOffset !== undefined) v.residualOffset = cfg.residualOffset;
                if (cfg.transparency !== undefined) v.transparency = cfg.transparency;
                if (cfg.customColors !== undefined && cfg.customColors.length > 0) {
                    v.colorPalette.setColorsFromHex(cfg.customColors);
                } else if (cfg.colorPreset !== undefined) {
                    v.colorPalette.loadPreset(cfg.colorPreset);
                } else {
                    v.colorPalette.applyHyprlandTheme();
                }
                if (cfg.colorMode !== undefined) v.colorMode = cfg.colorMode;
            }
        }
    }

    // Instancia en cada monitor activo
    Variants {
        id: monitorVariants
        model: Quickshell.screens

        PanelWindow {
            id: win
            property var modelData
            screen: modelData

            // Integración en la capa Bottom del compositor Wayland
            WlrLayershell.layer: WlrLayer.Bottom
            WlrLayershell.namespace: "audioviewer-desktop"

            anchors {
                top: true
                bottom: true
                left: true
                right: true
            }

            color: "transparent"

            property alias visualizer: visualizerItem

            CartesianVisualizer {
                id: visualizerItem
                anchors.fill: parent
                spectrumSource: sharedAudio
                showAxes: false
                autoFillScreen: false
                autoMaxThreshold: true
                mirrorEnabled: true
                residualEnabled: true
            }

            Component.onCompleted: {
                if (cfgMgr.hasSavedConfig()) {
                    var cfg = cfgMgr.loadConfig();
                    root.applyConfigToAll(cfg);
                } else {
                    visualizerItem.colorPalette.applyHyprlandTheme();
                }
            }
        }
    }
}
