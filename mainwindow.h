#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QProgressBar>
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
    
    // Playback control slots
    void onPlayPause();
    void onStop();
    void onVolumeChanged(int value);
    void onSeekPositionChanged(int value);
    void onAudioStateChanged(AudioManager::PlaybackState state);
    void onAudioPositionChanged(qint64 position);
    void onAudioDurationChanged(qint64 duration);

private:
    void initializeWindow();
    void setupBasicUI();
    void updatePlaybackControls();
    QString formatTime(qint64 microseconds) const;
    
private:
    Ui::MainWindow *ui;
    
    // UI elements
    QPushButton *m_testButton;
    QPushButton *m_debugButton;
    QPushButton *m_openFileButton;
    QPushButton *m_playPauseButton;
    QPushButton *m_stopButton;
    QLabel *m_statusLabel;
    QLabel *m_audioInfoLabel;
    QLabel *m_timeLabel;
    QSlider *m_volumeSlider;
    QSlider *m_seekSlider;
    QProgressBar *m_progressBar;
    
    // Audio management
    AudioManager *m_audioManager;
    qint64 m_duration;
    bool m_seekSliderPressed;
};
#endif // MAINWINDOW_H
