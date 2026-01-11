
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QElapsedTimer>
#include "playlist.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Repeat mode enumeration
enum class RepeatMode {
    Off,        // No repeat
    All,        // Repeat entire playlist
    One         // Repeat current song
};

//window class, handles UI, plaback and the stupid gradient effect
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //constrctor and destructor
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    //file and playlist management slots, open files, show queue, edit metadata
    void on_actionOpen_triggered();       
    void on_trackQueue_clicked();         
    void on_actionEditMetadata_triggered();
    
    //playback control slots
    void on_playPause_clicked();        
    void on_nextTrack_clicked();         
    void on_previousTrack_clicked();    
    void on_Shuffle_clicked();          
    //audio control slots
    void onMuteToggle();                 
    void on_volumeSlider_valueChanged(int value); 
    void on_seekSlider_valueChanged(int value);  
    //Mplayer signal handlers
    void onPositionChanged(qint64 position);       
    void onDurationChanged(qint64 duration);      
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status); 

    void on_repeatToggle_clicked();
    void on_trackStop_clicked();

protected:
    //event handlers for mouse tracking and button hold detection
    void mouseMoveEvent(QMouseEvent *event) override;    
    void paintEvent(QPaintEvent *event) override;       
    bool eventFilter(QObject *obj, QEvent *event) override; 
    
private:
    //helpers for track loading, metadata display, seeking
    void loadTrack(int index);       
    void updateNextTrackDisplay();   
    void displayMetadata();
    void seekForward();             
    void seekBackward();            
    
  
    Ui::MainWindow *ui;  ///< User interface pointer
    
    // Media playback components
    QMediaPlayer *MPlayer;
    QAudioOutput *audioOutput; 
    // Playlist management
    Playlist playlist;         
    Playlist originalPlaylist;      ///< Store original playlist order before shuffling
    int currentTrackIndex = -1;     ///< Index of currently playing track (-1 = none)
    
    // Playback state variables
    bool isPlaying = false;        
    bool isMuted = false;           
    bool isSeeking = false;      
    qint64 mediaDuration = 0;
    RepeatMode repeatMode = RepeatMode::Off;       ///< Total duration of current track in milliseconds
    bool isShuffleOn = false;      ///< Shuffle state (off by default)
    
    // Button press vs hold detection
    QElapsedTimer buttonPressTimer;
    bool isButtonHeld = false;     
    
    // Mouse gradient effect variables
    QPoint mousePos;                
    QPoint lastMousePos;            
    QElapsedTimer frameTimer;       
    qint64 lastUpdateTime;          
    static constexpr int TARGET_FRAME_TIME = 16; ///< Target frame time in ms (~60 FPS)
};

#endif // MAINWINDOW_H
