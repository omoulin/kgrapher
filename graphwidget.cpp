#include "graphwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QToolTip>
#include <QFont>
#include <QtMath>
#include <QtGlobal>
#include <cmath>

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void GraphWidget::setSamples(const QVector<QPointF> &samples)
{
    m_samples = samples;
    m_hasSamples = !samples.isEmpty();
    m_hasClickedPoint = false;
    if (m_autoYRange && m_hasSamples) {
        m_yMin = m_samples.first().y();
        m_yMax = m_yMin;
        for (const QPointF &pt : m_samples) {
            if (std::isfinite(pt.y())) {
                m_yMin = qMin(m_yMin, pt.y());
                m_yMax = qMax(m_yMax, pt.y());
            }
        }
        double margin = (m_yMax - m_yMin) * 0.05 + 0.1;
        if (m_yMax - m_yMin < 0.01) margin = 1;
        m_yMin -= margin;
        m_yMax += margin;
    }
    update();
}

void GraphWidget::setXRange(double xMin, double xMax)
{
    m_xMin = xMin;
    m_xMax = xMax;
    update();
}

void GraphWidget::setYRange(double yMin, double yMax)
{
    m_autoYRange = false;
    m_yMin = yMin;
    m_yMax = yMax;
    update();
}

void GraphWidget::setCurveColor(const QColor &c)
{
    if (c.isValid()) {
        m_curveColor = c;
        update();
    }
}

void GraphWidget::clear()
{
    m_samples.clear();
    m_hasSamples = false;
    m_hasClickedPoint = false;
    m_autoYRange = true;
    m_yMin = -3;
    m_yMax = 3;
    update();
}

QPointF GraphWidget::mapToWidget(double x, double y) const
{
    const double margin = MARGIN;
    double w = width() - 2 * margin;
    double h = height() - 2 * margin;
    if (w <= 0 || h <= 0) return QPointF(margin, margin);

    double xRange = m_xMax - m_xMin;
    double yRange = m_yMax - m_yMin;
    if (qAbs(xRange) < 1e-30) xRange = 1e-30;
    if (qAbs(yRange) < 1e-30) yRange = 1e-30;

    double sx = margin + (x - m_xMin) / xRange * w;
    double sy = margin + (m_yMax - y) / yRange * h; // y flipped for screen coords
    return QPointF(sx, sy);
}

QPointF GraphWidget::mapFromWidget(QPointF screenPos) const
{
    const double margin = MARGIN;
    double w = width() - 2 * margin;
    double h = height() - 2 * margin;
    if (w <= 0 || h <= 0) return QPointF(m_xMin, m_yMin);

    double xRange = m_xMax - m_xMin;
    double yRange = m_yMax - m_yMin;
    if (qAbs(xRange) < 1e-30) xRange = 1e-30;
    if (qAbs(yRange) < 1e-30) yRange = 1e-30;

    double x = m_xMin + (screenPos.x() - margin) / w * xRange;
    double y = m_yMax - (screenPos.y() - margin) / h * yRange;
    return QPointF(x, y);
}

double GraphWidget::valueAtX(double x) const
{
    if (m_samples.size() < 2) return qQNaN();
    if (x <= m_samples.first().x()) return m_samples.first().y();
    if (x >= m_samples.last().x()) return m_samples.last().y();
    for (int i = 0; i < m_samples.size() - 1; ++i) {
        if (m_samples[i].x() <= x && x <= m_samples[i + 1].x()) {
            const QPointF &a = m_samples[i];
            const QPointF &b = m_samples[i + 1];
            double t = (b.x() - a.x()) > 1e-30 ? (x - a.x()) / (b.x() - a.x()) : 0;
            return a.y() + t * (b.y() - a.y());
        }
    }
    return qQNaN();
}

