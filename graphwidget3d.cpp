#include "graphwidget3d.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <QtMath>
#include <cmath>
#include <algorithm>
#include <limits>

GraphWidget3D::GraphWidget3D(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
}

void GraphWidget3D::setSurface(const QVector<QVector<Point3D>> &grid)
{
    m_grid = grid;
    m_hasSurface = !grid.isEmpty() && !grid.first().isEmpty();
    m_zoomFactor = 1.8;
    m_hasClickedPoint = false;
    if (m_autoZRange && m_hasSurface) {
        m_zMin = m_grid.first().first().z;
        m_zMax = m_zMin;
        for (const auto &row : m_grid) {
            for (const Point3D &p : row) {
                if (std::isfinite(p.z)) {
                    m_zMin = qMin(m_zMin, p.z);
                    m_zMax = qMax(m_zMax, p.z);
                }
            }
        }
        double margin = (m_zMax - m_zMin) * 0.05 + 0.1;
        if (m_zMax - m_zMin < 0.01) margin = 1;
        m_zMin -= margin;
        m_zMax += margin;
    }
    update();
}

void GraphWidget3D::setXRange(double xMin, double xMax)
{
    m_xMin = xMin;
    m_xMax = xMax;
    update();
}

void GraphWidget3D::setYRange(double yMin, double yMax)
{
    m_yMin = yMin;
    m_yMax = yMax;
    update();
}

void GraphWidget3D::setZRange(double zMin, double zMax)
{
    m_autoZRange = false;
    m_zMin = zMin;
    m_zMax = zMax;
    update();
}

void GraphWidget3D::setSurfaceColor(const QColor &c)
{
    m_surfaceColor = c;
    update();
}

void GraphWidget3D::clear()
{
    m_grid.clear();
    m_hasSurface = false;
    m_hasClickedPoint = false;
    m_autoZRange = true;
    m_zMin = -5;
    m_zMax = 5;
    update();
}

QPointF GraphWidget3D::project(double x, double y, double z) const
{
    const double cx = width() / 2.0;
    const double cy = height() / 2.0;
    double range = qMax(qMax(m_xMax - m_xMin, m_yMax - m_yMin), m_zMax - m_zMin);
    if (range < 1e-30) range = 1.0;
    double scale = qMin(width(), height()) * 0.35 / range;
    scale *= m_zoomFactor;

    double ca = std::cos(m_azimuth), sa = std::sin(m_azimuth);
    double ce = std::cos(m_elevation), se = std::sin(m_elevation);
    double x1 = x * ca + y * sa;
    double y1 = -x * sa + y * ca;
    double z1 = z;
    double y2 = y1 * ce + z1 * se;
    double z2 = -y1 * se + z1 * ce;

    double sx = cx + (x1 * scale);
    double sy = cy - (y2 * scale);
    return QPointF(sx, sy);
}

double GraphWidget3D::projectDepth(double x, double y, double z) const
{
    double ca = std::cos(m_azimuth), sa = std::sin(m_azimuth);
    double ce = std::cos(m_elevation), se = std::sin(m_elevation);
    double x1 = x * ca + y * sa;
    double y1 = -x * sa + y * ca;
    double z1 = z;
    return -y1 * std::sin(m_elevation) + z1 * std::cos(m_elevation);
}

