#include "CartesianVisualizer.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace AudioViewer {

CartesianVisualizer::CartesianVisualizer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);

    connect(&m_palette, &ColorPalette::colorsChanged, this, [this]() {
        update();
    });
}

void CartesianVisualizer::setSpectrumSource(AudioSpectrumSource* source)
{
    if (m_spectrumSource == source) {
        return;
    }

    if (m_spectrumSource) {
        disconnect(m_spectrumSource, nullptr, this, nullptr);
    }

    m_spectrumSource = source;

    if (m_spectrumSource) {
        connect(m_spectrumSource, &AudioSpectrumSource::spectrumUpdated,
                this, [this](const QList<float>& spectrum) {
            updateResiduals(spectrum);
            m_currentSpectrum = spectrum;
            update();
        });
        connect(m_spectrumSource, &AudioSpectrumSource::peaksUpdated,
                this, [this](const QList<float>& peaks) {
            m_currentPeaks = peaks;
        });
    }

    emit spectrumSourceChanged();
}

void CartesianVisualizer::updateResiduals(const QList<float>& spectrum)
{
    if (m_residualSpectrum.size() != static_cast<size_t>(spectrum.size())) {
        m_residualSpectrum.assign(spectrum.size(), 0.0f);
    }

    for (size_t i = 0; i < static_cast<size_t>(spectrum.size()); ++i) {
        float cur = spectrum.at(static_cast<qsizetype>(i));
        if (cur >= m_residualSpectrum[i]) {
            m_residualSpectrum[i] = cur;
        } else {
            m_residualSpectrum[i] *= static_cast<float>(m_residualDecay);
            if (m_residualSpectrum[i] < 0.001f) {
                m_residualSpectrum[i] = 0.0f;
            }
        }
    }
}

QPointF CartesianVisualizer::projectPoint(qreal x, qreal y, qreal z, const QPointF& center) const
{
    // Conversión de grados a radianes
    constexpr qreal degToRad = std::numbers::pi / 180.0;
    qreal pitch = m_tiltPitch * degToRad;
    qreal yaw   = m_tiltYaw   * degToRad;

    // Rotación Pitch (eje X)
    qreal cosP = std::cos(pitch);
    qreal sinP = std::sin(pitch);
    qreal x1 = x;
    qreal y1 = y * cosP - z * sinP;
    qreal z1 = y * sinP + z * cosP;

    // Rotación Yaw (eje Y)
    qreal cosY = std::cos(yaw);
    qreal sinY = std::sin(yaw);
    qreal x2 =  x1 * cosY + z1 * sinY;
    qreal y2 =  y1;
    qreal z2 = -x1 * sinY + z1 * cosY;

    // Proyección en perspectiva con centro (0,0) de pantalla
    qreal depth = std::max<qreal>(50.0, m_perspectiveDepth);
    qreal denom = 1.0 + (z2 / depth);
    if (denom < 0.05) {
        denom = 0.05;
    }
    qreal scale = 1.0 / denom;

    // Cartesiano: +X hacia la derecha, +Y hacia arriba. Pantalla: +Y hacia abajo.
    qreal screenX = center.x() + (x2 * scale);
    qreal screenY = center.y() - (y2 * scale);

    return QPointF(screenX, screenY);
}

void CartesianVisualizer::drawAxes(QPainter* painter, const QPointF& center)
{
    painter->save();
    QPen pen(m_axisColor);
    pen.setWidthF(1.5);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);

    qreal spanX = width() * 0.55;
    qreal spanY = height() * 0.55;

    // Eje X: de (-spanX, 0, 0) a (+spanX, 0, 0)
    QPointF xNeg = projectPoint(-spanX, 0, 0, center);
    QPointF xPos = projectPoint(+spanX, 0, 0, center);
    painter->drawLine(xNeg, xPos);

    // Eje Y: de (0, -spanY, 0) a (0, +spanY, 0)
    QPointF yNeg = projectPoint(0, -spanY, 0, center);
    QPointF yPos = projectPoint(0, +spanY, 0, center);
    painter->drawLine(yNeg, yPos);

    // Punto central (0, 0)
    QPointF origin = projectPoint(0, 0, 0, center);
    painter->setBrush(QBrush(m_axisColor));
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(origin, 4.0, 4.0);

    // Si el eje de nacimiento está desplazado (axisOffset != 0), dibujamos la línea base
    if (std::abs(m_axisOffset) > 0.5) {
        QPen basePen(m_axisColor);
        basePen.setWidthF(1.2);
        basePen.setStyle(Qt::DotLine);
        painter->setPen(basePen);

        if (m_originAxis == AxisX) {
            QPointF bNeg = projectPoint(-spanX, m_axisOffset, 0, center);
            QPointF bPos = projectPoint(+spanX, m_axisOffset, 0, center);
            painter->drawLine(bNeg, bPos);
        } else {
            QPointF bNeg = projectPoint(m_axisOffset, -spanY, 0, center);
            QPointF bPos = projectPoint(m_axisOffset, +spanY, 0, center);
            painter->drawLine(bNeg, bPos);
        }
    }

    painter->restore();
}

qreal CartesianVisualizer::effectiveMaxReach() const
{
    qreal screenDim = (m_originAxis == AxisX) ? height() : width();
    if (screenDim <= 10.0) {
        screenDim = 1080.0;
    }
    qreal halfDim = screenDim * 0.5;

    // Distancia base disponible desde el eje de nacimiento hasta el borde de la pantalla
    qreal targetReach = 0.0;

    if (m_originAxis == AxisX) {
        if (m_mirrorEnabled) {
            // En modo bilateral/espejo:
            // Si el eje está centrado (axisOffset == 0), cada ala de halfDim cubre 50% de la pantalla (100% en total)
            // Si el eje está desplazado, el ala hacia el borde más lejano debe alcanzar ese borde
            qreal distTop    = halfDim - m_axisOffset;
            qreal distBottom = halfDim + m_axisOffset;
            targetReach = std::max(distTop, distBottom);
        } else if (m_invertDirection) {
            // Hacia abajo (-Y cartesiano)
            targetReach = halfDim + m_axisOffset;
        } else {
            // Hacia arriba (+Y cartesiano)
            targetReach = halfDim - m_axisOffset;
        }
    } else { // AxisY
        if (m_mirrorEnabled) {
            qreal distRight = halfDim - m_axisOffset;
            qreal distLeft  = halfDim + m_axisOffset;
            targetReach = std::max(distRight, distLeft);
        } else if (m_invertDirection) {
            targetReach = halfDim - m_axisOffset;
        } else {
            targetReach = halfDim + m_axisOffset;
        }
    }

    targetReach = std::max<qreal>(50.0, targetReach);

    // Compensación de perspectiva 3D (pitch y yaw):
    // La rotación en 3D (pitch / yaw) comprime la proyección en pantalla.
    // Calculamos el factor de escala proyectivo para que el 100% de alcance toque efectivamente el borde visual.
    qreal pitchRad = std::abs(m_tiltPitch) * (std::numbers::pi / 180.0);
    qreal cosP = std::cos(pitchRad);
    qreal sinP = std::sin(pitchRad);
    qreal depth = std::max<qreal>(50.0, m_perspectiveDepth);

    qreal compensation = 1.0;
    qreal denom = cosP - (targetReach / depth) * sinP * 0.5;
    if (denom > 0.15) {
        compensation = 1.0 / denom;
    } else if (cosP > 0.1) {
        compensation = 1.0 / cosP;
    }
    compensation = std::clamp<qreal>(compensation, 1.0, 2.5);

    qreal fullReach = targetReach * compensation;

    // Aplicar porcentaje de alcance (1.0 = 100% de la pantalla)
    return fullReach * m_reachPercent;
}

