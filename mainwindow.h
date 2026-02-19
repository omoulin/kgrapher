#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QComboBox;
class QStackedWidget;
class QDoubleSpinBox;
class QPushButton;
class GraphWidget;
class GraphWidget3D;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void drawGraph();
    void onViewModeChanged(int index);
    void chooseCurveColor();
    void fileNew();
    void fileOpen();
    void fileSave();
    void fileSaveAs();
    void helpAbout();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupMenus();
    bool maybeSave();
    bool saveFile(const QString &path);
    bool loadFile(const QString &path);
    QString equationText() const;
    void setEquationText(const QString &text);

    QWidget *m_central = nullptr;
    QLineEdit *m_equationEdit = nullptr;
    QComboBox *m_viewModeCombo = nullptr;
    QDoubleSpinBox *m_xMinSpin = nullptr;
    QDoubleSpinBox *m_xMaxSpin = nullptr;
    QDoubleSpinBox *m_yMinSpin = nullptr;
    QDoubleSpinBox *m_yMaxSpin = nullptr;
    QDoubleSpinBox *m_zMinSpin = nullptr;
    QDoubleSpinBox *m_zMaxSpin = nullptr;
    QPushButton *m_colorButton = nullptr;
    QStackedWidget *m_graphStack = nullptr;
    GraphWidget *m_graphWidget = nullptr;
    GraphWidget3D *m_graphWidget3D = nullptr;
    QString m_currentPath;
    bool m_equationModified = false;
};

#endif // MAINWINDOW_H