QVector<double> GraphWidget3D::tickValues(double minVal, double maxVal, int maxTicks) const
{
    QVector<double> ticks;
    double range = maxVal - minVal;
    if (range <= 0) return ticks;
    double step = range / qMax(1, maxTicks - 1);
    double magnitude = std::pow(10, std::floor(std::log10(std::abs(step) + 1e-30)));
    if (magnitude < 1e-30) magnitude = 1;
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

void GraphWidget3D::drawAxes3D(QPainter &p) const
{
    const double tickScreenLen = 6;
    const int maxTicks = 7;

    QVector<double> xTicks = tickValues(m_xMin, m_xMax, maxTicks);
    QVector<double> yTicks = tickValues(m_yMin, m_yMax, maxTicks);
    QVector<double> zTicks = tickValues(m_zMin, m_zMax, maxTicks);

    QPointF ox = project(0, 0, 0);
    QPointF xMinP = project(m_xMin, 0, 0);
    QPointF xMaxP = project(m_xMax, 0, 0);
    QPointF yMinP = project(0, m_yMin, 0);
    QPointF yMaxP = project(0, m_yMax, 0);
    QPointF zMinP = project(0, 0, m_zMin);
    QPointF zMaxP = project(0, 0, m_zMax);

    QPointF xAxisDir = xMaxP - xMinP;
    double xLen = std::sqrt(xAxisDir.x() * xAxisDir.x() + xAxisDir.y() * xAxisDir.y());
    if (xLen < 1e-6) xLen = 1;
    QPointF xPerp(-xAxisDir.y() / xLen, xAxisDir.x() / xLen);

    QPointF yAxisDir = yMaxP - yMinP;
    double yLen = std::sqrt(yAxisDir.x() * yAxisDir.x() + yAxisDir.y() * yAxisDir.y());
    if (yLen < 1e-6) yLen = 1;
    QPointF yPerp(-yAxisDir.y() / yLen, yAxisDir.x() / yLen);

    QPointF zAxisDir = zMaxP - zMinP;
    double zLen = std::sqrt(zAxisDir.x() * zAxisDir.x() + zAxisDir.y() * zAxisDir.y());
    if (zLen < 1e-6) zLen = 1;
    QPointF zPerp(-zAxisDir.y() / zLen, zAxisDir.x() / zLen);

    p.setPen(QPen(palette().color(QPalette::WindowText), 1.5));
    p.setBrush(Qt::NoBrush);
    p.setRenderHint(QPainter::Antialiasing, true);

    auto drawAxisLine = [&](const QPointF &a, const QPointF &b) {
        p.drawLine(a.toPoint(), b.toPoint());
    };
    auto drawTickAndLabel = [&](const QPointF &screenPos, const QPointF &perpDir, const QString &label) {
        QPointF p1 = screenPos + perpDir * tickScreenLen;
        QPointF p2 = screenPos - perpDir * tickScreenLen;
        p.drawLine(p1.toPoint(), p2.toPoint());
        if (!label.isEmpty()) {
            QRectF textRect(screenPos.x() + perpDir.x() * (tickScreenLen + 2) - 12,
                            screenPos.y() + perpDir.y() * (tickScreenLen + 2) - 8, 24, 16);
            p.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, label);
        }
    };

    auto isZero = [](double v) { return qAbs(v) < 1e-12; };
    bool originVisible = (m_xMin <= 0 && 0 <= m_xMax) && (m_yMin <= 0 && 0 <= m_yMax) && (m_zMin <= 0 && 0 <= m_zMax);

    drawAxisLine(xMinP, xMaxP);
    for (double t : xTicks)
        drawTickAndLabel(project(t, 0, 0), xPerp, (isZero(t) && originVisible) ? QString() : QString::number(t, 'g', 3));
    QPointF xEnd = xMaxP + QPointF(xAxisDir.x() / xLen * 14, xAxisDir.y() / xLen * 14);
    p.drawText(QRectF(xEnd.x() - 8, xEnd.y() - 8, 16, 16), Qt::AlignCenter, tr("x"));

    drawAxisLine(yMinP, yMaxP);
    for (double t : yTicks)
        drawTickAndLabel(project(0, t, 0), yPerp, (isZero(t) && originVisible) ? QString() : QString::number(t, 'g', 3));
    QPointF yEnd = yMaxP + QPointF(yAxisDir.x() / yLen * 14, yAxisDir.y() / yLen * 14);
    p.drawText(QRectF(yEnd.x() - 8, yEnd.y() - 8, 16, 16), Qt::AlignCenter, tr("y"));

    drawAxisLine(zMinP, zMaxP);
    for (double t : zTicks)
        drawTickAndLabel(project(0, 0, t), zPerp, (isZero(t) && originVisible) ? QString() : QString::number(t, 'g', 3));
    QPointF zEnd = zMaxP + QPointF(zAxisDir.x() / zLen * 14, zAxisDir.y() / zLen * 14);
    p.drawText(QRectF(zEnd.x() - 8, zEnd.y() - 8, 16, 16), Qt::AlignCenter, tr("z"));

    if (originVisible) {
        p.setPen(QPen(palette().color(QPalette::WindowText), 2));
        p.setBrush(palette().color(QPalette::WindowText));
        p.drawEllipse(ox.toPoint(), 3, 3);
        p.setPen(palette().color(QPalette::WindowText));
        p.drawText(QRectF(ox.x() - 10, ox.y() - 18, 20, 16), Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("0"));
    }
}

