import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import AudioViewer 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 800
    title: "AudioViewer"

    // Modo Fondo de Escritorio integrado
    property bool backgroundMode: (typeof cliBackgroundRequested !== "undefined") ? cliBackgroundRequested : false

    flags: backgroundMode ? (Qt.FramelessWindowHint | Qt.WindowStaysOnBottomHint | Qt.WindowTransparentForInput)
                          : (Qt.Window)
    color: backgroundMode ? "transparent" : (visualizer ? visualizer.colorPalette.themeBackgroundColor : "#0a0a12")
    visibility: backgroundMode ? Window.FullScreen : Window.AutomaticVisibility

    ConfigManager {
        id: cfgMgr
    }

    AudioSpectrumSource {
        id: audioSource
        barCount: 64
        sensitivity: 1.2
        gravity: 0.92
        attackRate: 0.45
        spatialSmoothing: 0.32
        targetFps: 60
        Component.onCompleted: audioSource.start()
    }

    // Monitorización ultra-ligera de cambios externos en la configuración (stat sin I/O innecesario)
    Timer {
        id: externalConfigWatcher
        interval: 300
        repeat: true
        running: true
        property var lastModMs: 0

        onTriggered: {
            var mod = cfgMgr.configLastModifiedMs();
            if (mod > 0 && mod !== lastModMs) {
                lastModMs = mod;
                var cfg = cfgMgr.loadConfig();
                applyConfigObject(cfg);
            }
        }
    }

    // Lienzo del Visualizador Cartesiano
    Item {
        anchors.fill: parent

        CartesianVisualizer {
            id: visualizer
            anchors.fill: parent
            spectrumSource: audioSource
            originAxis: axisCombo.currentIndex === 0 ? CartesianVisualizer.AxisX : CartesianVisualizer.AxisY
            frequencyLayout: freqCombo.currentIndex === 0 ? CartesianVisualizer.CenterOut : CartesianVisualizer.Linear
            mirrorEnabled: mirrorCheck.checked
            invertDirection: invertCheck.checked
            showAxes: showAxesCheck.checked
            autoFillScreen: autoFillCheck.checked
            autoMaxThreshold: reachPercentSlider.value >= 0.99
            reachPercent: reachPercentSlider.value
            axisColor: Qt.rgba(0.2, 0.8, 1.0, 0.35)

            // Nivel del eje de nacimiento (offset respecto al centro)
            axisOffset: axisOffsetSlider.value

            // Umbrales de barras
            minThreshold: minThreshSlider.value
            maxThreshold: 1.0
            showThresholdLine: showThreshLineCheck.checked
            thresholdLineColor: Qt.rgba(1.0, 0.35, 0.45, 0.6)

            // Geometría y Estilos Visuales
            visualStyle: styleCombo.currentIndex
            segmentHeight: segHeightSlider.value
            segmentGap: segGapSlider.value
            showPeakCaps: (styleCombo.currentIndex === 2) || showPeakCapsCheck.checked
            peakCapGap: peakCapGapSlider.value
            peakCapHeight: peakCapHeightSlider.value
            waveLayers: Math.round(waveLayersSlider.value)
            waveSpeed: waveSpeedSlider.value
            waveSpread: waveSpreadSlider.value
            dotSize: dotSizeSlider.value
            dotGap: dotGapSlider.value
            gridRows: Math.round(gridRowsSlider.value)
            stringGlow: stringGlowSlider.value
            barWidth: barWidthSlider.value
            barSpacing: barSpacingSlider.value

            // Perspectiva 3D
            tiltPitch: pitchSlider.value
            tiltYaw: yawSlider.value
            perspectiveDepth: depthSlider.value

            // Sombra residual y transparencia
            residualEnabled: residualCheck.checked
            residualDecay: decaySlider.value
            residualOpacity: resOpacitySlider.value
            residualOffset: resOffsetSlider.value
            transparency: globalAlphaSlider.value

            colorMode: colorModeCombo.currentIndex
        }

        // Overlay de información en modo interactivo (oculto en modo fondo)
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 16
            color: Qt.rgba(visualizer.colorPalette.themeBackgroundColor.r, visualizer.colorPalette.themeBackgroundColor.g, visualizer.colorPalette.themeBackgroundColor.b, 0.85)
            border.color: Qt.rgba(visualizer.colorPalette.themeAccentColor.r, visualizer.colorPalette.themeAccentColor.g, visualizer.colorPalette.themeAccentColor.b, 0.35)
            border.width: 1
            radius: 8
            visible: !root.backgroundMode
            implicitWidth: statusCol.implicitWidth + 24
            implicitHeight: statusCol.implicitHeight + 16

            ColumnLayout {
                id: statusCol
                anchors.centerIn: parent
                spacing: 4

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: !audioSource.isRunning ? "#ff3366"
                             : audioSource.isSleeping ? "#ffaa00" : "#00ff88"
                    }
                    Text {
                        text: !audioSource.isRunning ? "Detenido"
                            : audioSource.isSleeping ? "Modo Reposo (Silencio - 0% CPU)"
                            : "Capturando audio en vivo"
                        color: audioSource.isSleeping ? "#ffcc44" : "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Text {
                    text: "Dispositivo: " + audioSource.currentDevice
                    color: "#8899aa"
                    font.pixelSize: 10
                    Layout.maximumWidth: 320
                    elide: Text.ElideRight
                }

                Text {
                    text: "[H] Panel | [B] Modo Fondo de Pantalla Completa | [Esc] Salir"
                    color: "#00e5ff"
                    font.pixelSize: 10
                    font.bold: true
                }
            }
        }
    }

    // Botón flotante para alternar ajustes (solo visible en modo ventana)
    RoundButton {
        id: togglePanelBtn
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 16
        text: controlDrawer.visible ? "Cerrar" : "Ajustes"
        font.pixelSize: 13
        font.bold: true
        palette.button: visualizer.colorPalette.themeSurfaceColor
        palette.buttonText: visualizer.colorPalette.themeAccentColor
        visible: !root.backgroundMode
        onClicked: controlDrawer.visible = !controlDrawer.visible
        z: 100
    }

    // Atajos de teclado
    Shortcut {
        sequence: "H"
        onActivated: {
            if (!root.backgroundMode) {
                controlDrawer.visible = !controlDrawer.visible
            }
        }
    }

    Shortcut {
        sequence: "B"
        onActivated: {
            root.backgroundMode = !root.backgroundMode
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (root.backgroundMode) {
                root.backgroundMode = false
            }
        }
    }

    // Panel lateral de configuración
    Rectangle {
        id: controlDrawer
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 370
        color: Qt.rgba(visualizer.colorPalette.themeSurfaceColor.r, visualizer.colorPalette.themeSurfaceColor.g, visualizer.colorPalette.themeSurfaceColor.b, 0.96)
        border.color: Qt.rgba(visualizer.colorPalette.themeAccentColor.r, visualizer.colorPalette.themeAccentColor.g, visualizer.colorPalette.themeAccentColor.b, 0.35)
        border.width: 1
        visible: !root.backgroundMode
        z: 90

        Timer {
            id: autoSaveTimer
            interval: 60
            repeat: false
            onTriggered: saveConfiguration()
        }

        Timer {
            id: daemonStatusPoll
            interval: 1000
            repeat: true
            running: true
            onTriggered: cfgMgr.isDesktopDaemonRunning
        }

        // Barra Flotante Inferior de Acciones Rápidas (siempre fija y visible al desplazarse)
        Rectangle {
            id: floatingActionDock
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 108
            z: 100
            color: Qt.rgba(visualizer.colorPalette.themeBackgroundColor.r, visualizer.colorPalette.themeBackgroundColor.g, visualizer.colorPalette.themeBackgroundColor.b, 0.98)
            border.color: Qt.rgba(visualizer.colorPalette.themeAccentColor.r, visualizer.colorPalette.themeAccentColor.g, visualizer.colorPalette.themeAccentColor.b, 0.45)
            border.width: 1

            // Borde superior brillante
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 2
                color: cfgMgr.isDesktopDaemonRunning ? "#00ff88" : visualizer.colorPalette.themeAccentColor
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                // Estado del proceso en segundo plano
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: cfgMgr.isDesktopDaemonRunning ? "#00ff88" : "#ff3366"
                    }

                    Text {
                        text: cfgMgr.isDesktopDaemonRunning ? "Escritorio: ACTIVO (fondo Wayland)" : "Escritorio: DETENIDO"
                        color: cfgMgr.isDesktopDaemonRunning ? "#00ff88" : "#bbccdd"
                        font.pixelSize: 11
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Reiniciar"
                        implicitHeight: 22
                        font.pixelSize: 10
                        visible: cfgMgr.isDesktopDaemonRunning
                        ToolTip.visible: hovered
                        ToolTip.text: "Reiniciar visualizador en el escritorio"
                        onClicked: {
                            saveConfiguration();
                            cfgMgr.restartDesktopDaemon();
                        }
                    }
                }

                // Botones principales flotantes: Aplicar Cambios / Iniciar o Detener
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: "Aplicar Cambios"
                        font.bold: true
                        font.pixelSize: 12
                        palette.button: "#00e5ff"
                        palette.buttonText: "#050510"
                        ToolTip.visible: hovered
                        ToolTip.text: "Guarda los cambios y actualiza el fondo de pantalla inmediatamente"
                        onClicked: {
                            saveConfiguration();
                            if (cfgMgr.isDesktopDaemonRunning) {
                                cfgMgr.restartDesktopDaemon();
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: cfgMgr.isDesktopDaemonRunning ? "Detener" : "Iniciar"
                        font.bold: true
                        font.pixelSize: 12
                        palette.button: cfgMgr.isDesktopDaemonRunning ? "#ff3366" : "#1a2e3a"
                        palette.buttonText: cfgMgr.isDesktopDaemonRunning ? "#ffffff" : "#00ff88"
                        ToolTip.visible: hovered
                        ToolTip.text: cfgMgr.isDesktopDaemonRunning
                                      ? "Detiene el visualizador del fondo de escritorio"
                                      : "Inicia el visualizador en el fondo del escritorio"
                        onClicked: {
                            saveConfiguration();
                            if (cfgMgr.isDesktopDaemonRunning) {
                                cfgMgr.stopDesktopDaemon();
                            } else {
                                cfgMgr.startDesktopDaemon();
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        Layout.fillWidth: true
                        implicitHeight: 20
                        text: "Guardar Ajustes"
                        font.pixelSize: 10
                        onClicked: saveConfiguration()
                    }
                    Button {
                        Layout.fillWidth: true
                        implicitHeight: 20
                        text: "Cargar Ajustes"
                        font.pixelSize: 10
                        onClicked: loadConfiguration()
                    }
                }
            }
        }

        ScrollView {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: floatingActionDock.top
            anchors.margins: 16
            anchors.topMargin: 56
            anchors.bottomMargin: 8
            contentWidth: panelCol.width
            clip: true

            Column {
                id: panelCol
                width: 338
                spacing: 14

                // Título
                Text {
                    text: "AudioViewer - Configuración"
                    color: "#00e5ff"
                    font.pixelSize: 16
                    font.bold: true
                }
                Text {
                    text: "Los botones de Aplicar / Detener flotan al final del panel"
                    color: "#667788"
                    font.pixelSize: 10
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                // --- SECCIÓN 0: PERFILES Y PRESETS ---
                Rectangle {
                    width: parent.width
                    implicitHeight: presetCol.implicitHeight + 16
                    radius: 8
                    color: Qt.rgba(0.04, 0.14, 0.22, 0.55)
                    border.color: Qt.rgba(0.0, 0.9, 1.0, 0.35)
                    border.width: 1

                    ColumnLayout {
                        id: presetCol
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Perfiles & Presets"
                                color: "#00e5ff"
                                font.bold: true
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: cfgMgr.isFactoryPreset(presetSelectorCombo.currentText) ? "#ffaa00" : "#00ff88"
                            }
                            Text {
                                text: cfgMgr.isFactoryPreset(presetSelectorCombo.currentText) ? "Fábrica" : "Usuario"
                                color: "#8899aa"
                                font.pixelSize: 10
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            ComboBox {
                                id: presetSelectorCombo
                                Layout.fillWidth: true
                                model: cfgMgr.availablePresets
                                font.pixelSize: 11
                                onActivated: {
                                    loadNamedPreset(currentText);
                                }
                            }

                            Button {
                                text: "+"
                                font.bold: true
                                font.pixelSize: 13
                                implicitHeight: 32
                                implicitWidth: 32
                                ToolTip.visible: hovered
                                ToolTip.text: "Guardar configuración actual como nuevo perfil"
                                onClicked: savePresetDialog.open()
                            }

                            Button {
                                text: "-"
                                font.bold: true
                                font.pixelSize: 13
                                implicitHeight: 32
                                implicitWidth: 32
                                enabled: !cfgMgr.isFactoryPreset(presetSelectorCombo.currentText)
                                ToolTip.visible: hovered
                                ToolTip.text: "Eliminar perfil personalizado"
                                onClicked: {
                                    cfgMgr.deletePreset(presetSelectorCombo.currentText);
                                }
                            }
                        }
                    }
                }

                // --- SECCIÓN 1: PLANO CARTESIANO Y NIVEL DEL EJE ---
                Text { text: "1. Plano Cartesiano y Eje de Nacimiento"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                RowLayout {
                    width: parent.width
                    Text { text: "Eje de Origen:"; color: "#bbccdd"; font.pixelSize: 11; Layout.fillWidth: true }
                    ComboBox {
                        id: axisCombo
                        model: ["Eje X (Horizontal)", "Eje Y (Vertical)"]
                        currentIndex: 0
                        implicitWidth: 170
                        onCurrentIndexChanged: {
                            visualizer.originAxis = (currentIndex === 0 ? CartesianVisualizer.AxisX : CartesianVisualizer.AxisY);
                            autoSaveTimer.restart();
                        }
                    }
                }

                // Modificar el nivel del eje de nacimiento
                Column {
                    width: parent.width
                    spacing: 2
                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Nivel del Eje de Nacimiento: " + (axisOffsetSlider.value >= 0 ? "+" : "") + axisOffsetSlider.value.toFixed(0) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        Button {
                            text: "Centrar (0)"
                            font.pixelSize: 10
                            implicitHeight: 22
                            onClicked: axisOffsetSlider.value = 0.0
                        }
                    }
                    Slider {
                        id: axisOffsetSlider
                        width: parent.width
                        from: -400; to: 400; value: 0.0
                    }
                    Text {
                        text: axisCombo.currentIndex === 0 ? "Mueve el eje horizontal hacia arriba (+) o abajo (-)"
                                                            : "Mueve el eje vertical a la derecha (+) o izquierda (-)"
                        color: "#667788"
                        font.pixelSize: 10
                    }
                }

                RowLayout {
                    width: parent.width
                    Text { text: "Distribución:"; color: "#bbccdd"; font.pixelSize: 11; Layout.fillWidth: true }
                    ComboBox {
                        id: freqCombo
                        model: ["Centro hacia Afuera", "Lineal (Extremo a Extremo)"]
                        currentIndex: 0
                        implicitWidth: 170
                        onCurrentIndexChanged: {
                            visualizer.frequencyLayout = (currentIndex === 0 ? CartesianVisualizer.CenterOut : CartesianVisualizer.Linear);
                            autoSaveTimer.restart();
                        }
                    }
                }

                RowLayout {
                    width: parent.width
                    CheckBox {
                        id: mirrorCheck
                        text: "Efecto Espejo (Ambos semiejes)"
                        checked: true
                        palette.text: "#00e5ff"
                        onCheckedChanged: autoSaveTimer.restart()
                    }
                }

                RowLayout {
                    width: parent.width
                    CheckBox {
                        id: invertCheck
                        text: "Invertir Dirección Inicial"
                        checked: false
                        palette.text: "#bbccdd"
                        onCheckedChanged: autoSaveTimer.restart()
                    }
                }

                RowLayout {
                    width: parent.width
                    CheckBox {
                        id: showAxesCheck
                        text: "Mostrar Guías de Ejes (0,0)"
                        checked: false
                        palette.text: "#bbccdd"
                        onCheckedChanged: autoSaveTimer.restart()
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                // --- SECCIÓN 2: UMBRALES Y ALCANCE DE LAS BARRAS ---
                Text { text: "2. Umbrales y Alcance de las Barras"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                Column {
                    width: parent.width
                    spacing: 4

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Alcance Máximo de Pantalla: " + Math.round(reachPercentSlider.value * 100) + "%" +
                                  (reachPercentSlider.value >= 0.99 ? " (Toda la pantalla)" : "")
                            color: reachPercentSlider.value >= 0.99 ? "#00ff88" : "#00e5ff"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        Button {
                            text: "100%"
                            font.pixelSize: 10
                            implicitHeight: 20
                            onClicked: {
                                reachPercentSlider.value = 1.0;
                                autoSaveTimer.restart();
                            }
                        }
                    }

                    Slider {
                        id: reachPercentSlider
                        width: parent.width
                        from: 0.10; to: 1.0; stepSize: 0.02; value: 1.0
                        onMoved: autoSaveTimer.restart()
                        onValueChanged: {
                            visualizer.reachPercent = value;
                        }
                    }

                    Text {
                        text: reachPercentSlider.value >= 0.99
                              ? "Calculado automáticamente: al 100% las barras abarcan toda la pantalla independientemente de la resolución del monitor (~" + Math.round(visualizer.effectiveMaxReach) + " px)"
                              : ("Las barras alcanzarán como tope el " + Math.round(reachPercentSlider.value * 100) + "% del alto disponible en tu pantalla (~" + Math.round(visualizer.effectiveMaxReach) + " px)")
                        color: "#667788"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                // Compatibilidad con identificadores anteriores
                QtObject {
                    id: autoMaxThreshCheck
                    property bool checked: reachPercentSlider.value >= 0.99
                }
                QtObject {
                    id: maxThreshSlider
                    property real value: reachPercentSlider.value
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text {
                        text: "Filtro de Ruido Mínimo (Noise Gate): " + (minThreshSlider.value * 100).toFixed(1) + "%"
                        color: "#8899aa"
                        font.pixelSize: 11
                    }
                    Slider {
                        id: minThreshSlider
                        width: parent.width
                        from: 0.0; to: 0.40; value: 0.02
                        onMoved: autoSaveTimer.restart()
                    }
                    Text {
                        text: "Filtra ruidos estáticos de fondo menores a este valor"
                        color: "#667788"
                        font.pixelSize: 10
                    }
                }

                RowLayout {
                    width: parent.width
                    CheckBox {
                        id: showThreshLineCheck
                        text: "Mostrar Guía Visual del Alcance en el Plano"
                        checked: false
                        palette.text: "#00e5ff"
                        onCheckedChanged: autoSaveTimer.restart()
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                // --- SECCIÓN 3: ESTILO Y GEOMETRÍA DE BARRAS ---
                Text { text: "3. Estilo y Geometría de Barras"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                RowLayout {
                    width: parent.width
                    Text { text: "Modo Visual:"; color: "#bbccdd"; font.pixelSize: 11; Layout.fillWidth: true }
                    ComboBox {
                        id: styleCombo
                        model: [
                            "Barras Sólidas (Clásico)",
                            "Puntas / Agujas Dentadas",
                            "Picos Flotantes Desacoplados",
                            "Ondas Armónicas / Osciloscopio",
                            "Bloques LED (Retro Hi-Fi)",
                            "Matriz de Puntos (Cuentas)",
                            "Onda Zig-Zag Neón",
                            "Cuerdas / Fibras Luminosas",
                            "Cuadrícula VU (Studio Rack)"
                        ]
                        currentIndex: 0
                        implicitWidth: 200
                        onCurrentIndexChanged: {
                            visualizer.visualStyle = currentIndex;
                            autoSaveTimer.restart();
                        }
                    }
                }

                // Ajustes de Bloques LED (visibles en modo Bloques LED)
                Column {
                    width: parent.width
                    spacing: 4
                    visible: styleCombo.currentIndex === 4

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Altura Bloque LED: " + segHeightSlider.value.toFixed(0) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: segHeightSlider
                        width: parent.width
                        from: 2.0; to: 16.0; value: 6.0
                        onMoved: autoSaveTimer.restart()
                    }

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Separación Bloques: " + segGapSlider.value.toFixed(0) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: segGapSlider
                        width: parent.width
                        from: 1.0; to: 8.0; value: 2.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                // Ajustes de Matriz de Puntos (visibles en modo Puntos)
                Column {
                    width: parent.width
                    spacing: 4
                    visible: styleCombo.currentIndex === 5

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Diámetro de Puntos: " + dotSizeSlider.value.toFixed(1) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: dotSizeSlider
                        width: parent.width
                        from: 2.0; to: 24.0; value: 6.0; stepSize: 0.5
                        onMoved: autoSaveTimer.restart()
                    }

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Separación entre Puntos: " + dotGapSlider.value.toFixed(1) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: dotGapSlider
                        width: parent.width
                        from: 1.0; to: 15.0; value: 3.0; stepSize: 0.5
                        onMoved: autoSaveTimer.restart()
                    }
                }

                // Ajustes de Cuerdas / Fibras Luminosas (visibles en modo Cuerdas)
                Column {
                    width: parent.width
                    spacing: 4
                    visible: styleCombo.currentIndex === 7

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Intensidad Resonante: " + stringGlowSlider.value.toFixed(1) + "x"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: stringGlowSlider
                        width: parent.width
                        from: 0.5; to: 3.0; stepSize: 0.1; value: 1.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                // Ajustes de Cuadrícula VU (visibles en modo Cuadrícula)
                Column {
                    width: parent.width
                    spacing: 4
                    visible: styleCombo.currentIndex === 8

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Filas de la Matriz: " + Math.round(gridRowsSlider.value)
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: gridRowsSlider
                        width: parent.width
                        from: 6; to: 32; stepSize: 1; value: 14
                        onMoved: autoSaveTimer.restart()
                    }
                }

                // Opción para habilitar picos flotantes superiores (en barras sólidas, bloques y puntos)
                RowLayout {
                    width: parent.width
                    visible: styleCombo.currentIndex === 0 || styleCombo.currentIndex === 4 || styleCombo.currentIndex === 5
                    CheckBox {
                        id: showPeakCapsCheck
                        text: "Tapas / Picos Flotantes en la Cúspide"
                        checked: false
                        palette.text: "#00e5ff"
                        onCheckedChanged: autoSaveTimer.restart()
                    }
                }

                // Ajustes de Picos Flotantes (visibles en modo Picos Flotantes o con checkbox activo)
                Column {
                    width: parent.width
                    spacing: 4
                    visible: styleCombo.currentIndex === 2 || (showPeakCapsCheck.checked && (styleCombo.currentIndex === 0 || styleCombo.currentIndex === 4 || styleCombo.currentIndex === 5))

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Separación Pico Flotante: " + peakCapGapSlider.value.toFixed(1) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: peakCapGapSlider
                        width: parent.width
                        from: 1.0; to: 25.0; value: 4.0; stepSize: 0.5
                        onMoved: autoSaveTimer.restart()
                    }

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Grosor de Tapa / Pico: " + peakCapHeightSlider.value.toFixed(1) + " px"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: peakCapHeightSlider
                        width: parent.width
                        from: 1.0; to: 15.0; value: 3.0; stepSize: 0.5
                        onMoved: autoSaveTimer.restart()
                    }
                }

                // Ajustes de Ondas Armónicas / Osciloscopio (visibles en modo Ondas)
                Column {
                    width: parent.width
                    spacing: 4
                    visible: styleCombo.currentIndex === 3

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Capas Armónicas: " + waveLayersSlider.value.toFixed(0)
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: waveLayersSlider
                        width: parent.width
                        from: 2; to: 8; stepSize: 1; value: 5
                        onMoved: autoSaveTimer.restart()
                    }

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Velocidad de Flujo: " + waveSpeedSlider.value.toFixed(1) + "x"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: waveSpeedSlider
                        width: parent.width
                        from: 0.1; to: 4.0; stepSize: 0.1; value: 1.0
                        onMoved: autoSaveTimer.restart()
                    }

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Dispersión de Frecuencia: " + waveSpreadSlider.value.toFixed(1) + "x"
                            color: "#00e5ff"
                            font.pixelSize: 10
                            Layout.fillWidth: true
                        }
                    }
                    Slider {
                        id: waveSpreadSlider
                        width: parent.width
                        from: 0.2; to: 3.0; stepSize: 0.1; value: 1.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text {
                        text: styleCombo.currentIndex === 3
                              ? ("Grosor de Línea / Onda: " + barWidthSlider.value.toFixed(1) + " px")
                              : styleCombo.currentIndex === 6
                                 ? ("Grosor de Línea Zig-Zag: " + barWidthSlider.value.toFixed(1) + " px")
                                 : styleCombo.currentIndex === 7
                                    ? ("Grosor de Cuerdas: " + barWidthSlider.value.toFixed(1) + " px")
                                    : styleCombo.currentIndex === 1
                                       ? ("Ancho de Base Aguja: " + barWidthSlider.value.toFixed(1) + " px")
                                       : styleCombo.currentIndex === 5
                                          ? ("Diámetro Máximo Punto: " + barWidthSlider.value.toFixed(1) + " px")
                                          : ("Grosor / Ancho: " + barWidthSlider.value.toFixed(1) + " px")
                        color: "#8899aa"
                        font.pixelSize: 11
                    }
                    Slider {
                        id: barWidthSlider
                        width: parent.width
                        from: 2; to: 30; value: 9.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    visible: styleCombo.currentIndex !== 3 && styleCombo.currentIndex !== 6
                    Text { text: "Separación: " + barSpacingSlider.value.toFixed(1) + " px"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: barSpacingSlider
                        width: parent.width
                        from: 0; to: 20; value: 3.5
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2

                    Text {
                        text: "Alcance Efectivo: " + Math.round(reachPercentSlider.value * 100) + "% (~" + Math.round(visualizer.effectiveMaxReach) + " px según resolución)"
                        color: "#00ff88"
                        font.pixelSize: 11
                        font.bold: true
                    }
                    Text {
                        text: "El tamaño se autoescala fluidamente en base al porcentaje configurado en la Sección 2."
                        color: "#667788"
                        font.pixelSize: 10
                    }
                }

                // Compatibilidad con identificador maxLenSlider
                QtObject {
                    id: maxLenSlider
                    property real value: visualizer.effectiveMaxReach
                }

                CheckBox {
                    id: autoFillCheck
                    text: "Llenar pantalla completa con barras"
                    checked: false
                    visible: styleCombo.currentIndex !== 3 && styleCombo.currentIndex !== 6
                    palette.text: "#00e5ff"
                    onCheckedChanged: autoSaveTimer.restart()
                }

                Column {
                    width: parent.width
                    spacing: 2
                    visible: styleCombo.currentIndex !== 3 && styleCombo.currentIndex !== 6
                    opacity: autoFillCheck.checked ? 0.35 : 1.0

                    Text {
                        text: autoFillCheck.checked ? "Cantidad de Barras: Automático (calculado por resolución)"
                                                    : ("Cantidad de Barras: " + barCountSlider.value.toFixed(0))
                        color: autoFillCheck.checked ? "#00ff88" : "#8899aa"
                        font.pixelSize: 11
                        font.bold: autoFillCheck.checked
                    }
                    Slider {
                        id: barCountSlider
                        width: parent.width
                        enabled: !autoFillCheck.checked
                        from: 16; to: 128; stepSize: 4; value: 64
                        onMoved: {
                            audioSource.barCount = Math.round(value)
                            autoSaveTimer.restart()
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                // --- SECCIÓN 4: PERSPECTIVA 3D ---
                Text { text: "4. Perspectiva 3D / Inclinación"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Inclinación Pitch (X): " + pitchSlider.value.toFixed(1) + "°"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: pitchSlider
                        width: parent.width
                        from: -65; to: 65; value: 25.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Rotación Yaw (Y): " + yawSlider.value.toFixed(1) + "°"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: yawSlider
                        width: parent.width
                        from: -45; to: 45; value: 0.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Profundidad de Fuga: " + depthSlider.value.toFixed(0) + "px"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: depthSlider
                        width: parent.width
                        from: 200; to: 1600; value: 800.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                // --- SECCIÓN 5: SOMBRA RESIDUAL Y TRANSPARENCIA ---
                Text { text: "5. Sombra Residual & Transparencia"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                CheckBox {
                    id: residualCheck
                    text: "Activar Sombra Residual (Estela)"
                    checked: true
                    palette.text: "#00e5ff"
                    onCheckedChanged: autoSaveTimer.restart()
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Decaimiento de Sombra: " + decaySlider.value.toFixed(2); color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: decaySlider
                        width: parent.width
                        from: 0.60; to: 0.98; value: 0.91
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Opacidad de Sombra: " + (resOpacitySlider.value * 100).toFixed(0) + "%"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: resOpacitySlider
                        width: parent.width
                        from: 0.05; to: 1.0; value: 0.45
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Desplazamiento Residual: " + resOffsetSlider.value.toFixed(1) + " px"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: resOffsetSlider
                        width: parent.width
                        from: 0; to: 20; value: 5.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Opacidad Global: " + (globalAlphaSlider.value * 100).toFixed(0) + "%"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: globalAlphaSlider
                        width: parent.width
                        from: 0.1; to: 1.0; value: 1.0
                        onMoved: autoSaveTimer.restart()
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

                // --- SECCIÓN 6: PALETA DE COLORES (1 A 64 COLORES) ---
                Text { text: "6. Paleta de Colores (1 a 64)"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                // Botón para aplicar tema de Hyprland con advertencia
                Column {
                    width: parent.width
                    spacing: 4

                    Button {
                        id: hyprThemeBtn
                        width: parent.width
                        text: "Aplicar Colores del Tema de Hyprland"
                        font.bold: true
                        font.pixelSize: 11
                        palette.button: "#00adb8"
                        palette.buttonText: "#050510"
                        onClicked: {
                            var ok = visualizer.colorPalette.applyHyprlandTheme();
                            if (ok) {
                                presetCombo.currentIndex = -1;
                                autoSaveTimer.restart();
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: hyprWarningRow.implicitHeight + 10
                        radius: 4
                        color: Qt.rgba(0.0, 0.68, 0.72, 0.12)
                        border.color: Qt.rgba(0.0, 0.68, 0.72, 0.35)
                        border.width: 1

                        RowLayout {
                            id: hyprWarningRow
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 6

                            Text {
                                text: "!"
                                color: "#00adb8"
                                font.bold: true
                                font.pixelSize: 12
                            }

                            Text {
                                text: "Nota: Esta opción aplica únicamente en entornos Hyprland (extrae bordes de hyprctl y paleta del tema del sistema)."
                                color: "#bbddee"
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                RowLayout {
                    width: parent.width
                    Text { text: "Preset:"; color: "#bbccdd"; font.pixelSize: 11; Layout.fillWidth: true }
                    ComboBox {
                        id: presetCombo
                        model: visualizer.colorPalette.availablePresets()
                        currentIndex: 0
                        implicitWidth: 170
                        onActivated: (index) => {
                            visualizer.colorPalette.loadPreset(model[index]);
                            autoSaveTimer.restart();
                        }
                    }
                }

                RowLayout {
                    width: parent.width
                    Text { text: "Modo Gradiente:"; color: "#bbccdd"; font.pixelSize: 11; Layout.fillWidth: true }
                    ComboBox {
                        id: colorModeCombo
                        model: [
                            "Solo por Altura (Base a Punta)",
                            "Solo por Barras (Graves a Agudos)",
                            "Altura + Barras (Ambos combinados)"
                        ]
                        currentIndex: 0
                        implicitWidth: 185
                        onCurrentIndexChanged: {
                            visualizer.colorMode = currentIndex;
                            autoSaveTimer.restart();
                        }
                    }
                }

                // Vista previa continua del degradado
                Column {
                    width: parent.width
                    spacing: 4

                    Text {
                        text: "Degradado continuo:"
                        color: "#8899aa"
                        font.pixelSize: 10
                    }

                    Rectangle {
                        width: parent.width
                        height: 20
                        radius: 4
                        border.color: Qt.rgba(1, 1, 1, 0.2)
                        border.width: 1
                        clip: true

                        Row {
                            anchors.fill: parent
                            Repeater {
                                model: visualizer.colorPalette.colors
                                Rectangle {
                                    width: parent.width / Math.max(1, visualizer.colorPalette.count)
                                    height: parent.height
                                    color: modelData
                                }
                            }
                        }
                    }
                }

                // Visualización interactiva de colores (chips/swatches)
                Column {
                    width: parent.width
                    spacing: 6

                    RowLayout {
                        width: parent.width
                        Text {
                            text: "Colores Activos (" + visualizer.colorPalette.count + " / 64):"
                            color: "#bbccdd"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "+ Añadir"
                            font.pixelSize: 10
                            implicitHeight: 22
                            enabled: visualizer.colorPalette.count < 64
                            onClicked: {
                                var lastColor = visualizer.colorPalette.colors[visualizer.colorPalette.count - 1] || "#00e5ff";
                                colorEditPopup.targetIndex = -1;
                                colorEditPopup.currentColor = lastColor;
                                colorEditPopup.open();
                            }
                        }

                        Button {
                            text: "Gotero"
                            font.pixelSize: 10
                            implicitHeight: 22
                            enabled: visualizer.colorPalette.isEyedropperAvailable && visualizer.colorPalette.count < 64
                            palette.button: "#152535"
                            palette.buttonText: "#00e5ff"
                            ToolTip.visible: hovered
                            ToolTip.text: "Capturar color de cualquier parte de la pantalla (Hyprpicker)"
                            onClicked: {
                                visualizer.colorPalette.pickColorWithEyedropper(-1);
                            }
                        }
                    }

                    Text {
                        text: "Haz clic en cualquier color para editar (x para eliminar)"
                        color: "#667788"
                        font.pixelSize: 10
                    }

                    // Cuadrícula interactiva de colores
                    Flow {
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: visualizer.colorPalette.colors

                            Rectangle {
                                id: swatchItem
                                width: 34
                                height: 34
                                radius: 6
                                color: modelData
                                border.color: swatchMouse.containsMouse ? "#ffffff" : Qt.rgba(1, 1, 1, 0.25)
                                border.width: swatchMouse.containsMouse ? 2 : 1

                                ToolTip.visible: swatchMouse.containsMouse
                                ToolTip.delay: 300
                                ToolTip.text: (modelData ? modelData.toString().toUpperCase() : "") + "\nClic: Editar / Gotero"

                                MouseArea {
                                    id: swatchMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        colorEditPopup.targetIndex = index;
                                        colorEditPopup.currentColor = modelData;
                                        colorEditPopup.open();
                                    }
                                }

                                // Botón pequeño para borrar color si hay más de 1
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.margins: -4
                                    width: 14
                                    height: 14
                                    radius: 7
                                    color: "#ff3366"
                                    visible: swatchMouse.containsMouse && visualizer.colorPalette.count > 1
                                    z: 10

                                    Text {
                                        anchors.centerIn: parent
                                        text: "x"
                                        color: "#ffffff"
                                        font.pixelSize: 9
                                        font.bold: true
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            visualizer.colorPalette.removeColorAt(index);
                                            autoSaveTimer.restart();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }
                Text { text: "7. Dinámica de Audio & Ahorro de Energía"; color: "#ffffff"; font.bold: true; font.pixelSize: 12 }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Sensibilidad: " + sensSlider.value.toFixed(2) + "x"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: sensSlider
                        width: parent.width
                        from: 0.2; to: 3.5; value: 1.2
                        onMoved: {
                            audioSource.sensitivity = value;
                            autoSaveTimer.restart();
                        }
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Gravedad / Inercia de Caída: " + gravSlider.value.toFixed(2); color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: gravSlider
                        width: parent.width
                        from: 0.70; to: 0.98; value: 0.92
                        onMoved: {
                            audioSource.gravity = value;
                            autoSaveTimer.restart();
                        }
                    }
                    Text {
                        text: "Controla la suavidad y lentitud con la que caen las barras (valores mayores = caída más lenta y sedosa)."
                        color: "#667788"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Velocidad de Reacción (Ataque): " + attackSlider.value.toFixed(2); color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: attackSlider
                        width: parent.width
                        from: 0.15; to: 0.95; value: 0.45; stepSize: 0.05
                        onMoved: {
                            audioSource.attackRate = value;
                            autoSaveTimer.restart();
                        }
                    }
                    Text {
                        text: "Evita que las barras salten descontroladamente. Un ataque de 0.40 - 0.50 mantiene respuesta rítmica sin romper los efectos."
                        color: "#667788"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                Column {
                    width: parent.width
                    spacing: 2
                    Text { text: "Suavizado Espacial de Ondas / Bandas: " + (spatialSmoothSlider.value * 100).toFixed(0) + "%"; color: "#8899aa"; font.pixelSize: 11 }
                    Slider {
                        id: spatialSmoothSlider
                        width: parent.width
                        from: 0.0; to: 0.60; value: 0.32; stepSize: 0.02
                        onMoved: {
                            audioSource.spatialSmoothing = value;
                            autoSaveTimer.restart();
                        }
                    }
                    Text {
                        text: "Armoniza barras adyacentes para que las ondas armónicas, zig-zag y agujas se mantengan continuas y no se vean caóticas."
                        color: "#667788"
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }

                RowLayout {
                    width: parent.width
                    CheckBox {
                        id: autoSleepCheck
                        text: "Modo Reposo en Silencio (0% CPU/GPU)"
                        checked: audioSource.autoSleepEnabled
                        palette.text: "#00e5ff"
                        onCheckedChanged: {
                            audioSource.autoSleepEnabled = checked;
                            autoSaveTimer.restart();
                        }
                    }
                }

                Text {
                    text: "Suspende el renderizado a 0 FPS tras 2s sin sonido. Reactivación inmediata al sonar música."
                    color: "#667788"
                    font.pixelSize: 10
                    wrapMode: Text.WordWrap
                    width: parent.width
                }

                Item { width: 1; height: 25 }
            }
        }
    }

    // Funciones de guardado y carga de configuración
    function getCurrentConfigJson() {
        return {
            "presetName": (typeof presetSelectorCombo !== "undefined" && presetSelectorCombo.currentText) ? presetSelectorCombo.currentText : "",
            "originAxis": axisCombo.currentIndex,
            "axisOffset": axisOffsetSlider.value,
            "frequencyLayout": freqCombo.currentIndex,
            "mirrorEnabled": mirrorCheck.checked,
            "invertDirection": invertCheck.checked,
            "showAxes": showAxesCheck.checked,
            "autoFillScreen": autoFillCheck.checked,
            "autoMaxThreshold": reachPercentSlider.value >= 0.99,
            "reachPercent": reachPercentSlider.value,
            "minThreshold": minThreshSlider.value,
            "maxThreshold": reachPercentSlider.value,
            "showThresholdLine": showThreshLineCheck.checked,
            "barWidth": barWidthSlider.value,
            "barSpacing": barSpacingSlider.value,
            "maxBarLength": maxLenSlider.value,
            "barCount": audioSource.barCount,
            "tiltPitch": pitchSlider.value,
            "tiltYaw": yawSlider.value,
            "perspectiveDepth": depthSlider.value,
            "residualEnabled": residualCheck.checked,
            "residualDecay": decaySlider.value,
            "residualOpacity": resOpacitySlider.value,
            "residualOffset": resOffsetSlider.value,
            "transparency": globalAlphaSlider.value,
            "colorPreset": presetCombo.currentIndex >= 0 ? presetCombo.currentText : "Personalizado",
            "customColors": visualizer.colorPalette.colorsAsHex(),
            "colorMode": colorModeCombo.currentIndex,
            "visualStyle": styleCombo.currentIndex,
            "segmentHeight": segHeightSlider.value,
            "segmentGap": segGapSlider.value,
            "showPeakCaps": showPeakCapsCheck.checked,
            "peakCapGap": peakCapGapSlider.value,
            "peakCapHeight": peakCapHeightSlider.value,
            "waveLayers": Math.round(waveLayersSlider.value),
            "waveSpeed": waveSpeedSlider.value,
            "waveSpread": waveSpreadSlider.value,
            "dotSize": dotSizeSlider.value,
            "dotGap": dotGapSlider.value,
            "gridRows": Math.round(gridRowsSlider.value),
            "stringGlow": stringGlowSlider.value,
            "sensitivity": sensSlider.value,
            "gravity": gravSlider.value,
            "attackRate": attackSlider.value,
            "spatialSmoothing": spatialSmoothSlider.value,
            "autoSleepEnabled": autoSleepCheck.checked
        };
    }

    function saveConfiguration() {
        var cfg = getCurrentConfigJson();
        cfgMgr.saveConfig(cfg);
        if (typeof externalConfigWatcher !== "undefined") {
            externalConfigWatcher.lastModMs = cfgMgr.configLastModifiedMs();
        }
    }

    function applyConfigObject(cfg) {
        if (!cfg || Object.keys(cfg).length === 0) return;

        if (cfg.originAxis !== undefined) axisCombo.currentIndex = cfg.originAxis;
        if (cfg.axisOffset !== undefined) axisOffsetSlider.value = cfg.axisOffset;
        if (cfg.frequencyLayout !== undefined) freqCombo.currentIndex = cfg.frequencyLayout;
        if (cfg.mirrorEnabled !== undefined) mirrorCheck.checked = cfg.mirrorEnabled;
        if (cfg.invertDirection !== undefined) invertCheck.checked = cfg.invertDirection;
        if (cfg.showAxes !== undefined) showAxesCheck.checked = cfg.showAxes;
        if (cfg.autoFillScreen !== undefined) autoFillCheck.checked = cfg.autoFillScreen;
        if (cfg.reachPercent !== undefined) {
            reachPercentSlider.value = cfg.reachPercent;
        } else if (cfg.autoMaxThreshold === true) {
            reachPercentSlider.value = 1.0;
        } else if (cfg.maxThreshold !== undefined && cfg.maxThreshold <= 1.0) {
            reachPercentSlider.value = cfg.maxThreshold;
        }
        if (cfg.minThreshold !== undefined) minThreshSlider.value = cfg.minThreshold;
        if (cfg.showThresholdLine !== undefined) showThreshLineCheck.checked = cfg.showThresholdLine;
        if (cfg.barWidth !== undefined) barWidthSlider.value = cfg.barWidth;
        if (cfg.barSpacing !== undefined) barSpacingSlider.value = cfg.barSpacing;
        if (cfg.maxBarLength !== undefined) maxLenSlider.value = cfg.maxBarLength;
        if (cfg.barCount !== undefined) {
            barCountSlider.value = cfg.barCount;
            audioSource.barCount = cfg.barCount;
        }
        if (cfg.tiltPitch !== undefined) pitchSlider.value = cfg.tiltPitch;
        if (cfg.tiltYaw !== undefined) yawSlider.value = cfg.tiltYaw;
        if (cfg.perspectiveDepth !== undefined) depthSlider.value = cfg.perspectiveDepth;
        if (cfg.residualEnabled !== undefined) residualCheck.checked = cfg.residualEnabled;
        if (cfg.residualDecay !== undefined) decaySlider.value = cfg.residualDecay;
        if (cfg.residualOpacity !== undefined) resOpacitySlider.value = cfg.residualOpacity;
        if (cfg.residualOffset !== undefined) resOffsetSlider.value = cfg.residualOffset;
        if (cfg.transparency !== undefined) globalAlphaSlider.value = cfg.transparency;
        if (cfg.customColors !== undefined && cfg.customColors.length > 0) {
            visualizer.colorPalette.setColorsFromHex(cfg.customColors);
            if (cfg.colorPreset !== undefined) {
                var found = false;
                for (var j = 0; j < presetCombo.model.length; ++j) {
                    if (presetCombo.model[j] === cfg.colorPreset) {
                        presetCombo.currentIndex = j;
                        found = true;
                        break;
                    }
                }
                if (!found) presetCombo.currentIndex = -1;
            }
        } else if (cfg.colorPreset !== undefined) {
            for (var i = 0; i < presetCombo.model.length; ++i) {
                if (presetCombo.model[i] === cfg.colorPreset) {
                    presetCombo.currentIndex = i;
                    visualizer.colorPalette.loadPreset(cfg.colorPreset);
                    break;
                }
            }
        }
        if (cfg.colorMode !== undefined) colorModeCombo.currentIndex = cfg.colorMode;
        if (cfg.sensitivity !== undefined) {
            sensSlider.value = cfg.sensitivity;
            audioSource.sensitivity = cfg.sensitivity;
        }
        if (cfg.gravity !== undefined) {
            gravSlider.value = cfg.gravity;
            audioSource.gravity = cfg.gravity;
        }
        if (cfg.attackRate !== undefined) {
            attackSlider.value = cfg.attackRate;
            audioSource.attackRate = cfg.attackRate;
        }
        if (cfg.spatialSmoothing !== undefined) {
            spatialSmoothSlider.value = cfg.spatialSmoothing;
            audioSource.spatialSmoothing = cfg.spatialSmoothing;
        }
        if (cfg.autoSleepEnabled !== undefined) {
            autoSleepCheck.checked = cfg.autoSleepEnabled;
            audioSource.autoSleepEnabled = cfg.autoSleepEnabled;
        }
        if (cfg.visualStyle !== undefined) styleCombo.currentIndex = cfg.visualStyle;
        if (cfg.segmentHeight !== undefined) segHeightSlider.value = cfg.segmentHeight;
        if (cfg.segmentGap !== undefined) segGapSlider.value = cfg.segmentGap;
        if (cfg.showPeakCaps !== undefined) showPeakCapsCheck.checked = cfg.showPeakCaps;
        if (cfg.peakCapGap !== undefined) peakCapGapSlider.value = cfg.peakCapGap;
        if (cfg.peakCapHeight !== undefined) peakCapHeightSlider.value = cfg.peakCapHeight;
        if (cfg.waveLayers !== undefined) waveLayersSlider.value = cfg.waveLayers;
        if (cfg.waveSpeed !== undefined) waveSpeedSlider.value = cfg.waveSpeed;
        if (cfg.waveSpread !== undefined) waveSpreadSlider.value = cfg.waveSpread;
        if (cfg.dotSize !== undefined) dotSizeSlider.value = cfg.dotSize;
        if (cfg.dotGap !== undefined) dotGapSlider.value = cfg.dotGap;
        if (cfg.gridRows !== undefined) gridRowsSlider.value = cfg.gridRows;
        if (cfg.stringGlow !== undefined) stringGlowSlider.value = cfg.stringGlow;

        if (cfg.presetName !== undefined && typeof presetSelectorCombo !== "undefined") {
            var pIndex = presetSelectorCombo.find(cfg.presetName);
            if (pIndex >= 0) {
                presetSelectorCombo.currentIndex = pIndex;
            }
        }
    }

    function loadConfiguration() {
        var cfg = cfgMgr.loadConfig();
        applyConfigObject(cfg);
    }

    function loadNamedPreset(name) {
        if (!name || name === "") return;
        var cfg = cfgMgr.loadPreset(name);
        if (!cfg || Object.keys(cfg).length === 0) return;
        var activeStyle = styleCombo.currentIndex;
        applyConfigObject(cfg);
        // Separación de presets y estilos visuales: el tema del preset se aplica a la visualización activa
        styleCombo.currentIndex = activeStyle;
        autoSaveTimer.restart();
    }

    // Diálogo Nativo de Selección de Color (Qt Quick Dialogs)
    ColorDialog {
        id: nativeColorDialog
        title: "Selector de Color - AudioViewer"
        selectedColor: "#00adb8"
        property int targetIndex: -1

        onAccepted: {
            if (targetIndex >= 0 && targetIndex < visualizer.colorPalette.count) {
                visualizer.colorPalette.setColorAt(targetIndex, selectedColor);
            } else if (targetIndex === -1 && visualizer.colorPalette.count < 64) {
                visualizer.colorPalette.addColor(selectedColor);
            }
            presetCombo.currentIndex = -1;
            autoSaveTimer.restart();
        }
    }

    // Modal interactivo de personalización de color y cuenta gotas
    Popup {
        id: colorEditPopup
        property int targetIndex: -1
        property color currentColor: "#00adb8"

        anchors.centerIn: parent
        width: 320
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: visualizer.colorPalette.themeSurfaceColor
            radius: 12
            border.color: Qt.rgba(visualizer.colorPalette.themeAccentColor.r, visualizer.colorPalette.themeAccentColor.g, visualizer.colorPalette.themeAccentColor.b, 0.4)
            border.width: 1
        }

        contentItem: ColumnLayout {
            spacing: 12

            Text {
                text: colorEditPopup.targetIndex >= 0 
                      ? ("Personalizar Color #" + (colorEditPopup.targetIndex + 1))
                      : "Añadir Nuevo Color"
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 13
            }

            // Vista previa del color seleccionado y campo Hex
            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Rectangle {
                    width: 44
                    height: 44
                    radius: 8
                    color: colorEditPopup.currentColor
                    border.color: Qt.rgba(1, 1, 1, 0.3)
                    border.width: 1
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3

                    Text {
                        text: "Código Hex:"
                        color: visualizer.colorPalette.themeMutedColor
                        font.pixelSize: 10
                    }

                    TextField {
                        id: hexInputField
                        Layout.fillWidth: true
                        text: colorEditPopup.currentColor.toString().toUpperCase()
                        font.bold: true
                        font.pixelSize: 12
                        palette.base: visualizer.colorPalette.themeBackgroundColor
                        palette.text: visualizer.colorPalette.themeAccentColor
                        onTextEdited: {
                            var c = Qt.color(text);
                            if (c !== undefined && text.length >= 4) {
                                colorEditPopup.currentColor = c;
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1, 1, 1, 0.1) }

            // Botón Cuenta Gotas (Eyedropper de pantalla via Hyprpicker)
            Button {
                Layout.fillWidth: true
                text: "Cuenta Gotas (Pantalla)"
                font.bold: true
                font.pixelSize: 11
                palette.button: "#132838"
                palette.buttonText: "#00e5ff"
                enabled: visualizer.colorPalette.isEyedropperAvailable
                ToolTip.visible: hovered
                ToolTip.text: "Captura el color de cualquier píxel en cualquier ventana o monitor"
                onClicked: {
                    var idx = colorEditPopup.targetIndex;
                    colorEditPopup.close();
                    visualizer.colorPalette.pickColorWithEyedropper(idx);
                }
            }

            // Botón para abrir el selector de colores completo
            Button {
                Layout.fillWidth: true
                text: "Selector de Colores..."
                font.pixelSize: 11
                palette.button: "#222236"
                palette.buttonText: "#ffffff"
                onClicked: {
                    nativeColorDialog.targetIndex = colorEditPopup.targetIndex;
                    nativeColorDialog.selectedColor = colorEditPopup.currentColor;
                    colorEditPopup.close();
                    nativeColorDialog.open();
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "Aplicar"
                    palette.button: "#00adb8"
                    palette.buttonText: "#050510"
                    font.bold: true
                    onClicked: {
                        var col = Qt.color(hexInputField.text);
                        if (col !== undefined) {
                            if (colorEditPopup.targetIndex >= 0 && colorEditPopup.targetIndex < visualizer.colorPalette.count) {
                                visualizer.colorPalette.setColorAt(colorEditPopup.targetIndex, col);
                            } else if (colorEditPopup.targetIndex === -1 && visualizer.colorPalette.count < 64) {
                                visualizer.colorPalette.addColor(col);
                            }
                            presetCombo.currentIndex = -1;
                            autoSaveTimer.restart();
                        }
                        colorEditPopup.close();
                    }
                }

                Button {
                    text: "Eliminar"
                    visible: colorEditPopup.targetIndex >= 0 && visualizer.colorPalette.count > 1
                    palette.button: "#441a24"
                    palette.buttonText: "#ff4466"
                    onClicked: {
                        visualizer.colorPalette.removeColorAt(colorEditPopup.targetIndex);
                        presetCombo.currentIndex = -1;
                        autoSaveTimer.restart();
                        colorEditPopup.close();
                    }
                }

                Button {
                    text: "Cancelar"
                    palette.button: "#1a1a24"
                    palette.buttonText: "#bbccdd"
                    onClicked: colorEditPopup.close()
                }
            }
        }
    }

    Connections {
        target: visualizer.colorPalette
        function onEyedropperColorPicked(index, col) {
            presetCombo.currentIndex = -1;
            autoSaveTimer.restart();
        }
        function onColorsChanged() {
            autoSaveTimer.restart();
        }
    }

    // Diálogo emergente para guardar un nuevo preset / perfil
    Popup {
        id: savePresetDialog
        anchors.centerIn: parent
        width: 320
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: visualizer.colorPalette.themeSurfaceColor
            radius: 12
            border.color: visualizer.colorPalette.themeAccentColor
            border.width: 1
        }
        contentItem: ColumnLayout {
            spacing: 12

            Text {
                text: "Guardar Perfil / Preset"
                color: "#ffffff"
                font.bold: true
                font.pixelSize: 13
            }

            Text {
                text: "Escribe un nombre para guardar todos los ajustes actuales:"
                color: visualizer.colorPalette.themeMutedColor
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            TextField {
                id: newPresetNameInput
                Layout.fillWidth: true
                placeholderText: "Nombre (ej. Mi Neon Synth)"
                font.pixelSize: 12
                palette.base: visualizer.colorPalette.themeBackgroundColor
                palette.text: visualizer.colorPalette.themeAccentColor
                onAccepted: saveBtn.clicked()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    id: saveBtn
                    Layout.fillWidth: true
                    text: "Guardar"
                    palette.button: "#00adb8"
                    palette.buttonText: "#050510"
                    font.bold: true
                    onClicked: {
                        var name = newPresetNameInput.text.trim();
                        if (name.length > 0) {
                            cfgMgr.savePreset(name, getCurrentConfigJson());
                            newPresetNameInput.text = "";
                            savePresetDialog.close();
                            // Seleccionar el recién guardado
                            var idx = presetSelectorCombo.find(name);
                            if (idx >= 0) presetSelectorCombo.currentIndex = idx;
                        }
                    }
                }

                Button {
                    text: "Cancelar"
                    palette.button: "#1a1a24"
                    palette.buttonText: "#bbccdd"
                    onClicked: {
                        newPresetNameInput.text = "";
                        savePresetDialog.close();
                    }
                }
            }
        }
    }

    Connections {
        target: cfgMgr
        function onPresetsChanged() {
            var current = presetSelectorCombo.currentText;
            presetSelectorCombo.model = cfgMgr.availablePresets;
            var idx = presetSelectorCombo.find(current);
            if (idx >= 0) {
                presetSelectorCombo.currentIndex = idx;
            }
        }
    }

    Component.onCompleted: {
        if (cfgMgr.hasSavedConfig()) {
            loadConfiguration();
        }
    }
}
