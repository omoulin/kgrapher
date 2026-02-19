#include "mainwindow.h"
#include "graphwidget.h"
#include "graphwidget3d.h"
#include "expressionparser.h"
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QColorDialog>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_central = new QWidget(this);
    setCentralWidget(m_central);
    QVBoxLayout *layout = new QVBoxLayout(m_central);
    layout->setContentsMargins(4, 4, 4, 4);

    QHBoxLayout *topRow = new QHBoxLayout;
    m_equationEdit = new QLineEdit(this);
    m_equationEdit->setPlaceholderText(tr("2D: y = f(x) e.g. x^2, sin(x)"));
    m_equationEdit->setClearButtonEnabled(true);
    connect(m_equationEdit, &QLineEdit::textChanged, this, [this] { m_equationModified = true; });
    topRow->addWidget(m_equationEdit, 1);

    m_viewModeCombo = new QComboBox(this);
    m_viewModeCombo->addItem(tr("2D"));
    m_viewModeCombo->addItem(tr("3D"));
    m_viewModeCombo->setToolTip(tr("2D: y = f(x). 3D: z = f(x,y), drag to rotate."));
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onViewModeChanged);
    topRow->addWidget(m_viewModeCombo);

    QPushButton *graphBtn = new QPushButton(tr("&Graph"), this);
    graphBtn->setDefault(true);
    connect(graphBtn, &QPushButton::clicked, this, &MainWindow::drawGraph);
    topRow->addWidget(graphBtn);

    layout->addLayout(topRow);

    m_graphStack = new QStackedWidget(this);
    m_graphWidget = new GraphWidget(this);
    m_graphWidget->setXRange(-10, 10);
    m_graphStack->addWidget(m_graphWidget);
    m_graphWidget3D = new GraphWidget3D(this);
    m_graphWidget3D->setXRange(-5, 5);
    m_graphWidget3D->setYRange(-5, 5);
    m_graphStack->addWidget(m_graphWidget3D);

    QHBoxLayout *rangeRow = new QHBoxLayout;
    rangeRow->addWidget(new QLabel(tr("X:"), this));
    m_xMinSpin = new QDoubleSpinBox(this);
    m_xMinSpin->setRange(-1e6, 1e6);
    m_xMinSpin->setDecimals(2);
    m_xMinSpin->setValue(-3);
    m_xMinSpin->setMinimumWidth(70);
    rangeRow->addWidget(m_xMinSpin);
    m_xMaxSpin = new QDoubleSpinBox(this);
    m_xMaxSpin->setRange(-1e6, 1e6);
    m_xMaxSpin->setDecimals(2);
    m_xMaxSpin->setValue(3);
    m_xMaxSpin->setMinimumWidth(70);
    rangeRow->addWidget(m_xMaxSpin);

    rangeRow->addWidget(new QLabel(tr("Y:"), this));
    m_yMinSpin = new QDoubleSpinBox(this);
    m_yMinSpin->setRange(-1e6, 1e6);
    m_yMinSpin->setDecimals(2);
    m_yMinSpin->setValue(-3);
    m_yMinSpin->setMinimumWidth(70);
    rangeRow->addWidget(m_yMinSpin);
    m_yMaxSpin = new QDoubleSpinBox(this);
    m_yMaxSpin->setRange(-1e6, 1e6);
    m_yMaxSpin->setDecimals(2);
    m_yMaxSpin->setValue(3);
    m_yMaxSpin->setMinimumWidth(70);
    rangeRow->addWidget(m_yMaxSpin);

    rangeRow->addWidget(new QLabel(tr("Z:"), this));
    m_zMinSpin = new QDoubleSpinBox(this);
    m_zMinSpin->setRange(-1e6, 1e6);
    m_zMinSpin->setDecimals(2);
    m_zMinSpin->setValue(-5);
    m_zMinSpin->setMinimumWidth(70);
    rangeRow->addWidget(m_zMinSpin);
    m_zMaxSpin = new QDoubleSpinBox(this);
    m_zMaxSpin->setRange(-1e6, 1e6);
    m_zMaxSpin->setDecimals(2);
    m_zMaxSpin->setValue(5);
    m_zMaxSpin->setMinimumWidth(70);
    rangeRow->addWidget(m_zMaxSpin);

    rangeRow->addSpacing(12);
    m_colorButton = new QPushButton(tr("Curve &color…"), this);
    m_colorButton->setToolTip(tr("Choose color for the curve (2D) or surface (3D)"));
    QColor initialColor = m_graphWidget->curveColor();
    if (initialColor.isValid())
        m_colorButton->setStyleSheet(QStringLiteral("background-color: %1; min-width: 60px;").arg(initialColor.name()));
    connect(m_colorButton, &QPushButton::clicked, this, &MainWindow::chooseCurveColor);
    rangeRow->addWidget(m_colorButton);
    rangeRow->addStretch(1);

    layout->addLayout(rangeRow);

    layout->addWidget(m_graphStack, 1);

    setWindowTitle(tr("KGrapher"));
    resize(800, 600);

    setupMenus();
}