QColor GraphWidget3D::colorForZ(double z) const
{
    double t = (m_zMax - m_zMin) > 0 ? (z - m_zMin) / (m_zMax - m_zMin) : 0;
    t = qBound(0.0, t, 1.0);
    int r = static_cast<int>(70 + t * 150);
    int g = static_cast<int>(130 + (1 - t) * 80);
    int b = static_cast<int>(180);
    return QColor(r, g, b);
}

QColor GraphWidget3D::colorForZWithBase(double z) const
{
    if (!m_surfaceColor.isValid())
        return colorForZ(z);
    double t = (m_zMax - m_zMin) > 0 ? (z - m_zMin) / (m_zMax - m_zMin) : 0;
    t = qBound(0.0, t, 1.0);
    int h, s, l;
    m_surfaceColor.getHsl(&h, &s, &l);
    const int lLow = 45;
    const int lHigh = 220;
    int lNew = static_cast<int>(lLow + t * (lHigh - lLow));
    lNew = qBound(0, lNew, 255);
    return QColor::fromHsl(h, s, lNew);
}

void GraphWidget3D::drawSurface(QPainter &p) const
{
    if (!m_hasSurface || m_grid.size() < 2 || m_grid.first().size() < 2)
        return;

    struct Quad {
        QPolygonF screen;
        double depth;
        QColor color;
    };
    QVector<Quad> quads;

    for (int i = 0; i < m_grid.size() - 1; ++i) {
        for (int j = 0; j < m_grid[i].size() - 1; ++j) {
            const Point3D &p00 = m_grid[i][j];
            const Point3D &p10 = m_grid[i + 1][j];
            const Point3D &p11 = m_grid[i + 1][j + 1];
            const Point3D &p01 = m_grid[i][j + 1];
            if (!std::isfinite(p00.z) || !std::isfinite(p10.z) || !std::isfinite(p11.z) || !std::isfinite(p01.z))
                continue;
            double depth = (projectDepth(p00.x, p00.y, p00.z) + projectDepth(p10.x, p10.y, p10.z)
                         + projectDepth(p11.x, p11.y, p11.z) + projectDepth(p01.x, p01.y, p01.z)) / 4;
            double zMid = (p00.z + p10.z + p11.z + p01.z) / 4;
            QColor quadColor = colorForZWithBase(zMid);
            QPointF a = project(p00.x, p00.y, p00.z);
            QPointF b = project(p10.x, p10.y, p10.z);
            QPointF c = project(p11.x, p11.y, p11.z);
            QPointF d = project(p01.x, p01.y, p01.z);
            QPolygonF poly;
            poly << a << b << c << d;
            quads.append({ poly, depth, quadColor });
        }
    }

    std::sort(quads.begin(), quads.end(), [](const Quad &a, const Quad &b) { return a.depth < b.depth; });

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (const Quad &q : quads) {
        p.setBrush(q.color);
        p.setPen(QPen(palette().color(QPalette::Mid), 0.5));
        p.drawPolygon(q.screen);
    }
}

void GraphWidget3D::drawWireframe(QPainter &p) const
{
    if (!m_hasSurface || m_grid.size() < 2 || m_grid.first().size() < 2)
        return;

    p.setPen(QPen(palette().color(QPalette::WindowText), 0.8));
    p.setBrush(Qt::NoBrush);

    for (int i = 0; i < m_grid.size(); ++i) {
        for (int j = 0; j < m_grid[i].size() - 1; ++j) {
            const Point3D &a = m_grid[i][j];
            const Point3D &b = m_grid[i][j + 1];
            if (std::isfinite(a.z) && std::isfinite(b.z))
                p.drawLine(project(a.x, a.y, a.z).toPoint(), project(b.x, b.y, b.z).toPoint());
        }
    }
    for (int j = 0; j < m_grid.first().size(); ++j) {
        for (int i = 0; i < m_grid.size() - 1; ++i) {
            const Point3D &a = m_grid[i][j];
            const Point3D &b = m_grid[i + 1][j];
            if (std::isfinite(a.z) && std::isfinite(b.z))
                p.drawLine(project(a.x, a.y, a.z).toPoint(), project(b.x, b.y, b.z).toPoint());
        }
    }
}