void CartesianVisualizer::setAutoMaxThreshold(bool val)
{
    if (m_autoMaxThreshold != val) {
        m_autoMaxThreshold = val;
        if (val) {
            m_reachPercent = 1.0;
            emit reachPercentChanged();
        }
        emit autoMaxThresholdChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setReachPercent(qreal pct)
{
    pct = std::clamp(pct, 0.05, 1.5);
    if (std::abs(m_reachPercent - pct) > 0.001) {
        m_reachPercent = pct;
        m_autoMaxThreshold = (pct >= 0.99);
        emit reachPercentChanged();
        emit autoMaxThresholdChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    emit effectiveMaxReachChanged();
}

void CartesianVisualizer::drawThresholdLine(QPainter* painter, const QPointF& center)
{
    painter->save();
    QPen pen(m_thresholdLineColor);
    pen.setWidthF(1.5);
    pen.setStyle(Qt::DashDotLine);
    painter->setPen(pen);

    qreal spanX = width() * 0.55;
    qreal spanY = height() * 0.55;
    qreal reach = effectiveMaxReach();

    if (m_originAxis == AxisX) {
        qreal yPosThresh = m_axisOffset + reach;
        QPointF p1 = projectPoint(-spanX, yPosThresh, 0, center);
        QPointF p2 = projectPoint(+spanX, yPosThresh, 0, center);
        painter->drawLine(p1, p2);

        if (m_mirrorEnabled) {
            qreal yNegThresh = m_axisOffset - reach;
            QPointF m1 = projectPoint(-spanX, yNegThresh, 0, center);
            QPointF m2 = projectPoint(+spanX, yNegThresh, 0, center);
            painter->drawLine(m1, m2);
        }
    } else {
        qreal xPosThresh = m_axisOffset + reach;
        QPointF p1 = projectPoint(xPosThresh, -spanY, 0, center);
        QPointF p2 = projectPoint(xPosThresh, +spanY, 0, center);
        painter->drawLine(p1, p2);

        if (m_mirrorEnabled) {
            qreal xNegThresh = m_axisOffset - reach;
            QPointF m1 = projectPoint(xNegThresh, -spanY, 0, center);
            QPointF m2 = projectPoint(xNegThresh, +spanY, 0, center);
            painter->drawLine(m1, m2);
        }
    }
    painter->restore();
}

void CartesianVisualizer::drawBarQuad(QPainter* painter, const QPointF& center,
                                      qreal axisCoord1, qreal axisCoord2,
                                      qreal height, qreal zOffset,
                                      const QColor& colorBase, const QColor& colorTip,
                                      qreal opacity)
{
    if (std::abs(height) < 0.5) {
        return;
    }

    bool isFlat2D = (std::abs(m_tiltPitch) < 0.1 && std::abs(m_tiltYaw) < 0.1 && std::abs(zOffset) < 0.1);
    if (isFlat2D) {
        qreal barLeft  = std::min(axisCoord1, axisCoord2);
        qreal barWidth = std::abs(axisCoord2 - axisCoord1);
        QRectF rect;
        if (m_originAxis == AxisX) {
            qreal screenX = center.x() + barLeft;
            qreal screenY0 = center.y() - m_axisOffset;
            qreal screenY1 = center.y() - (m_axisOffset + height);
            rect = QRectF(screenX, std::min(screenY0, screenY1), barWidth, std::abs(screenY1 - screenY0));
        } else {
            qreal screenX0 = center.x() + m_axisOffset;
            qreal screenX1 = center.x() + (m_axisOffset + height);
            qreal screenY = center.y() - (barLeft + barWidth);
            rect = QRectF(std::min(screenX0, screenX1), screenY, std::abs(screenX1 - screenX0), barWidth);
        }

        if (colorBase == colorTip) {
            QColor c = colorBase;
            c.setAlphaF(c.alphaF() * opacity);
            painter->fillRect(rect, c);
            return;
        }

        QLinearGradient grad(rect.bottomLeft(), rect.topLeft());
        if (height < 0) {
            grad.setStart(rect.topLeft());
            grad.setFinalStop(rect.bottomLeft());
        }
        if (m_originAxis == AxisY) {
            grad = (height >= 0) ? QLinearGradient(rect.topLeft(), rect.topRight())
                                 : QLinearGradient(rect.topRight(), rect.topLeft());
        }
        QColor cBase = colorBase;
        cBase.setAlphaF(cBase.alphaF() * opacity);
        QColor cTip = colorTip;
        cTip.setAlphaF(cTip.alphaF() * opacity);
        grad.setColorAt(0.0, cBase);
        grad.setColorAt(1.0, cTip);
        painter->fillRect(rect, grad);
        return;
    }

    QPointF p0, p1, p2, p3;
    if (m_originAxis == AxisX) {
        p0 = projectPoint(axisCoord1, m_axisOffset, zOffset, center);
        p1 = projectPoint(axisCoord2, m_axisOffset, zOffset, center);
        p2 = projectPoint(axisCoord2, m_axisOffset + height, zOffset, center);
        p3 = projectPoint(axisCoord1, m_axisOffset + height, zOffset, center);
    } else {
        p0 = projectPoint(m_axisOffset, axisCoord1, zOffset, center);
        p1 = projectPoint(m_axisOffset, axisCoord2, zOffset, center);
        p2 = projectPoint(m_axisOffset + height, axisCoord2, zOffset, center);
        p3 = projectPoint(m_axisOffset + height, axisCoord1, zOffset, center);
    }

    const QPointF points[4] = {p0, p1, p2, p3};
    QPointF baseMid((p0.x() + p1.x()) * 0.5, (p0.y() + p1.y()) * 0.5);
    QPointF tipMid((p2.x() + p3.x()) * 0.5, (p2.y() + p3.y()) * 0.5);

    QLinearGradient grad(baseMid, tipMid);
    QColor cBase = colorBase;
    cBase.setAlphaF(cBase.alphaF() * opacity);
    QColor cTip = colorTip;
    cTip.setAlphaF(cTip.alphaF() * opacity);

    grad.setColorAt(0.0, cBase);
    grad.setColorAt(1.0, cTip);

    painter->setBrush(QBrush(grad));
    painter->setPen(Qt::NoPen);
    painter->drawConvexPolygon(points, 4);
}

void CartesianVisualizer::drawSegmentedBar(QPainter* painter, const QPointF& center,
                                           qreal axisCoord1, qreal axisCoord2,
                                           qreal height, qreal zOffset,
                                           const QColor& colorBase, const QColor& colorTip,
                                           qreal opacity)
{
    if (std::abs(height) < 1.0) {
        return;
    }

    qreal segStep = m_segmentHeight + m_segmentGap;
    if (segStep < 1.0) {
        drawBarQuad(painter, center, axisCoord1, axisCoord2, height, zOffset, colorBase, colorTip, opacity);
        return;
    }

    qreal absH = std::abs(height);
    int numSegments = static_cast<int>(std::floor(absH / segStep));
    if (numSegments <= 0) return;

    qreal sign = (height >= 0.0) ? 1.0 : -1.0;
    bool isFlat2D = (std::abs(m_tiltPitch) < 0.1 && std::abs(m_tiltYaw) < 0.1 && std::abs(zOffset) < 0.1);

    if (isFlat2D) {
        // Camino ultra-rápido SIMD en 2D plano (fillRect de memoria directa)
        qreal barLeft  = std::min(axisCoord1, axisCoord2);
        qreal barWidth = std::abs(axisCoord2 - axisCoord1);

        for (int s = 0; s < numSegments; ++s) {
            qreal h0 = sign * (s * segStep);
            qreal h1 = h0 + sign * m_segmentHeight;

            float t0 = static_cast<float>(s) / static_cast<float>(std::max(1, numSegments - 1));
            QColor c = colorBase;
            if (m_colorMode == GradientAlongHeight || m_colorMode == Both) {
                c = m_palette.sample(t0);
            } else {
                c = colorTip;
            }
            c.setAlphaF(c.alphaF() * opacity);

            QRectF rect;
            if (m_originAxis == AxisX) {
                qreal screenX = center.x() + barLeft;
                qreal screenY0 = center.y() - (m_axisOffset + h0);
                qreal screenY1 = center.y() - (m_axisOffset + h1);
                rect = QRectF(screenX, std::min(screenY0, screenY1), barWidth, std::abs(screenY1 - screenY0));
            } else {
                qreal screenX0 = center.x() + (m_axisOffset + h0);
                qreal screenX1 = center.x() + (m_axisOffset + h1);
                qreal screenY = center.y() - (barLeft + barWidth);
                rect = QRectF(std::min(screenX0, screenX1), screenY, std::abs(screenX1 - screenX0), barWidth);
            }

            painter->fillRect(rect, c);
        }
        return;
    }

    // Camino 3D proyectado optimizado (sin allocations heap de QPolygonF ni QLinearGradient)
    painter->setPen(Qt::NoPen);
    for (int s = 0; s < numSegments; ++s) {
        qreal h0 = sign * (s * segStep);
        qreal h1 = h0 + sign * m_segmentHeight;

        float t0 = static_cast<float>(s) / static_cast<float>(std::max(1, numSegments - 1));
        QColor c = colorBase;
        if (m_colorMode == GradientAlongHeight || m_colorMode == Both) {
            c = m_palette.sample(t0);
        } else {
            c = colorTip;
        }
        c.setAlphaF(c.alphaF() * opacity);

        QPointF p0, p1, p2, p3;
        if (m_originAxis == AxisX) {
            p0 = projectPoint(axisCoord1, m_axisOffset + h0, zOffset, center);
            p1 = projectPoint(axisCoord2, m_axisOffset + h0, zOffset, center);
            p2 = projectPoint(axisCoord2, m_axisOffset + h1, zOffset, center);
            p3 = projectPoint(axisCoord1, m_axisOffset + h1, zOffset, center);
        } else {
            p0 = projectPoint(m_axisOffset + h0, axisCoord1, zOffset, center);
            p1 = projectPoint(m_axisOffset + h0, axisCoord2, zOffset, center);
            p2 = projectPoint(m_axisOffset + h1, axisCoord2, zOffset, center);
            p3 = projectPoint(m_axisOffset + h1, axisCoord1, zOffset, center);
        }

        const QPointF points[4] = {p0, p1, p2, p3};
        painter->setBrush(c);
        painter->drawConvexPolygon(points, 4);
    }
}

void CartesianVisualizer::drawPeakCap(QPainter* painter, const QPointF& center,
                                      qreal axisCoord1, qreal axisCoord2,
                                      qreal peakHeight, qreal zOffset,
                                      const QColor& colorTip, qreal opacity)
{
    if (std::abs(peakHeight) < 1.0) {
        return;
    }

    qreal capThick = std::max(1.5, m_peakCapHeight);
    qreal sign = (peakHeight >= 0.0) ? 1.0 : -1.0;
    qreal h0 = peakHeight;
    qreal h1 = peakHeight + sign * capThick;

    QColor c = colorTip.lighter(135);
    c.setAlphaF(std::min(1.0, (opacity + 0.25) * 1.0));

    bool isFlat2D = (std::abs(m_tiltPitch) < 0.1 && std::abs(m_tiltYaw) < 0.1 && std::abs(zOffset) < 0.1);
    if (isFlat2D) {
        qreal barLeft  = std::min(axisCoord1, axisCoord2);
        qreal barWidth = std::abs(axisCoord2 - axisCoord1);
        QRectF rect;
        if (m_originAxis == AxisX) {
            qreal screenX = center.x() + barLeft;
            qreal screenY0 = center.y() - (m_axisOffset + h0);
            qreal screenY1 = center.y() - (m_axisOffset + h1);
            rect = QRectF(screenX, std::min(screenY0, screenY1), barWidth, std::abs(screenY1 - screenY0));
        } else {
            qreal screenX0 = center.x() + (m_axisOffset + h0);
            qreal screenX1 = center.x() + (m_axisOffset + h1);
            qreal screenY = center.y() - (barLeft + barWidth);
            rect = QRectF(std::min(screenX0, screenX1), screenY, std::abs(screenX1 - screenX0), barWidth);
        }
        painter->fillRect(rect, c);
        return;
    }

    QPointF p0, p1, p2, p3;
    if (m_originAxis == AxisX) {
        p0 = projectPoint(axisCoord1, m_axisOffset + h0, zOffset, center);
        p1 = projectPoint(axisCoord2, m_axisOffset + h0, zOffset, center);
        p2 = projectPoint(axisCoord2, m_axisOffset + h1, zOffset, center);
        p3 = projectPoint(axisCoord1, m_axisOffset + h1, zOffset, center);
    } else {
        p0 = projectPoint(m_axisOffset + h0, axisCoord1, zOffset, center);
        p1 = projectPoint(m_axisOffset + h0, axisCoord2, zOffset, center);
        p2 = projectPoint(m_axisOffset + h1, axisCoord2, zOffset, center);
        p3 = projectPoint(m_axisOffset + h1, axisCoord1, zOffset, center);
    }

    const QPointF points[4] = {p0, p1, p2, p3};
    painter->setBrush(QBrush(c));
    painter->setPen(Qt::NoPen);
    painter->drawConvexPolygon(points, 4);
}

void CartesianVisualizer::drawSpike(QPainter* painter, const QPointF& center,
                                    qreal axisCoord1, qreal axisCoord2,
                                    qreal height, qreal zOffset,
                                    const QColor& colorBase, const QColor& colorTip,
                                    qreal opacity)
{
    if (std::abs(height) < 1.0) {
        return;
    }

    qreal midCoord = (axisCoord1 + axisCoord2) * 0.5;
    QPointF p0, pTip, p1;

    if (m_originAxis == AxisX) {
        p0   = projectPoint(axisCoord1, m_axisOffset, zOffset, center);
        pTip = projectPoint(midCoord, m_axisOffset + height, zOffset, center);
        p1   = projectPoint(axisCoord2, m_axisOffset, zOffset, center);
    } else {
        p0   = projectPoint(m_axisOffset, axisCoord1, zOffset, center);
        pTip = projectPoint(m_axisOffset + height, midCoord, zOffset, center);
        p1   = projectPoint(m_axisOffset, axisCoord2, zOffset, center);
    }

    const QPointF points[3] = {p0, pTip, p1};

    QPointF baseMid((p0.x() + p1.x()) * 0.5, (p0.y() + p1.y()) * 0.5);
    QLinearGradient grad(baseMid, pTip);
    QColor cBase = colorBase;
    cBase.setAlphaF(cBase.alphaF() * opacity);
    QColor cTip = colorTip;
    cTip.setAlphaF(cTip.alphaF() * opacity);
    grad.setColorAt(0.0, cBase);
    grad.setColorAt(1.0, cTip);

    painter->setBrush(QBrush(grad));
    painter->setPen(Qt::NoPen);
    painter->drawConvexPolygon(points, 3);
}

void CartesianVisualizer::drawHarmonicWaves(QPainter* painter, const QPointF& center,
                                            qreal totalSpan, qreal step, int numBars, qreal mainAlpha)
{
    Q_UNUSED(step);
    if (m_currentSpectrum.isEmpty() || numBars < 2) return;

    qreal halfSpan = totalSpan * 0.5;
    auto sampleNormAmp = [this, halfSpan](qreal x) -> qreal {
        qreal normPos = (x + halfSpan) / (halfSpan * 2.0);
        normPos = std::clamp(normPos, 0.0, 1.0);

        float sIdx = normPos * static_cast<float>(m_currentSpectrum.size() - 1);
        int i0 = static_cast<int>(std::floor(sIdx));
        int i1 = std::min<int>(i0 + 1, static_cast<int>(m_currentSpectrum.size() - 1));
        float frac = sIdx - static_cast<float>(i0);

        float rawVal = m_currentSpectrum.at(i0) * (1.0f - frac) + m_currentSpectrum.at(i1) * frac;

        float effVal = 0.0f;
        if (rawVal >= m_minThreshold) {
            float norm = (rawVal - static_cast<float>(m_minThreshold)) /
                         std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
            effVal = std::clamp(norm, 0.0f, 1.0f);
        }
        return effVal * effectiveMaxReach();
    };

    int numLayers = std::clamp(m_waveLayers, 2, 8);
    qreal xStep = std::max(2.5, totalSpan / 180.0);

    for (int L = 0; L < numLayers; ++L) {
        float layerT = static_cast<float>(L) / std::max(1, numLayers - 1);
        QColor waveCol = m_palette.sample(layerT);
        qreal layerAlpha = (0.35 + 0.55 * (1.0 - layerT * 0.45)) * mainAlpha;
        waveCol.setAlphaF(std::clamp(layerAlpha, 0.0, 1.0));

        qreal freqK = (0.012 + 0.006 * L) * m_waveSpread;
        qreal phaseShift = m_wavePhase * (1.0 + 0.28 * L) * m_waveSpeed + (L * 1.04719755);

        QPainterPath path;
        QPainterPath mirrorPath;
        bool firstPt = true;

        for (qreal x = -halfSpan; x <= halfSpan + 0.1; x += xStep) {
            qreal amp = sampleNormAmp(x);
            qreal sinVal = std::sin(freqK * x + phaseShift);
            qreal harmonic2 = 0.32 * std::cos(freqK * 2.2 * x - phaseShift * 0.7);
            qreal yOffset = amp * (sinVal + harmonic2);

            if (m_invertDirection) {
                yOffset = -yOffset;
            }

            QPointF pt = (m_originAxis == AxisX)
                ? projectPoint(x, m_axisOffset + yOffset, 0.0, center)
                : projectPoint(m_axisOffset + yOffset, x, 0.0, center);

            if (firstPt) {
                path.moveTo(pt);
            } else {
                path.lineTo(pt);
            }

            if (m_mirrorEnabled) {
                QPointF mPt = (m_originAxis == AxisX)
                    ? projectPoint(x, m_axisOffset - yOffset, 0.0, center)
                    : projectPoint(m_axisOffset - yOffset, x, 0.0, center);
                if (firstPt) {
                    mirrorPath.moveTo(mPt);
                } else {
                    mirrorPath.lineTo(mPt);
                }
            }

            firstPt = false;
        }

        qreal penWidth = std::max(1.8, m_barWidth * 0.28);
        QPen pen(waveCol, penWidth);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);

        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);

        if (m_mirrorEnabled) {
            painter->drawPath(mirrorPath);
        }
    }
}

void CartesianVisualizer::drawDotColumn(QPainter* painter, const QPointF& center,
                                        qreal axisCoord1, qreal axisCoord2,
                                        qreal height, qreal zOffset,
                                        const QColor& colorBase, const QColor& colorTip,
                                        qreal opacity)
{
    if (std::abs(height) < 1.0) {
        return;
    }

    qreal dotStep = m_dotSize + m_dotGap;
    if (dotStep < 1.5) dotStep = 6.0;
    qreal absH = std::abs(height);
    int numDots = static_cast<int>(std::floor(absH / dotStep));
    if (numDots <= 0) return;

    qreal sign = (height >= 0.0) ? 1.0 : -1.0;
    qreal midCoord = (axisCoord1 + axisCoord2) * 0.5;
    qreal diam = std::min(m_barWidth, m_dotSize);
    qreal radius = std::max(1.0, diam * 0.5);

    for (int s = 0; s < numDots; ++s) {
        qreal h = sign * (s * dotStep + radius);
        float t = static_cast<float>(s) / static_cast<float>(std::max(1, numDots - 1));

        QColor c = colorBase;
        if (m_colorMode == GradientAlongHeight || m_colorMode == Both) {
            c = m_palette.sample(t);
        } else {
            c = colorTip;
        }
        c.setAlphaF(c.alphaF() * opacity);

        QPointF pt = (m_originAxis == AxisX)
            ? projectPoint(midCoord, m_axisOffset + h, zOffset, center)
            : projectPoint(m_axisOffset + h, midCoord, zOffset, center);

        painter->setBrush(QBrush(c));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(pt, radius, radius);
    }
}

void CartesianVisualizer::drawZigZagWave(QPainter* painter, const QPointF& center,
                                         qreal startCoord, qreal step, int numBars,
                                         qreal maxReach, qreal mainAlpha)
{
    if (numBars < 2 || m_currentSpectrum.isEmpty()) return;

    const int maxRank = (m_frequencyLayout == CenterOut) ? std::max(1, (numBars - 1) / 2) : std::max(1, numBars - 1);

    auto sampleAmp = [this, maxRank, maxReach](int rankIdx) -> qreal {
        float t = std::clamp(static_cast<float>(rankIdx) / static_cast<float>(maxRank), 0.0f, 1.0f);
        float sIdx = t * static_cast<float>(m_currentSpectrum.size() - 1);
        int i0 = static_cast<int>(std::floor(sIdx));
        int i1 = std::min<int>(i0 + 1, static_cast<int>(m_currentSpectrum.size() - 1));
        float frac = sIdx - static_cast<float>(i0);
        float rawVal = m_currentSpectrum.at(i0) * (1.0f - frac) + m_currentSpectrum.at(i1) * frac;
        float effVal = 0.0f;
        if (rawVal >= m_minThreshold) {
            float norm = (rawVal - static_cast<float>(m_minThreshold)) /
                         std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
            effVal = std::clamp(norm, 0.0f, 1.0f);
        }
        qreal h = effVal * maxReach;
        if (m_invertDirection) h = -h;
        return h;
    };

    struct ZigPoint {
        QPointF pt;
        float colorT;
    };

    auto renderZigPass = [&](bool mirror) {
        std::vector<ZigPoint> points;
        points.reserve(numBars * 2);

        qreal sign = mirror ? -1.0 : 1.0;

        for (int i = 0; i < numBars; ++i) {
            int specIdx = i;
            qreal cMid;
            if (m_frequencyLayout == CenterOut) {
                int rank = i / 2;
                qreal offset = rank * step;
                cMid = (i % 2 == 0) ? (offset + m_barWidth * 0.5) : (-offset - m_barWidth * 0.5);
                specIdx = std::min(rank, maxRank);
            } else {
                cMid = startCoord + i * step + m_barWidth * 0.5;
            }

            qreal h = sampleAmp(specIdx) * sign;
            float t = static_cast<float>(specIdx) / static_cast<float>(maxRank);

            QPointF pPeak = (m_originAxis == AxisX)
                ? projectPoint(cMid, m_axisOffset + h, 0.0, center)
                : projectPoint(m_axisOffset + h, cMid, 0.0, center);
            points.push_back({pPeak, t});

            qreal nextMid = cMid + step * 0.5;
            qreal hValley = -h * 0.65;
            QPointF pValley = (m_originAxis == AxisX)
                ? projectPoint(nextMid, m_axisOffset + hValley, 0.0, center)
                : projectPoint(m_axisOffset + hValley, nextMid, 0.0, center);
            points.push_back({pValley, t});
        }

        if (points.size() < 2) return;

        // Pasada de resplandor difuso
        qreal glowWidth = std::max(4.0, m_barWidth * 0.7);
        for (size_t k = 0; k + 1 < points.size(); ++k) {
            QColor col = m_palette.sample(points[k].colorT);
            col.setAlphaF(0.28 * mainAlpha);
            QPen pen(col, glowWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
            painter->drawLine(points[k].pt, points[k + 1].pt);
        }

        // Pasada de núcleo de neón
        qreal strokeWidth = std::max(2.2, m_barWidth * 0.35);
        for (size_t k = 0; k + 1 < points.size(); ++k) {
            QColor col = m_palette.sample(points[k].colorT).lighter(125);
            col.setAlphaF(mainAlpha);
            QPen pen(col, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(pen);
            painter->drawLine(points[k].pt, points[k + 1].pt);
        }
    };

    renderZigPass(false);
    if (m_mirrorEnabled) {
        renderZigPass(true);
    }
}

void CartesianVisualizer::drawSilkStrings(QPainter* painter, const QPointF& center,
                                          qreal startCoord, qreal step, int numBars,
                                          qreal maxReach, qreal mainAlpha)
{
    if (numBars <= 0 || m_currentSpectrum.isEmpty()) return;

    const int maxRank = (m_frequencyLayout == CenterOut) ? std::max(1, (numBars - 1) / 2) : std::max(1, numBars - 1);

    auto sampleAmp = [this, maxRank, maxReach](int rankIdx) -> qreal {
        float t = std::clamp(static_cast<float>(rankIdx) / static_cast<float>(maxRank), 0.0f, 1.0f);
        float sIdx = t * static_cast<float>(m_currentSpectrum.size() - 1);
        int i0 = static_cast<int>(std::floor(sIdx));
        int i1 = std::min<int>(i0 + 1, static_cast<int>(m_currentSpectrum.size() - 1));
        float frac = sIdx - static_cast<float>(i0);
        float rawVal = m_currentSpectrum.at(i0) * (1.0f - frac) + m_currentSpectrum.at(i1) * frac;
        float effVal = 0.0f;
        if (rawVal >= m_minThreshold) {
            float norm = (rawVal - static_cast<float>(m_minThreshold)) /
                         std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
            effVal = std::clamp(norm, 0.0f, 1.0f);
        }
        return effVal * maxReach;
    };

    qreal stringSpan = maxReach * 0.95;

    for (int i = 0; i < numBars; ++i) {
        int specIdx = i;
        qreal cMid;
        if (m_frequencyLayout == CenterOut) {
            int rank = i / 2;
            qreal offset = rank * step;
            cMid = (i % 2 == 0) ? (offset + m_barWidth * 0.5) : (-offset - m_barWidth * 0.5);
            specIdx = std::min(rank, maxRank);
        } else {
            cMid = startCoord + i * step + m_barWidth * 0.5;
        }

        qreal h = sampleAmp(specIdx);
        float t = static_cast<float>(specIdx) / static_cast<float>(maxRank);
        QColor col = m_palette.sample(t);

        // 1. Cuerda / Hilo vertical continuo
        QPointF pTop = (m_originAxis == AxisX)
            ? projectPoint(cMid, m_axisOffset + stringSpan, 0.0, center)
            : projectPoint(m_axisOffset + stringSpan, cMid, 0.0, center);
        QPointF pBottom = (m_originAxis == AxisX)
            ? projectPoint(cMid, m_axisOffset - stringSpan, 0.0, center)
            : projectPoint(m_axisOffset - stringSpan, cMid, 0.0, center);

        QColor baseCol = col.darker(180);
        baseCol.setAlphaF(0.25 * mainAlpha);
        QPen basePen(baseCol, 1.2, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(basePen);
        painter->drawLine(pBottom, pTop);

        // 2. Núcleo luminoso resonante que pulsa y se ensancha con el sonido
        if (h >= 1.0) {
            QPointF rTop = (m_originAxis == AxisX)
                ? projectPoint(cMid, m_axisOffset + h, 0.0, center)
                : projectPoint(m_axisOffset + h, cMid, 0.0, center);
            QPointF rBottom = (m_originAxis == AxisX)
                ? projectPoint(cMid, m_axisOffset - h, 0.0, center)
                : projectPoint(m_axisOffset - h, cMid, 0.0, center);

            QLinearGradient grad(rBottom, rTop);
            QColor fadeCol = col; fadeCol.setAlphaF(0.0);
            QColor tipCol = col; tipCol.setAlphaF(0.85 * mainAlpha);
            QColor hotCore = col.lighter(170); hotCore.setAlphaF(1.0 * mainAlpha);

            grad.setColorAt(0.0, fadeCol);
            grad.setColorAt(0.35, tipCol);
            grad.setColorAt(0.50, hotCore);
            grad.setColorAt(0.65, tipCol);
            grad.setColorAt(1.0, fadeCol);

            qreal glowThickness = std::max(2.0, m_barWidth * 0.35 * m_stringGlow);
            QPen glowPen(QBrush(grad), glowThickness, Qt::SolidLine, Qt::RoundCap);
            painter->setPen(glowPen);
            painter->drawLine(rBottom, rTop);
        }
    }
}

void CartesianVisualizer::drawVuGridMatrix(QPainter* painter, const QPointF& center,
                                           qreal startCoord, qreal step, int numBars,
                                           qreal maxReach, qreal mainAlpha)
{
    if (numBars <= 0 || m_currentSpectrum.isEmpty()) return;

    int M = std::clamp(m_gridRows, 4, 36);
    qreal rowStep = maxReach / static_cast<qreal>(M);
    qreal cellH = std::max(2.0, rowStep - m_segmentGap);
    const int maxRank = (m_frequencyLayout == CenterOut) ? std::max(1, (numBars - 1) / 2) : std::max(1, numBars - 1);
    bool isFlat2D = (std::abs(m_tiltPitch) < 0.1 && std::abs(m_tiltYaw) < 0.1);

    auto sampleNorm = [this, maxRank](int rankIdx) -> float {
        float t = std::clamp(static_cast<float>(rankIdx) / static_cast<float>(maxRank), 0.0f, 1.0f);
        float sIdx = t * static_cast<float>(m_currentSpectrum.size() - 1);
        int i0 = static_cast<int>(std::floor(sIdx));
        int i1 = std::min<int>(i0 + 1, static_cast<int>(m_currentSpectrum.size() - 1));
        float frac = sIdx - static_cast<float>(i0);
        float rawVal = m_currentSpectrum.at(i0) * (1.0f - frac) + m_currentSpectrum.at(i1) * frac;
        if (rawVal < m_minThreshold) return 0.0f;
        float norm = (rawVal - static_cast<float>(m_minThreshold)) /
                     std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
        return std::clamp(norm, 0.0f, 1.0f);
    };

    auto drawSingleMatrix = [&](qreal dirSign) {
        painter->setPen(Qt::NoPen);

        for (int i = 0; i < numBars; ++i) {
            int specIdx = i;
            qreal c1, c2;
            if (m_frequencyLayout == CenterOut) {
                int rank = i / 2;
                qreal offset = rank * step;
                if (i % 2 == 0) {
                    c1 = offset;
                    c2 = offset + m_barWidth;
                } else {
                    c2 = -offset;
                    c1 = -offset - m_barWidth;
                }
                specIdx = std::min(rank, maxRank);
            } else {
                c1 = startCoord + i * step;
                c2 = c1 + m_barWidth;
            }

            float normAmp = sampleNorm(specIdx);
            int activeRows = std::clamp<int>(static_cast<int>(std::round(normAmp * M)), 0, M);

            // 1. Trazado rápido de ranura inactiva de fondo por columna (1 solo draw call por columna)
            qreal trackH = dirSign * (M * rowStep);
            QColor slotColor = m_palette.sample(0.2f).darker(350);
            slotColor.setAlphaF(0.10 * mainAlpha);

            if (isFlat2D) {
                qreal barLeft  = std::min(c1, c2);
                qreal barWidth = std::abs(c2 - c1);
                if (m_originAxis == AxisX) {
                    qreal screenX = center.x() + barLeft;
                    qreal screenY0 = center.y() - m_axisOffset;
                    qreal screenY1 = center.y() - (m_axisOffset + trackH);
                    painter->fillRect(QRectF(screenX, std::min(screenY0, screenY1), barWidth, std::abs(screenY1 - screenY0)), slotColor);
                } else {
                    qreal screenX0 = center.x() + m_axisOffset;
                    qreal screenX1 = center.x() + (m_axisOffset + trackH);
                    qreal screenY = center.y() - (barLeft + barWidth);
                    painter->fillRect(QRectF(std::min(screenX0, screenX1), screenY, std::abs(screenX1 - screenX0), barWidth), slotColor);
                }
            }

            // 2. Dibujar ÚNICAMENTE las celdas encendidas / activas
            for (int r = 0; r < activeRows; ++r) {
                qreal h0 = dirSign * (r * rowStep);
                qreal h1 = h0 + dirSign * cellH;

                float rowT = static_cast<float>(r) / static_cast<float>(std::max(1, M - 1));
                QColor c = m_palette.sample(rowT).lighter(120);
                c.setAlphaF(mainAlpha);

                if (isFlat2D) {
                    qreal barLeft  = std::min(c1, c2);
                    qreal barWidth = std::abs(c2 - c1);
                    if (m_originAxis == AxisX) {
                        qreal screenX = center.x() + barLeft;
                        qreal screenY0 = center.y() - (m_axisOffset + h0);
                        qreal screenY1 = center.y() - (m_axisOffset + h1);
                        painter->fillRect(QRectF(screenX, std::min(screenY0, screenY1), barWidth, std::abs(screenY1 - screenY0)), c);
                    } else {
                        qreal screenX0 = center.x() + (m_axisOffset + h0);
                        qreal screenX1 = center.x() + (m_axisOffset + h1);
                        qreal screenY = center.y() - (barLeft + barWidth);
                        painter->fillRect(QRectF(std::min(screenX0, screenX1), screenY, std::abs(screenX1 - screenX0), barWidth), c);
                    }
                } else {
                    QPointF p0, p1, p2, p3;
                    if (m_originAxis == AxisX) {
                        p0 = projectPoint(c1, m_axisOffset + h0, 0.0, center);
                        p1 = projectPoint(c2, m_axisOffset + h0, 0.0, center);
                        p2 = projectPoint(c2, m_axisOffset + h1, 0.0, center);
                        p3 = projectPoint(c1, m_axisOffset + h1, 0.0, center);
                    } else {
                        p0 = projectPoint(m_axisOffset + h0, c1, 0.0, center);
                        p1 = projectPoint(m_axisOffset + h0, c2, 0.0, center);
                        p2 = projectPoint(m_axisOffset + h1, c2, 0.0, center);
                        p3 = projectPoint(m_axisOffset + h1, c1, 0.0, center);
                    }

                    const QPointF points[4] = {p0, p1, p2, p3};
                    painter->setBrush(c);
                    painter->drawConvexPolygon(points, 4);
                }
            }
        }
    };

    qreal initialSign = m_invertDirection ? -1.0 : 1.0;
    drawSingleMatrix(initialSign);
    if (m_mirrorEnabled) {
        drawSingleMatrix(-initialSign);
    }
}

void CartesianVisualizer::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    m_wavePhase += 0.035 * m_waveSpeed;
    if (m_wavePhase > 100000.0) m_wavePhase = 0.0;

    QPointF center(width() * 0.5, height() * 0.5);

    // Dibujar guías del plano cartesiano si están habilitadas
    if (m_showAxes) {
        drawAxes(painter, center);
    }

    // Dibujar línea del umbral si está habilitada
    if (m_showThresholdLine) {
        drawThresholdLine(painter, center);
    }

    int numBars = m_currentSpectrum.size();
    if (numBars <= 0) {
        return;
    }

    const qreal step = m_barWidth + m_barSpacing;
    qreal totalSpan = numBars * step - m_barSpacing;

    if (m_autoFillScreen && step > 0.5) {
        qreal available = (m_originAxis == AxisX) ? width() : height();
        numBars = std::max<int>(8, static_cast<int>(std::floor(available / step)));
        totalSpan = numBars * step - m_barSpacing;
    }

    const qreal startCoord = -totalSpan * 0.5;
    const int maxRank = (m_frequencyLayout == CenterOut) ? std::max(1, (numBars - 1) / 2) : std::max(1, numBars - 1);

    // Helper lambda para interpolar muestras del espectro
    auto sampleSpectrum = [this, maxRank](int rankIdx, float& outRaw, float& outRes) {
        float t = static_cast<float>(rankIdx) / static_cast<float>(maxRank);
        t = std::clamp(t, 0.0f, 1.0f);

        if (m_currentSpectrum.isEmpty()) {
            outRaw = 0.0f;
            outRes = 0.0f;
            return;
        }

        float sIdx = t * static_cast<float>(m_currentSpectrum.size() - 1);
        int i0 = static_cast<int>(std::floor(sIdx));
        int i1 = std::min<int>(i0 + 1, static_cast<int>(m_currentSpectrum.size() - 1));
        float frac = sIdx - static_cast<float>(i0);

        outRaw = m_currentSpectrum.at(i0) * (1.0f - frac) + m_currentSpectrum.at(i1) * frac;
        if (!m_residualSpectrum.empty()) {
            int r0 = std::min<int>(i0, static_cast<int>(m_residualSpectrum.size() - 1));
            int r1 = std::min<int>(i1, static_cast<int>(m_residualSpectrum.size() - 1));
            outRes = m_residualSpectrum[r0] * (1.0f - frac) + m_residualSpectrum[r1] * frac;
        } else {
            outRes = 0.0f;
        }
    };

    // Dibujar cada barra en dos pasadas: 1) Sombra residual, 2) Barra principal
    // Pasada 1: Sombra residual (si está activa)
    if (m_residualEnabled && m_residualOpacity > 0.01) {
        qreal shadowAlpha = m_residualOpacity * m_transparency;

        for (int i = 0; i < numBars; ++i) {
            qreal c1, c2;
            int specIdx = i;

            if (m_frequencyLayout == CenterOut) {
                int rank = i / 2;
                qreal offset = rank * step;
                if (i % 2 == 0) {
                    c1 = offset;
                    c2 = offset + m_barWidth;
                } else {
                    c2 = -offset;
                    c1 = -offset - m_barWidth;
                }
                specIdx = std::min(rank, maxRank);
            } else {
                c1 = startCoord + i * step;
                c2 = c1 + m_barWidth;
            }

            float rawDummy = 0.0f;
            float resVal = 0.0f;
            sampleSpectrum(specIdx, rawDummy, resVal);

            // Procesar umbrales para la sombra residual
            float effRes = 0.0f;
            if (resVal >= m_minThreshold) {
                float norm = (resVal - static_cast<float>(m_minThreshold)) /
                             std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
                effRes = std::clamp(norm, 0.0f, 1.0f);
            }

            qreal resHeight = effRes * effectiveMaxReach();
            if (m_invertDirection) {
                resHeight = -resHeight;
            }

            // Colores de la estela residual
            float t = static_cast<float>(specIdx) / static_cast<float>(maxRank);
            t = std::clamp(t, 0.0f, 1.0f);
            QColor baseColor, tipColor;
            if (m_colorMode == GradientAlongHeight) {
                baseColor = m_palette.sample(0.0f);
                tipColor  = m_palette.sample(1.0f);
            } else if (m_colorMode == GradientAcrossBars) {
                baseColor = m_palette.sample(t);
                tipColor  = m_palette.sample(t);
            } else { // Both
                baseColor = m_palette.sample(t * 0.5f);
                tipColor  = m_palette.sample(0.5f + t * 0.5f);
            }

            // Barra residual en dirección principal
            drawBarQuad(painter, center, c1, c2, resHeight, -m_residualOffset,
                        baseColor, tipColor, shadowAlpha);

            // Barra residual en modo espejo
            if (m_mirrorEnabled) {
                drawBarQuad(painter, center, c1, c2, -resHeight, -m_residualOffset,
                            baseColor, tipColor, shadowAlpha);
            }
        }
    }

    // Pasada 2: Renderizado reactivo según estilo visual
    qreal mainAlpha = m_transparency;

    // Si el estilo es HarmonicWaves, delegamos el renderizado completo al motor armónico multicapa
    if (m_visualStyle == HarmonicWaves) {
        qreal waveSpan = std::max(totalSpan, (m_originAxis == AxisX ? width() : height()));
        drawHarmonicWaves(painter, center, waveSpan, step, numBars, mainAlpha);
        return;
    }

    if (m_visualStyle == ZigZagWave) {
        drawZigZagWave(painter, center, startCoord, step, numBars, effectiveMaxReach(), mainAlpha);
        return;
    }

    if (m_visualStyle == SilkStrings) {
        drawSilkStrings(painter, center, startCoord, step, numBars, effectiveMaxReach(), mainAlpha);
        return;
    }

    if (m_visualStyle == VuGridMatrix) {
        drawVuGridMatrix(painter, center, startCoord, step, numBars, effectiveMaxReach(), mainAlpha);
        return;
    }

    for (int i = 0; i < numBars; ++i) {
        qreal c1, c2;
        int specIdx = i;

        if (m_frequencyLayout == CenterOut) {
            int rank = i / 2;
            qreal offset = rank * step;
            if (i % 2 == 0) {
                c1 = offset;
                c2 = offset + m_barWidth;
            } else {
                c2 = -offset;
                c1 = -offset - m_barWidth;
            }
            specIdx = std::min(rank, maxRank);
        } else {
            c1 = startCoord + i * step;
            c2 = c1 + m_barWidth;
        }

        float rawVal = 0.0f;
        float resDummy = 0.0f;
        sampleSpectrum(specIdx, rawVal, resDummy);

        // Procesar umbrales (mínimo de ruido y techo máximo)
        float effVal = 0.0f;
        if (rawVal >= m_minThreshold) {
            float norm = (rawVal - static_cast<float>(m_minThreshold)) /
                         std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
            effVal = std::clamp(norm, 0.0f, 1.0f);
        }

        qreal height = effVal * effectiveMaxReach();
        if (m_invertDirection) {
            height = -height;
        }

        float t = static_cast<float>(specIdx) / static_cast<float>(maxRank);
        t = std::clamp(t, 0.0f, 1.0f);
        QColor baseColor, tipColor;
        if (m_colorMode == GradientAlongHeight) {
            baseColor = m_palette.sample(0.0f);
            tipColor  = m_palette.sample(1.0f);
        } else if (m_colorMode == GradientAcrossBars) {
            baseColor = m_palette.sample(t);
            tipColor  = m_palette.sample(t);
        } else { // Both
            baseColor = m_palette.sample(t * 0.5f);
            tipColor  = m_palette.sample(0.5f + t * 0.5f);
        }

        // Picos para tapas flotantes
        float peakVal = rawVal;
        if (!m_currentPeaks.isEmpty()) {
            float sIdx = (static_cast<float>(specIdx) / static_cast<float>(maxRank)) * static_cast<float>(m_currentPeaks.size() - 1);
            int pIdx = std::clamp(static_cast<int>(std::round(sIdx)), 0, static_cast<int>(m_currentPeaks.size() - 1));
            peakVal = m_currentPeaks.at(pIdx);
        }

        float effPeak = 0.0f;
        if (peakVal >= m_minThreshold) {
            float norm = (peakVal - static_cast<float>(m_minThreshold)) /
                         std::max(0.001f, static_cast<float>(m_maxThreshold - m_minThreshold));
            effPeak = std::clamp(norm, 0.0f, 1.0f);
        }
        qreal peakHeight = effPeak * effectiveMaxReach();
        if (m_invertDirection) {
            peakHeight = -peakHeight;
        }

        if (m_visualStyle == SpikeWaveform) {
            // Estilo 1: Agujas / Puntas dentadas de frecuencia
            drawSpike(painter, center, c1, c2, height, 0.0, baseColor, tipColor, mainAlpha);
            if (m_mirrorEnabled) {
                drawSpike(painter, center, c1, c2, -height, 0.0, baseColor, tipColor, mainAlpha);
            }
        } else if (m_visualStyle == FloatingPeakCaps) {
            // Estilo 2: Barras con tapas flotantes desacopladas
            qreal capH = (height >= 0)
                ? std::max(height + m_peakCapGap, peakHeight)
                : std::min(height - m_peakCapGap, peakHeight);

            drawBarQuad(painter, center, c1, c2, height, 0.0, baseColor, tipColor, mainAlpha);
            if (effPeak >= 0.005f || std::abs(height) >= 1.0) {
                drawPeakCap(painter, center, c1, c2, capH, 0.0, tipColor, mainAlpha);
            }
            if (m_mirrorEnabled) {
                drawBarQuad(painter, center, c1, c2, -height, 0.0, baseColor, tipColor, mainAlpha);
                if (effPeak >= 0.005f || std::abs(height) >= 1.0) {
                    drawPeakCap(painter, center, c1, c2, -capH, 0.0, tipColor, mainAlpha);
                }
            }
        } else if (m_visualStyle == SegmentedBlocks) {
            // Bloques LED segmentados retro
            drawSegmentedBar(painter, center, c1, c2, height, 0.0, baseColor, tipColor, mainAlpha);
            if (m_showPeakCaps && (effPeak >= 0.005f || std::abs(height) >= 1.0)) {
                drawPeakCap(painter, center, c1, c2, peakHeight, 0.0, tipColor, mainAlpha);
            }
            if (m_mirrorEnabled) {
                drawSegmentedBar(painter, center, c1, c2, -height, 0.0, baseColor, tipColor, mainAlpha);
                if (m_showPeakCaps && (effPeak >= 0.005f || std::abs(height) >= 1.0)) {
                    drawPeakCap(painter, center, c1, c2, -peakHeight, 0.0, tipColor, mainAlpha);
                }
            }
        } else if (m_visualStyle == DotMatrix) {
            // Matriz de puntos / cuentas redondas espectrales
            drawDotColumn(painter, center, c1, c2, height, 0.0, baseColor, tipColor, mainAlpha);
            if (m_mirrorEnabled) {
                drawDotColumn(painter, center, c1, c2, -height, 0.0, baseColor, tipColor, mainAlpha);
            }
        } else {
            // SolidBars: Barras clásicas
            drawBarQuad(painter, center, c1, c2, height, 0.0, baseColor, tipColor, mainAlpha);
            if (m_showPeakCaps && (effPeak >= 0.005f || std::abs(height) >= 1.0)) {
                drawPeakCap(painter, center, c1, c2, peakHeight, 0.0, tipColor, mainAlpha);
            }
            if (m_mirrorEnabled) {
                drawBarQuad(painter, center, c1, c2, -height, 0.0, baseColor, tipColor, mainAlpha);
                if (m_showPeakCaps && (effPeak >= 0.005f || std::abs(height) >= 1.0)) {
                    drawPeakCap(painter, center, c1, c2, -peakHeight, 0.0, tipColor, mainAlpha);
                }
            }
        }
    }
}

// Setters con emisión de señales
void CartesianVisualizer::setOriginAxis(OriginAxis axis) {
    if (m_originAxis != axis) {
        m_originAxis = axis;
        emit originAxisChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setFrequencyLayout(FrequencyLayout layout) {
    if (m_frequencyLayout != layout) {
        m_frequencyLayout = layout;
        emit frequencyLayoutChanged();
        update();
    }
}

void CartesianVisualizer::setMirrorEnabled(bool enabled) {
    if (m_mirrorEnabled != enabled) {
        m_mirrorEnabled = enabled;
        emit mirrorEnabledChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setInvertDirection(bool inverted) {
    if (m_invertDirection != inverted) {
        m_invertDirection = inverted;
        emit invertDirectionChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setShowAxes(bool show) {
    if (m_showAxes != show) {
        m_showAxes = show;
        emit showAxesChanged();
        update();
    }
}

void CartesianVisualizer::setAxisColor(const QColor& color) {
    if (m_axisColor != color) {
        m_axisColor = color;
        emit axisColorChanged();
        update();
    }
}

void CartesianVisualizer::setBarWidth(qreal width) {
    if (width > 0.5 && m_barWidth != width) {
        m_barWidth = width;
        emit barWidthChanged();
        update();
    }
}

void CartesianVisualizer::setBarSpacing(qreal spacing) {
    if (spacing >= 0.0 && m_barSpacing != spacing) {
        m_barSpacing = spacing;
        emit barSpacingChanged();
        update();
    }
}

void CartesianVisualizer::setMaxBarLength(qreal length) {
    if (length > 10.0 && m_maxBarLength != length) {
        m_maxBarLength = length;
        emit maxBarLengthChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setBarRadius(qreal radius) {
    if (radius >= 0.0 && m_barRadius != radius) {
        m_barRadius = radius;
        emit barRadiusChanged();
        update();
    }
}

void CartesianVisualizer::setTiltPitch(qreal pitch) {
    if (m_tiltPitch != pitch) {
        m_tiltPitch = pitch;
        emit tiltPitchChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setTiltYaw(qreal yaw) {
    if (m_tiltYaw != yaw) {
        m_tiltYaw = yaw;
        emit tiltYawChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setPerspectiveDepth(qreal depth) {
    if (depth > 50.0 && m_perspectiveDepth != depth) {
        m_perspectiveDepth = depth;
        emit perspectiveDepthChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setResidualEnabled(bool enabled) {
    if (m_residualEnabled != enabled) {
        m_residualEnabled = enabled;
        emit residualEnabledChanged();
        update();
    }
}

void CartesianVisualizer::setResidualDecay(qreal decay) {
    if (decay >= 0.1 && decay <= 0.999 && m_residualDecay != decay) {
        m_residualDecay = decay;
        emit residualDecayChanged();
    }
}

void CartesianVisualizer::setResidualOpacity(qreal opacity) {
    if (opacity >= 0.0 && opacity <= 1.0 && m_residualOpacity != opacity) {
        m_residualOpacity = opacity;
        emit residualOpacityChanged();
        update();
    }
}

void CartesianVisualizer::setResidualOffset(qreal offset) {
    if (m_residualOffset != offset) {
        m_residualOffset = offset;
        emit residualOffsetChanged();
        update();
    }
}

void CartesianVisualizer::setTransparency(qreal transp) {
    if (transp >= 0.0 && transp <= 1.0 && m_transparency != transp) {
        m_transparency = transp;
        emit transparencyChanged();
        update();
    }
}

void CartesianVisualizer::setColorMode(ColorMode mode) {
    if (m_colorMode != mode) {
        m_colorMode = mode;
        emit colorModeChanged();
        update();
    }
}

void CartesianVisualizer::setAxisOffset(qreal offset) {
    if (m_axisOffset != offset) {
        m_axisOffset = offset;
        emit axisOffsetChanged();
        emit effectiveMaxReachChanged();
        update();
    }
}

void CartesianVisualizer::setMinThreshold(qreal thresh) {
    thresh = std::clamp(thresh, 0.0, 0.8);
    if (m_minThreshold != thresh) {
        m_minThreshold = thresh;
        emit minThresholdChanged();
        update();
    }
}

void CartesianVisualizer::setMaxThreshold(qreal thresh) {
    thresh = std::clamp(thresh, 0.1, 1.0);
    if (m_maxThreshold != thresh) {
        m_maxThreshold = thresh;
        emit maxThresholdChanged();
        update();
    }
}

void CartesianVisualizer::setShowThresholdLine(bool show) {
    if (m_showThresholdLine != show) {
        m_showThresholdLine = show;
        emit showThresholdLineChanged();
        update();
    }
}

void CartesianVisualizer::setThresholdLineColor(const QColor& color) {
    if (m_thresholdLineColor != color) {
        m_thresholdLineColor = color;
        emit thresholdLineColorChanged();
        update();
    }
}

void CartesianVisualizer::setAutoFillScreen(bool fill) {
    if (m_autoFillScreen != fill) {
        m_autoFillScreen = fill;
        emit autoFillScreenChanged();
        update();
    }
}

void CartesianVisualizer::setVisualStyle(VisualStyle style) {
    if (m_visualStyle != style) {
        m_visualStyle = style;
        emit visualStyleChanged();
        update();
    }
}

void CartesianVisualizer::setSegmentHeight(qreal h) {
    if (h >= 1.0 && h <= 50.0 && m_segmentHeight != h) {
        m_segmentHeight = h;
        emit segmentHeightChanged();
        update();
    }
}

void CartesianVisualizer::setSegmentGap(qreal g) {
    if (g >= 0.0 && g <= 20.0 && m_segmentGap != g) {
        m_segmentGap = g;
        emit segmentGapChanged();
        update();
    }
}

void CartesianVisualizer::setShowPeakCaps(bool show) {
    if (m_showPeakCaps != show) {
        m_showPeakCaps = show;
        emit showPeakCapsChanged();
        update();
    }
}

void CartesianVisualizer::setPeakCapGap(qreal gap) {
    if (gap >= 0.0 && gap <= 50.0 && m_peakCapGap != gap) {
        m_peakCapGap = gap;
        emit peakCapGapChanged();
        update();
    }
}

void CartesianVisualizer::setPeakCapHeight(qreal h) {
    if (h >= 1.0 && h <= 30.0 && m_peakCapHeight != h) {
        m_peakCapHeight = h;
        emit peakCapHeightChanged();
        update();
    }
}

void CartesianVisualizer::setWaveLayers(int layers) {
    layers = std::clamp(layers, 1, 10);
    if (m_waveLayers != layers) {
        m_waveLayers = layers;
        emit waveLayersChanged();
        update();
    }
}

void CartesianVisualizer::setWaveSpeed(qreal speed) {
    if (speed >= 0.0 && speed <= 10.0 && m_waveSpeed != speed) {
        m_waveSpeed = speed;
        emit waveSpeedChanged();
        update();
    }
}

void CartesianVisualizer::setWaveSpread(qreal spread) {
    if (spread >= 0.1 && spread <= 10.0 && m_waveSpread != spread) {
        m_waveSpread = spread;
        emit waveSpreadChanged();
        update();
    }
}

void CartesianVisualizer::setDotSize(qreal size) {
    size = std::clamp(size, 1.0, 40.0);
    if (m_dotSize != size) {
        m_dotSize = size;
        emit dotSizeChanged();
        update();
    }
}

void CartesianVisualizer::setDotGap(qreal gap) {
    gap = std::clamp(gap, 0.0, 30.0);
    if (m_dotGap != gap) {
        m_dotGap = gap;
        emit dotGapChanged();
        update();
    }
}

void CartesianVisualizer::setGridRows(int rows) {
    rows = std::clamp(rows, 4, 48);
    if (m_gridRows != rows) {
        m_gridRows = rows;
        emit gridRowsChanged();
        update();
    }
}

void CartesianVisualizer::setStringGlow(qreal glow) {
    glow = std::clamp(glow, 0.1, 5.0);
    if (m_stringGlow != glow) {
        m_stringGlow = glow;
        emit stringGlowChanged();
        update();
    }
}

} // namespace AudioViewer