void GraphWidget::drawClickedPoint(QPainter &p) const
{
    if (!m_hasClickedPoint) return;
    double drawY = std::isfinite(m_clickedCurveY) ? m_clickedCurveY : m_clickedDataPoint.y();
    QPointF screen = mapToWidget(m_clickedDataPoint.x(), drawY);
    p.setPen(QPen(Qt::darkRed, 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(screen.toPoint(), 6, 6);
    p.drawLine(screen.toPoint() + QPoint(-10, 0), screen.toPoint() + QPoint(10, 0));
    p.drawLine(screen.toPoint() + QPoint(0, -10), screen.toPoint() + QPoint(0, 10));
}

QVector<double> GraphWidget::tickValues(double minVal, double maxVal, int maxTicks) const
{
    QVector<double> ticks;
    double range = maxVal - minVal;
    if (range <= 0) return ticks;
    double step = range / qMax(1, maxTicks - 1);
    if (step <= 0) return ticks;
    double magnitude = std::pow(10, std::floor(std::log10(step + 1e-30)));
    if (magnitude < 1e-30) magnitude = 1e-30;
    double norm = step / magnitude;
    if (norm <= 1.0) step = magnitude;
    else if (norm <= 2.0) step = 2 * magnitude;
    else if (norm <= 5.0) step = 5 * magnitude;
    else step = 10 * magnitude;
    double start = std::ceil(minVal / step) * step;
    for (double v = start; v <= maxVal + step * 0.001; v += step)
        ticks.append(v);
    if (ticks.isEmpty()) ticks.append(minVal);
    return ticks;
}

void GraphWidget::drawGrid(QPainter &p) const
{
    double w = width() - 2 * MARGIN;
    double h = height() - 2 * MARGIN;
    if (w <= 0 || h <= 0) return;

    QVector<double> xTicks = tickValues(m_xMin, m_xMax, 8);
    QVector<double> yTicks = tickValues(m_yMin, m_yMax, 8);

    p.setPen(QPen(palette().color(QPalette::Mid), 0.5, Qt::DotLine));
    for (double x : xTicks) {
        if (x <= m_xMin || x >= m_xMax) continue;
        QPointF a = mapToWidget(x, m_yMin);
        QPointF b = mapToWidget(x, m_yMax);
        p.drawLine(a.toPoint(), b.toPoint());
    }
    for (double y : yTicks) {
        if (y <= m_yMin || y >= m_yMax) continue;
        QPointF a = mapToWidget(m_xMin, y);
        QPointF b = mapToWidget(m_xMax, y);
        p.drawLine(a.toPoint(), b.toPoint());
    }
}

void GraphWidget::drawAxes(QPainter &p) const
{
    p.setPen(QPen(palette().color(QPalette::WindowText), 1));
    if (m_xMin <= 0 && 0 <= m_xMax) {
        QPointF a = mapToWidget(0, m_yMin);
        QPointF b = mapToWidget(0, m_yMax);
        p.drawLine(a.toPoint(), b.toPoint());
    }
    if (m_yMin <= 0 && 0 <= m_yMax) {
        QPointF a = mapToWidget(m_xMin, 0);
        QPointF b = mapToWidget(m_xMax, 0);
        p.drawLine(a.toPoint(), b.toPoint());
    }
}

void GraphWidget::drawAxisLabels(QPainter &p) const
{
    QVector<double> xTicks = tickValues(m_xMin, m_xMax, 8);
    QVector<double> yTicks = tickValues(m_yMin, m_yMax, 8);

    p.setPen(palette().color(QPalette::WindowText));
    p.setFont(QFont(p.font().family(), 9));

    for (double x : xTicks) {
        QPointF pos = mapToWidget(x, m_yMin);
        QString label = QString::number(x, 'g', 4);
        QRectF r(pos.x() - 25, height() - MARGIN + 2, 50, 18);
        p.drawText(r, Qt::AlignHCenter | Qt::AlignTop, label);
    }
    for (double y : yTicks) {
        QPointF pos = mapToWidget(m_xMin, y);
        QString label = QString::number(y, 'g', 4);
        QRectF r(2, pos.y() - 9, MARGIN - 4, 18);
        p.drawText(r, Qt::AlignRight | Qt::AlignVCenter, label);
    }
}

void GraphWidget::drawCurve(QPainter &p) const
{
    if (m_samples.size() < 2) return;

    p.setPen(QPen(m_curveColor, 2));
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF prev = mapToWidget(m_samples.first().x(), m_samples.first().y());
    for (int i = 1; i < m_samples.size(); ++i) {
        const QPointF &pt = m_samples.at(i);
        if (!std::isfinite(pt.y())) {
            if (i + 1 < m_samples.size())
                prev = mapToWidget(m_samples.at(i + 1).x(), m_samples.at(i + 1).y());
            continue;
        }
        QPointF cur = mapToWidget(pt.x(), pt.y());
        p.drawLine(prev.toPoint(), cur.toPoint());
        prev = cur;
    }
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
    double factor = event->angleDelta().y() > 0 ? 0.85 : 1.0 / 0.85;
    zoomAtCenter(factor);
    event->accept();
}

void GraphWidget::zoomAtCenter(double factor)
{
    double xCenter = (m_xMin + m_xMax) / 2;
    double yCenter = (m_yMin + m_yMax) / 2;
    double xHalf = (m_xMax - m_xMin) / 2;
    double yHalf = (m_yMax - m_yMin) / 2;
    xHalf *= factor;
    yHalf *= factor;
    if (xHalf < 1e-10) xHalf = 1e-10;
    if (yHalf < 1e-10) yHalf = 1e-10;
    m_xMin = xCenter - xHalf;
    m_xMax = xCenter + xHalf;
    m_yMin = yCenter - yHalf;
    m_yMax = yCenter + yHalf;
    m_autoYRange = false;
    update();
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    QPointF screenPos = event->position().toPoint();
    const double margin = MARGIN;
    if (screenPos.x() < margin || screenPos.x() > width() - margin
        || screenPos.y() < margin || screenPos.y() > height() - margin) {
        m_hasClickedPoint = false;
        update();
        return;
    }
    m_clickedDataPoint = mapFromWidget(screenPos);
    m_clickedCurveY = valueAtX(m_clickedDataPoint.x());
    m_hasClickedPoint = true;

    QString msg;
    if (std::isfinite(m_clickedCurveY))
        msg = tr("x = %1, y = %2").arg(m_clickedDataPoint.x(), 0, 'g', 4).arg(m_clickedCurveY, 0, 'g', 4);
    else
        msg = tr("x = %1, y = %2 (off curve)").arg(m_clickedDataPoint.x(), 0, 'g', 4).arg(m_clickedDataPoint.y(), 0, 'g', 4);
    QToolTip::showText(event->globalPosition().toPoint(), msg, this, QRect(), 5000);
    update();
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Base));

    drawGrid(p);
    drawAxes(p);
    drawAxisLabels(p);
    drawCurve(p);
    drawClickedPoint(p);
}
