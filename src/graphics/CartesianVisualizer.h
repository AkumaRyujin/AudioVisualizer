#pragma once

#include <QQuickPaintedItem>
#include <QColor>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <vector>
#include <QtQml/qqmlregistration.h>
#include "ColorPalette.h"
#include "../audio/AudioSpectrumSource.h"

namespace AudioViewer {

class CartesianVisualizer : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT

    // Ejes y configuración cartesiana
    Q_PROPERTY(OriginAxis originAxis READ originAxis WRITE setOriginAxis NOTIFY originAxisChanged)
    Q_PROPERTY(FrequencyLayout frequencyLayout READ frequencyLayout WRITE setFrequencyLayout NOTIFY frequencyLayoutChanged)
    Q_PROPERTY(bool mirrorEnabled READ mirrorEnabled WRITE setMirrorEnabled NOTIFY mirrorEnabledChanged)
    Q_PROPERTY(bool invertDirection READ invertDirection WRITE setInvertDirection NOTIFY invertDirectionChanged)
    Q_PROPERTY(bool showAxes READ showAxes WRITE setShowAxes NOTIFY showAxesChanged)
    Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor NOTIFY axisColorChanged)
    Q_PROPERTY(qreal axisOffset READ axisOffset WRITE setAxisOffset NOTIFY axisOffsetChanged)

    // Umbrales de barras y alcance automático
    Q_PROPERTY(bool autoMaxThreshold READ autoMaxThreshold WRITE setAutoMaxThreshold NOTIFY autoMaxThresholdChanged)
    Q_PROPERTY(qreal reachPercent READ reachPercent WRITE setReachPercent NOTIFY reachPercentChanged)
    Q_PROPERTY(qreal effectiveMaxReach READ effectiveMaxReach NOTIFY effectiveMaxReachChanged)
    Q_PROPERTY(qreal minThreshold READ minThreshold WRITE setMinThreshold NOTIFY minThresholdChanged)
    Q_PROPERTY(qreal maxThreshold READ maxThreshold WRITE setMaxThreshold NOTIFY maxThresholdChanged)
    Q_PROPERTY(bool showThresholdLine READ showThresholdLine WRITE setShowThresholdLine NOTIFY showThresholdLineChanged)
    Q_PROPERTY(QColor thresholdLineColor READ thresholdLineColor WRITE setThresholdLineColor NOTIFY thresholdLineColorChanged)

    // Estilos visuales de barras
    Q_PROPERTY(VisualStyle visualStyle READ visualStyle WRITE setVisualStyle NOTIFY visualStyleChanged)
    Q_PROPERTY(qreal segmentHeight READ segmentHeight WRITE setSegmentHeight NOTIFY segmentHeightChanged)
    Q_PROPERTY(qreal segmentGap READ segmentGap WRITE setSegmentGap NOTIFY segmentGapChanged)
    Q_PROPERTY(bool showPeakCaps READ showPeakCaps WRITE setShowPeakCaps NOTIFY showPeakCapsChanged)
    Q_PROPERTY(qreal peakCapGap READ peakCapGap WRITE setPeakCapGap NOTIFY peakCapGapChanged)
    Q_PROPERTY(qreal peakCapHeight READ peakCapHeight WRITE setPeakCapHeight NOTIFY peakCapHeightChanged)
    Q_PROPERTY(int waveLayers READ waveLayers WRITE setWaveLayers NOTIFY waveLayersChanged)
    Q_PROPERTY(qreal waveSpeed READ waveSpeed WRITE setWaveSpeed NOTIFY waveSpeedChanged)
    Q_PROPERTY(qreal waveSpread READ waveSpread WRITE setWaveSpread NOTIFY waveSpreadChanged)
    Q_PROPERTY(qreal dotSize READ dotSize WRITE setDotSize NOTIFY dotSizeChanged)
    Q_PROPERTY(qreal dotGap READ dotGap WRITE setDotGap NOTIFY dotGapChanged)
    Q_PROPERTY(int gridRows READ gridRows WRITE setGridRows NOTIFY gridRowsChanged)
    Q_PROPERTY(qreal stringGlow READ stringGlow WRITE setStringGlow NOTIFY stringGlowChanged)

    // Geometría de barras
    Q_PROPERTY(bool autoFillScreen READ autoFillScreen WRITE setAutoFillScreen NOTIFY autoFillScreenChanged)
    Q_PROPERTY(qreal barWidth READ barWidth WRITE setBarWidth NOTIFY barWidthChanged)
    Q_PROPERTY(qreal barSpacing READ barSpacing WRITE setBarSpacing NOTIFY barSpacingChanged)
    Q_PROPERTY(qreal maxBarLength READ maxBarLength WRITE setMaxBarLength NOTIFY maxBarLengthChanged)
    Q_PROPERTY(qreal barRadius READ barRadius WRITE setBarRadius NOTIFY barRadiusChanged)

    // Perspectiva 3D / 2.5D
    Q_PROPERTY(qreal tiltPitch READ tiltPitch WRITE setTiltPitch NOTIFY tiltPitchChanged)
    Q_PROPERTY(qreal tiltYaw READ tiltYaw WRITE setTiltYaw NOTIFY tiltYawChanged)
    Q_PROPERTY(qreal perspectiveDepth READ perspectiveDepth WRITE setPerspectiveDepth NOTIFY perspectiveDepthChanged)

    // Sombra residual y transparencia
    Q_PROPERTY(bool residualEnabled READ residualEnabled WRITE setResidualEnabled NOTIFY residualEnabledChanged)
    Q_PROPERTY(qreal residualDecay READ residualDecay WRITE setResidualDecay NOTIFY residualDecayChanged)
    Q_PROPERTY(qreal residualOpacity READ residualOpacity WRITE setResidualOpacity NOTIFY residualOpacityChanged)
    Q_PROPERTY(qreal residualOffset READ residualOffset WRITE setResidualOffset NOTIFY residualOffsetChanged)
    Q_PROPERTY(qreal transparency READ transparency WRITE setTransparency NOTIFY transparencyChanged)

    // Colores y fuente de audio
    Q_PROPERTY(ColorPalette* colorPalette READ colorPalette CONSTANT)
    Q_PROPERTY(ColorMode colorMode READ colorMode WRITE setColorMode NOTIFY colorModeChanged)
    Q_PROPERTY(AudioSpectrumSource* spectrumSource READ spectrumSource WRITE setSpectrumSource NOTIFY spectrumSourceChanged)

