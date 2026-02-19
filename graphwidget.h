#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QColor>

class GraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);

    void setSamples(const QVector<QPointF> &samples);
    void setXRange(double xMin, double xMax);
    void setYRange(double yMin, double yMax);
    void setAutoYRange(bool autoY) { m_autoYRange = autoY; }
    void setCurveColor(const QColor &c);
    QColor curveColor() const { return m_curveColor; }
    void clear();

    QSize minimumSizeHint() const override { return QSize(400, 300); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void zoomAtCenter(double factor);
    QPointF mapFromWidget(QPointF screenPos) const;
    double valueAtX(double x) const;
    void drawClickedPoint(QPainter &p) const;

    QVector<QPointF> m_samples;
    double m_xMin = -3;
    double m_xMax = 3;
    double m_yMin = -3;
    double m_yMax = 3;
    bool m_autoYRange = true;
    bool m_hasSamples = false;
    bool m_hasClickedPoint = false;
    QPointF m_clickedDataPoint;
    double m_clickedCurveY = 0;
    QColor m_curveColor = QColor(0, 100, 200);

    QPointF mapToWidget(double x, double y) const;
    QVector<double> tickValues(double minVal, double maxVal, int maxTicks) const;
    void drawGrid(QPainter &p) const;
    void drawAxes(QPainter &p) const;
    void drawAxisLabels(QPainter &p) const;
    void drawCurve(QPainter &p) const;

    static const int MARGIN = 48;
};

#endif // GRAPHWIDGET_H
