#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onButtonClicked();
    void onDebugInfo();

private:
    void initializeWindow();
    void setupBasicUI();
    
private:
    Ui::MainWindow *ui;
    QPushButton *m_testButton;
    QPushButton *m_debugButton;
    QLabel *m_statusLabel;
};
#endif // MAINWINDOW_H