public:
    enum OriginAxis {
        AxisX, // Barras nacen del eje horizontal X (y=0)
        AxisY  // Barras nacen del eje vertical Y (x=0)
    };
    Q_ENUM(OriginAxis)

    enum FrequencyLayout {
        CenterOut, // Graves en el centro (0,0), agudos hacia los extremos
        Linear     // Distribución continua de un extremo al otro
    };
    Q_ENUM(FrequencyLayout)

    enum ColorMode {
        GradientAlongHeight, // Gradiente desde la base hasta la punta
        GradientAcrossBars,  // Gradiente a lo largo del rango de barras
        Both                 // Combinación de ambos
    };
    Q_ENUM(ColorMode)

    enum VisualStyle {
        SolidBars,         // 0: Barras sólidas rectangulares
        SpikeWaveform,     // 1: Puntas / Agujas dentadas de frecuencia
        FloatingPeakCaps,  // 2: Barras con picos flotantes desacoplados
        HarmonicWaves,     // 3: Ondas armónicas entrelazadas / Osciloscopio
        SegmentedBlocks,   // 4: Bloques LED segmentados retro
        DotMatrix,         // 5: Matriz de Puntos / Cuentas espectrales
        ZigZagWave,        // 6: Onda Zig-Zag Neón / Poligonal
        SilkStrings,       // 7: Cuerdas Resonantes / Fibras Ópticas
        VuGridMatrix       // 8: Matriz Cuadrícula VU (Studio Rack)
    };
    Q_ENUM(VisualStyle)

    explicit CartesianVisualizer(QQuickItem* parent = nullptr);
    ~CartesianVisualizer() override = default;

    void paint(QPainter* painter) override;

    OriginAxis originAxis() const { return m_originAxis; }
    void setOriginAxis(OriginAxis axis);

    FrequencyLayout frequencyLayout() const { return m_frequencyLayout; }
    void setFrequencyLayout(FrequencyLayout layout);

    bool mirrorEnabled() const { return m_mirrorEnabled; }
    void setMirrorEnabled(bool enabled);

    bool invertDirection() const { return m_invertDirection; }
    void setInvertDirection(bool inverted);

    bool showAxes() const { return m_showAxes; }
    void setShowAxes(bool show);

    bool autoFillScreen() const { return m_autoFillScreen; }
    void setAutoFillScreen(bool fill);

    QColor axisColor() const { return m_axisColor; }
    void setAxisColor(const QColor& color);

    qreal axisOffset() const { return m_axisOffset; }
    void setAxisOffset(qreal offset);

    bool autoMaxThreshold() const { return m_autoMaxThreshold; }
    void setAutoMaxThreshold(bool val);

    qreal reachPercent() const { return m_reachPercent; }
    void setReachPercent(qreal pct);

    qreal effectiveMaxReach() const;

    qreal minThreshold() const { return m_minThreshold; }
    void setMinThreshold(qreal thresh);

    qreal maxThreshold() const { return m_maxThreshold; }
    void setMaxThreshold(qreal thresh);

    bool showThresholdLine() const { return m_showThresholdLine; }
    void setShowThresholdLine(bool show);

    QColor thresholdLineColor() const { return m_thresholdLineColor; }
    void setThresholdLineColor(const QColor& color);

    VisualStyle visualStyle() const { return m_visualStyle; }
    void setVisualStyle(VisualStyle style);

    qreal segmentHeight() const { return m_segmentHeight; }
    void setSegmentHeight(qreal h);

    qreal segmentGap() const { return m_segmentGap; }
    void setSegmentGap(qreal g);

    bool showPeakCaps() const { return m_showPeakCaps; }
    void setShowPeakCaps(bool show);

    qreal peakCapGap() const { return m_peakCapGap; }
    void setPeakCapGap(qreal gap);

    qreal peakCapHeight() const { return m_peakCapHeight; }
    void setPeakCapHeight(qreal h);

    int waveLayers() const { return m_waveLayers; }
    void setWaveLayers(int layers);

    qreal waveSpeed() const { return m_waveSpeed; }
    void setWaveSpeed(qreal speed);

    qreal waveSpread() const { return m_waveSpread; }
    void setWaveSpread(qreal spread);

    qreal dotSize() const { return m_dotSize; }
    void setDotSize(qreal size);

    qreal dotGap() const { return m_dotGap; }
    void setDotGap(qreal gap);

    int gridRows() const { return m_gridRows; }
    void setGridRows(int rows);

    qreal stringGlow() const { return m_stringGlow; }
    void setStringGlow(qreal glow);

    qreal barWidth() const { return m_barWidth; }
    void setBarWidth(qreal width);

    qreal barSpacing() const { return m_barSpacing; }
    void setBarSpacing(qreal spacing);

    qreal maxBarLength() const { return m_maxBarLength; }
    void setMaxBarLength(qreal length);

    qreal barRadius() const { return m_barRadius; }
    void setBarRadius(qreal radius);

    qreal tiltPitch() const { return m_tiltPitch; }
    void setTiltPitch(qreal pitch);

    qreal tiltYaw() const { return m_tiltYaw; }
    void setTiltYaw(qreal yaw);

    qreal perspectiveDepth() const { return m_perspectiveDepth; }
    void setPerspectiveDepth(qreal depth);

    bool residualEnabled() const { return m_residualEnabled; }
    void setResidualEnabled(bool enabled);

    qreal residualDecay() const { return m_residualDecay; }
    void setResidualDecay(qreal decay);

    qreal residualOpacity() const { return m_residualOpacity; }
    void setResidualOpacity(qreal opacity);

    qreal residualOffset() const { return m_residualOffset; }
    void setResidualOffset(qreal offset);

    qreal transparency() const { return m_transparency; }
    void setTransparency(qreal alpha);

    ColorPalette* colorPalette() { return &m_palette; }
    ColorMode colorMode() const { return m_colorMode; }
    void setColorMode(ColorMode mode);

    AudioSpectrumSource* spectrumSource() const { return m_spectrumSource; }
    void setSpectrumSource(AudioSpectrumSource* source);

