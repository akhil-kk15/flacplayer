#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QListWidget>
#include "audiomanager.h"
#include "playlist.h"
#include "conversiondialog.h"

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
    void onDebugInfo();
    void onOpenFile();
    void onShowAudioInfo();
    void onFileOpened(const QString &fileName);
    void onFileClosed();
    void onAudioError(const QString &error);
    
    // Playback control slots
    void onPlayPause();
    void onStop();
    void onPrevious();
    void onNext();
    void onVolumeChanged(int value);
    void onSeekPositionChanged(int value);
    void onAudioStateChanged(AudioManager::PlaybackState state);
    void onAudioPositionChanged(qint64 position);
    void onAudioDurationChanged(qint64 duration);
    void onAlbumArtChanged(const QPixmap &albumArt);
    void onTrackFinished();
    void onMetadataChanged(const AudioMetadata &metadata);
    
    // Playlist slots
    void onPlaylistChanged();
    void onCurrentIndexChanged(int index);
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void onShuffleToggled(bool checked);
    void onShuffleChanged(bool enabled);
    void onSavePlaylist();
    void onLoadPlaylist();
    void onNewPlaylist();
    void onClearPlaylist();
    void onEditMetadata();
    void onConvertToMP3();

private:
    void initializeWindow();
    void setupBasicUI();
    void updatePlaybackControls();
    void loadTrackAtIndex(int index);
    QString formatTime(qint64 microseconds) const;
    
private:
    Ui::MainWindow *ui;
    
    // UI elements
    QPushButton *m_debugButton;
    QPushButton *m_openFileButton;
    QPushButton *m_infoButton;
    QPushButton *m_playPauseButton;
    QPushButton *m_stopButton;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QPushButton *m_shuffleButton;
    QLabel *m_statusLabel;
    QLabel *m_audioInfoLabel;
    QLabel *m_albumArtLabel;
    QLabel *m_titleLabel;
    QLabel *m_artistLabel;
    QLabel *m_albumLabel;
    QLabel *m_yearLabel;
    QLabel *m_timeLabel;
    QLabel *m_totalTimeLabel;
    QSlider *m_volumeSlider;
    QSlider *m_seekSlider;
    QListWidget *m_playlistWidget;
    QPushButton *m_savePlaylistButton;
    QPushButton *m_loadPlaylistButton;
    QPushButton *m_newPlaylistButton;
    QPushButton *m_clearPlaylistButton;
    QPushButton *m_editMetadataButton;
    QPushButton *m_convertButton;
    QLabel *m_playlistNameLabel;
    
    // Audio management
    AudioManager *m_audioManager;
    Playlist *m_playlist;
    qint64 m_duration;
    bool m_seekSliderPressed;
};
#endif // MAINWINDOW_H
