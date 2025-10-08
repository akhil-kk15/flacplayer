#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include "audiomanager.h"

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
    void onOpenFile();
    void onFileOpened(const QString &fileName);
    void onFileClosed();
    void onAudioError(const QString &error);

private:
    void initializeWindow();
    void setupBasicUI();
    
private:
    Ui::MainWindow *ui;
    
    // UI elements
    QPushButton *m_testButton;
    QPushButton *m_debugButton;
    QPushButton *m_openFileButton;
    QLabel *m_statusLabel;
    QLabel *m_audioInfoLabel;
    
    // Audio management
    AudioManager *m_audioManager;
};
#endif // MAINWINDOW_H
