#ifndef GRAPHWIDGET3D_H
#define GRAPHWIDGET3D_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QColor>

struct Point3D {
    double x = 0, y = 0, z = 0;
};

class GraphWidget3D : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWidget3D(QWidget *parent = nullptr);

    void setSurface(const QVector<QVector<Point3D>> &grid);
    void setXRange(double xMin, double xMax);
    void setYRange(double yMin, double yMax);
    void setZRange(double zMin, double zMax);
    void setAutoZRange(bool autoZ) { m_autoZRange = autoZ; }
    void setSurfaceColor(const QColor &c);
    QColor surfaceColor() const { return m_surfaceColor; }
    void clear();

    double azimuth() const { return m_azimuth; }
    double elevation() const { return m_elevation; }
    void setAzimuth(double a) { m_azimuth = a; update(); }
    void setElevation(double e) { m_elevation = e; update(); }

    QSize minimumSizeHint() const override { return QSize(400, 300); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QVector<QVector<Point3D>> m_grid;
    double m_xMin = -5, m_xMax = 5;
    double m_yMin = -5, m_yMax = 5;
    double m_zMin = -5, m_zMax = 5;
    bool m_autoZRange = true;
    bool m_hasSurface = false;

    double m_azimuth = 0.6;
    double m_elevation = 0.5;
    double m_zoomFactor = 1.8;
    QPoint m_lastMouse;
    QColor m_surfaceColor;

    QPointF project(double x, double y, double z) const;
    double projectDepth(double x, double y, double z) const;
    void drawSurface(QPainter &p) const;
    void drawWireframe(QPainter &p) const;
    void drawAxes3D(QPainter &p) const;
    void drawAxisLabels(QPainter &p) const;
    void drawClickedPoint(QPainter &p) const;
    QColor colorForZ(double z) const;
    QColor colorForZWithBase(double z) const;
    QVector<double> tickValues(double minVal, double maxVal, int maxTicks) const;
    Point3D pointAtScreen(QPoint screenPos) const;

    bool m_hasClickedPoint = false;
    Point3D m_clickedPoint3D;
};

#endif // GRAPHWIDGET3D_H