void MainWindow::onViewModeChanged(int index)
{
    m_graphStack->setCurrentIndex(index);
    if (index == 0) {
        m_equationEdit->setPlaceholderText(tr("2D: y = f(x) e.g. x^2, sin(x), 2*x+1"));
    } else {
        m_equationEdit->setPlaceholderText(tr("3D: z = f(x,y) e.g. x^2+y^2, sin(sqrt(x^2+y^2))"));
    }
}

void MainWindow::chooseCurveColor()
{
    QColor current = m_graphWidget->curveColor();
    QColor c = QColorDialog::getColor(current, this, tr("Curve color"));
    if (c.isValid()) {
        m_graphWidget->setCurveColor(c);
        m_graphWidget3D->setSurfaceColor(c);
        m_colorButton->setStyleSheet(QStringLiteral("background-color: %1; min-width: 60px;").arg(c.name()));
    }
}

void MainWindow::drawGraph()
{
    QString expr = m_equationEdit->text().trimmed();
    if (expr.isEmpty()) {
        QMessageBox::information(this, tr("Graph"), tr("Enter an equation first."));
        return;
    }

    ExpressionParser parser;
    if (!parser.parse(expr)) {
        QMessageBox::warning(this, tr("Invalid equation"),
            tr("Could not parse equation: %1").arg(parser.errorString()));
        return;
    }

    if (m_viewModeCombo->currentIndex() == 0) {
        const int numSamples = 2000;
        double xMin = m_xMinSpin->value();
        double xMax = m_xMaxSpin->value();
        double yMin = m_yMinSpin->value();
        double yMax = m_yMaxSpin->value();
        if (xMin >= xMax) xMax = xMin + 1.0;
        if (yMin >= yMax) yMax = yMin + 1.0;
        QVector<QPointF> samples;
        samples.reserve(numSamples);
        for (int i = 0; i <= numSamples; ++i) {
            double x = xMin + (xMax - xMin) * i / numSamples;
            double y = parser.eval(x);
            samples.append(QPointF(x, y));
        }
        m_graphWidget->setXRange(xMin, xMax);
        m_graphWidget->setYRange(yMin, yMax);
        m_graphWidget->setAutoYRange(false);
        m_graphWidget->setSamples(samples);
    } else {
        const int gridSize = 80;
        double xMin = m_xMinSpin->value();
        double xMax = m_xMaxSpin->value();
        double yMin = m_yMinSpin->value();
        double yMax = m_yMaxSpin->value();
        double zMin = m_zMinSpin->value();
        double zMax = m_zMaxSpin->value();
        if (xMin >= xMax) xMax = xMin + 1.0;
        if (yMin >= yMax) yMax = yMin + 1.0;
        if (zMin >= zMax) zMax = zMin + 1.0;
        QVector<QVector<Point3D>> grid;
        grid.reserve(gridSize + 1);
        for (int i = 0; i <= gridSize; ++i) {
            double x = xMin + (xMax - xMin) * i / gridSize;
            QVector<Point3D> row;
            row.reserve(gridSize + 1);
            for (int j = 0; j <= gridSize; ++j) {
                double y = yMin + (yMax - yMin) * j / gridSize;
                double z = parser.eval(x, y);
                row.append({ x, y, z });
            }
            grid.append(row);
        }
        m_graphWidget3D->setXRange(xMin, xMax);
        m_graphWidget3D->setYRange(yMin, yMax);
        m_graphWidget3D->setZRange(zMin, zMax);
        m_graphWidget3D->setAutoZRange(false);
        m_graphWidget3D->setSurface(grid);
    }
}