Point3D GraphWidget3D::pointAtScreen(QPoint screenPos) const
{
    if (!m_hasSurface || m_grid.isEmpty()) return Point3D();

    Point3D best;
    double bestDist = std::numeric_limits<double>::max();

    for (const auto &row : m_grid) {
        for (const Point3D &pt : row) {
            if (!std::isfinite(pt.z)) continue;
            QPointF proj = project(pt.x, pt.y, pt.z);
            double dx = proj.x() - screenPos.x();
            double dy = proj.y() - screenPos.y();
            double d = dx * dx + dy * dy;
            if (d < bestDist) {
                bestDist = d;
                best = pt;
            }
        }
    }
    return best;
}

void GraphWidget3D::drawClickedPoint(QPainter &p) const
{
    if (!m_hasClickedPoint || !m_hasSurface) return;
    QPointF screen = project(m_clickedPoint3D.x, m_clickedPoint3D.y, m_clickedPoint3D.z);
    p.setPen(QPen(Qt::darkRed, 2));
    p.setBrush(QColor(255, 200, 200, 180));
    p.drawEllipse(screen.toPoint(), 8, 8);
}

void GraphWidget3D::drawAxisLabels(QPainter &p) const
{
    if (!m_hasSurface) return;

    p.setPen(palette().color(QPalette::WindowText));
    QFont f = p.font();
    f.setPointSize(9);
    p.setFont(f);

    QString xRange = QStringLiteral("x: %1 … %2").arg(m_xMin, 0, 'g', 3).arg(m_xMax, 0, 'g', 3);
    QString yRange = QStringLiteral("y: %1 … %2").arg(m_yMin, 0, 'g', 3).arg(m_yMax, 0, 'g', 3);
    QString zRange = QStringLiteral("z: %1 … %2").arg(m_zMin, 0, 'g', 3).arg(m_zMax, 0, 'g', 3);

    const int pad = 8;
    QRect r(pad, pad, width() - 2 * pad, 48);
    p.fillRect(r, QColor(255, 255, 255, 220));
    p.drawRect(r);
    p.drawText(r.adjusted(4, 2, -4, -2), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
               xRange + QChar('\n') + yRange + QChar('\n') + zRange);
}

void GraphWidget3D::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Base));

    if (m_hasSurface) {
        drawSurface(p);
        drawWireframe(p);
        drawAxes3D(p);
        drawClickedPoint(p);
    } else {
        p.setPen(palette().color(QPalette::PlaceholderText));
        p.drawText(rect(), Qt::AlignCenter, tr("Enter z = f(x,y) and click Graph in 3D mode"));
    }
}

void GraphWidget3D::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastMouse = event->position().toPoint();
        if (m_hasSurface) {
            m_clickedPoint3D = pointAtScreen(m_lastMouse);
            m_hasClickedPoint = true;
            QString msg = tr("x = %1, y = %2, z = %3")
                .arg(m_clickedPoint3D.x, 0, 'g', 4)
                .arg(m_clickedPoint3D.y, 0, 'g', 4)
                .arg(m_clickedPoint3D.z, 0, 'g', 4);
            QToolTip::showText(event->globalPosition().toPoint(), msg, this, QRect(), 5000);
        }
        update();
    }
}

void GraphWidget3D::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;
    QPoint delta = event->position().toPoint() - m_lastMouse;
    m_lastMouse = event->position().toPoint();
    m_azimuth += delta.x() * 0.01;
    m_elevation += delta.y() * 0.01;
    m_elevation = qBound(-1.4, m_elevation, 1.4);
    update();
}

void GraphWidget3D::wheelEvent(QWheelEvent *event)
{
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    m_zoomFactor *= factor;
    m_zoomFactor = qBound(0.1, m_zoomFactor, 20.0);
    update();
    event->accept();
}