signals:
    void originAxisChanged();
    void frequencyLayoutChanged();
    void mirrorEnabledChanged();
    void invertDirectionChanged();
    void showAxesChanged();
    void autoFillScreenChanged();
    void axisColorChanged();
    void axisOffsetChanged();
    void autoMaxThresholdChanged();
    void reachPercentChanged();
    void effectiveMaxReachChanged();
    void minThresholdChanged();
    void maxThresholdChanged();
    void showThresholdLineChanged();
    void thresholdLineColorChanged();
    void visualStyleChanged();
    void segmentHeightChanged();
    void segmentGapChanged();
    void showPeakCapsChanged();
    void peakCapGapChanged();
    void peakCapHeightChanged();
    void waveLayersChanged();
    void waveSpeedChanged();
    void waveSpreadChanged();
    void dotSizeChanged();
    void dotGapChanged();
    void gridRowsChanged();
    void stringGlowChanged();
    void barWidthChanged();
    void barSpacingChanged();
    void maxBarLengthChanged();
    void barRadiusChanged();
    void tiltPitchChanged();
    void tiltYawChanged();
    void perspectiveDepthChanged();
    void residualEnabledChanged();
    void residualDecayChanged();
    void residualOpacityChanged();
    void residualOffsetChanged();
    void transparencyChanged();
    void colorModeChanged();
    void spectrumSourceChanged();

protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    QPointF projectPoint(qreal x, qreal y, qreal z, const QPointF& center) const;

    void updateResiduals(const QList<float>& spectrum);
    void drawAxes(QPainter* painter, const QPointF& center);
    void drawThresholdLine(QPainter* painter, const QPointF& center);
    void drawBarQuad(QPainter* painter, const QPointF& center,
                     qreal axisCoord1, qreal axisCoord2,
                     qreal height, qreal zOffset,
                     const QColor& colorBase, const QColor& colorTip,
                     qreal opacity);
    void drawSpike(QPainter* painter, const QPointF& center,
                   qreal axisCoord1, qreal axisCoord2,
                   qreal height, qreal zOffset,
                   const QColor& colorBase, const QColor& colorTip,
                   qreal opacity);
    void drawSegmentedBar(QPainter* painter, const QPointF& center,
                          qreal axisCoord1, qreal axisCoord2,
                          qreal height, qreal zOffset,
                          const QColor& colorBase, const QColor& colorTip,
                          qreal opacity);
    void drawPeakCap(QPainter* painter, const QPointF& center,
                     qreal axisCoord1, qreal axisCoord2,
                     qreal peakHeight, qreal zOffset,
                     const QColor& colorTip, qreal opacity);
    void drawHarmonicWaves(QPainter* painter, const QPointF& center,
                           qreal totalSpan, qreal step, int numBars, qreal mainAlpha);
    void drawDotColumn(QPainter* painter, const QPointF& center,
                       qreal axisCoord1, qreal axisCoord2,
                       qreal height, qreal zOffset,
                       const QColor& colorBase, const QColor& colorTip,
                       qreal opacity);
    void drawZigZagWave(QPainter* painter, const QPointF& center,
                        qreal startCoord, qreal step, int numBars, qreal maxReach, qreal mainAlpha);
    void drawSilkStrings(QPainter* painter, const QPointF& center,
                         qreal startCoord, qreal step, int numBars, qreal maxReach, qreal mainAlpha);
    void drawVuGridMatrix(QPainter* painter, const QPointF& center,
                          qreal startCoord, qreal step, int numBars, qreal maxReach, qreal mainAlpha);

    OriginAxis m_originAxis{AxisX};
    FrequencyLayout m_frequencyLayout{CenterOut};
    bool m_mirrorEnabled{true};
    bool m_invertDirection{false};
    bool m_showAxes{false};
    bool m_autoFillScreen{false};
    QColor m_axisColor{QColor(255, 255, 255, 60)};
    qreal m_axisOffset{0.0};

    qreal m_minThreshold{0.02};
    qreal m_maxThreshold{1.0};
    bool m_autoMaxThreshold{true};
    qreal m_reachPercent{1.0};
    bool m_showThresholdLine{false};
    QColor m_thresholdLineColor{QColor(255, 100, 100, 100)};

    VisualStyle m_visualStyle{SolidBars};
    qreal m_segmentHeight{6.0};
    qreal m_segmentGap{2.0};
    bool m_showPeakCaps{false};
    qreal m_peakCapGap{4.0};
    qreal m_peakCapHeight{3.0};

    int m_waveLayers{5};
    qreal m_waveSpeed{1.0};
    qreal m_waveSpread{1.0};
    qreal m_wavePhase{0.0};
    qreal m_dotSize{6.0};
    qreal m_dotGap{3.0};
    int m_gridRows{14};
    qreal m_stringGlow{1.0};

    qreal m_barWidth{10.0};
    qreal m_barSpacing{4.0};
    qreal m_maxBarLength{240.0};
    qreal m_barRadius{2.0};

    qreal m_tiltPitch{25.0};
    qreal m_tiltYaw{0.0};
    qreal m_perspectiveDepth{800.0};

    bool m_residualEnabled{true};
    qreal m_residualDecay{0.91};
    qreal m_residualOpacity{0.45};
    qreal m_residualOffset{4.0};
    qreal m_transparency{1.0};

    ColorPalette m_palette;
    ColorMode m_colorMode{Both};
    AudioSpectrumSource* m_spectrumSource{nullptr};

    QList<float> m_currentSpectrum;
    QList<float> m_currentPeaks;
    std::vector<float> m_residualSpectrum;
};

} // namespace AudioViewer