void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::fileNew);
    fileMenu->addAction(tr("&Open…"), QKeySequence::Open, this, &MainWindow::fileOpen);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::fileSave);
    fileMenu->addAction(tr("Save &As…"), QKeySequence::SaveAs, this, &MainWindow::fileSaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), QKeySequence::Quit, qApp, &QApplication::quit);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&Undo"), QKeySequence::Undo, m_equationEdit, &QLineEdit::undo);
    editMenu->addAction(tr("&Redo"), QKeySequence::Redo, m_equationEdit, &QLineEdit::redo);
    editMenu->addSeparator();
    editMenu->addAction(tr("Cu&t"), QKeySequence::Cut, m_equationEdit, &QLineEdit::cut);
    editMenu->addAction(tr("&Copy"), QKeySequence::Copy, m_equationEdit, &QLineEdit::copy);
    editMenu->addAction(tr("&Paste"), QKeySequence::Paste, m_equationEdit, &QLineEdit::paste);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::helpAbout);
}

void MainWindow::fileNew()
{
    if (maybeSave()) {
        setEquationText(QString());
        m_currentPath.clear();
        setWindowTitle(tr("KGrapher"));
    }
}

void MainWindow::fileOpen()
{
    if (!maybeSave())
        return;
    QString path = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                tr("Text files (*.txt);;All files (*)"));
    if (!path.isEmpty())
        loadFile(path);
}

void MainWindow::fileSave()
{
    if (m_currentPath.isEmpty())
        fileSaveAs();
    else
        saveFile(m_currentPath);
}

void MainWindow::fileSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save As"), m_currentPath,
                                                 tr("Text files (*.txt);;All files (*)"));
    if (!path.isEmpty())
        saveFile(path);
}

void MainWindow::helpAbout()
{
    QMessageBox::about(this, tr("About KGrapher"),
        tr("<h3>KGrapher 1.0</h3>"
           "<p>Enter an equation in LaTeX-style (e.g. x^2, sin(x), 2*x+1) and click Graph.</p>"
           "<p>Built with Qt %1.</p>").arg(qVersion()));
}

QString MainWindow::equationText() const
{
    return m_equationEdit ? m_equationEdit->text() : QString();
}

void MainWindow::setEquationText(const QString &text)
{
    if (m_equationEdit) {
        m_equationEdit->blockSignals(true);
        m_equationEdit->setText(text);
        m_equationEdit->blockSignals(false);
        m_equationModified = false;
    }
}

bool MainWindow::maybeSave()
{
    if (!m_equationModified)
        return true;
    QMessageBox::StandardButton b = QMessageBox::question(this, tr("Unsaved changes"),
        tr("The equation has been modified. Save changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (b == QMessageBox::Cancel)
        return false;
    if (b == QMessageBox::Save)
        fileSave();
    return true;
}

bool MainWindow::saveFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    if (f.write(equationText().toUtf8()) < 0) {
        f.close();
        return false;
    }
    f.close();
    m_currentPath = path;
    setWindowTitle(tr("%1 - KGrapher").arg(QFileInfo(path).fileName()));
    m_equationModified = false;
    return true;
}

bool MainWindow::loadFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    setEquationText(QString::fromUtf8(f.readAll()));
    f.close();
    m_currentPath = path;
    setWindowTitle(tr("%1 - KGrapher").arg(QFileInfo(path).fileName()));
    m_graphWidget->clear();
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}
