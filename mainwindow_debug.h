#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QFileInfo>

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
    void onOpenFile();
    void onPlay();
    void onStop();
    void onDebugInfo();

private:
    void initializeWindow();
    void setupUI();
    void connectSignals();
    
private:
    Ui::MainWindow *ui;
    
    // UI Components
    QPushButton *m_openButton;
    QPushButton *m_playButton;
    QPushButton *m_stopButton;
    QPushButton *m_debugButton;
    QLabel *m_statusLabel;
    
    // Data
    QString m_currentFile;
};
#endif // MAINWINDOW_H