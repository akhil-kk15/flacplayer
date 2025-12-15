/**
 * @file mainwindow.h
 * @brief Main window header for FLAC Player application
 * 
 * This header defines the MainWindow class which handles all UI interactions,
 * media playback, playlist management, and visual effects.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QElapsedTimer>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Main application window class for FLAC Player
 * 
 * Handles media playback, playlist management, UI updates, and mouse gradient effects.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // ========== CONSTRUCTOR / DESTRUCTOR ==========
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // ========== FILE & PLAYLIST MANAGEMENT SLOTS ==========
    void on_actionOpen_triggered();       ///< Open file dialog to add tracks to playlist
    void on_trackQueue_clicked();         ///< Show track queue dialog
    
    // ========== PLAYBACK CONTROL SLOTS ==========
    void on_playPause_clicked();          ///< Toggle play/pause
    void on_nextTrack_clicked();          ///< Next track or seek forward (click vs hold)
    void on_previousTrack_clicked();      ///< Previous track or seek backward (click vs hold)
    void on_Shuffle_clicked();            ///< Shuffle button (placeholder)
    
    // ========== AUDIO CONTROL SLOTS ==========
    void onMuteToggle();                  ///< Toggle mute/unmute audio
    void on_volumeSlider_valueChanged(int value);  ///< Adjust volume when slider moves
    void on_seekSlider_valueChanged(int value);    ///< Seek to position when slider moves
    
    // ========== MEDIA PLAYER SIGNAL HANDLERS ==========
    void onPositionChanged(qint64 position);       ///< Update UI when playback position changes
    void onDurationChanged(qint64 duration);       ///< Update UI when track duration is loaded
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);  ///< Handle media status (e.g., end of track)

protected:
    // ========== EVENT HANDLERS (OVERRIDES) ==========
    void mouseMoveEvent(QMouseEvent *event) override;    ///< Track mouse for gradient effect
    void paintEvent(QPaintEvent *event) override;        ///< Draw gradient following mouse
    bool eventFilter(QObject *obj, QEvent *event) override;  ///< Filter events for buttons and widgets
    
private:
    // ========== HELPER METHODS ==========
    void loadTrack(int index);        ///< Load and prepare a track for playback
    void updateNextTrackDisplay();    ///< Update the "Next Track" label
    void displayMetadata();           ///< Extract and display metadata including album art
    void seekForward();               ///< Seek forward 10 seconds in current track
    void seekBackward();              ///< Seek backward 10 seconds in current track
    
    // ========== MEMBER VARIABLES ==========
    
    // UI pointer
    Ui::MainWindow *ui;  ///< User interface pointer
    
    // Media playback components
    QMediaPlayer *MPlayer;          ///< Qt media player for audio playback
    QAudioOutput *audioOutput;      ///< Qt6 audio output device
    
    // Playlist management
    QStringList playlist;           ///< List of file paths in the queue
    int currentTrackIndex = -1;     ///< Index of currently playing track (-1 = none)
    
    // Playback state variables
    bool isPlaying = false;         ///< Track play/pause state (manual tracking)
    bool isMuted = false;           ///< Track mute state
    bool isSeeking = false;         ///< Flag to prevent feedback loop during seek
    qint64 mediaDuration = 0;       ///< Total duration of current track in milliseconds
    
    // Button press/hold detection
    QElapsedTimer buttonPressTimer; ///< Timer to measure button press duration
    bool isButtonHeld = false;      ///< Flag indicating if button was held (vs clicked)
    
    // Mouse gradient effect variables
    QPoint mousePos;                ///< Current mouse position for gradient effect
    QPoint lastMousePos;            ///< Previous mouse position for optimized repainting
    QElapsedTimer frameTimer;       ///< Timer to control repaint rate
    qint64 lastUpdateTime;          ///< Last time mouse position was updated (for throttling)
    static constexpr int TARGET_FRAME_TIME = 16; ///< Target frame time ~60fps
};

#endif // MAINWINDOW_H
