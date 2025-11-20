#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSlider>
#include <QProgressBar>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QInputDialog>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_audioManager(nullptr)
    , m_duration(0)
    , m_seekSliderPressed(false)
{
    qDebug() << "=== MainWindow Constructor START ===";
    
    ui->setupUi(this);
    
    // Initialize audio manager
    m_audioManager = new AudioManager(this);
    connect(m_audioManager, &AudioManager::fileOpened, this, &MainWindow::onFileOpened);
    connect(m_audioManager, &AudioManager::fileClosed, this, &MainWindow::onFileClosed);
    connect(m_audioManager, &AudioManager::errorOccurred, this, &MainWindow::onAudioError);
    connect(m_audioManager, &AudioManager::stateChanged, this, &MainWindow::onAudioStateChanged);
    connect(m_audioManager, &AudioManager::positionChanged, this, &MainWindow::onAudioPositionChanged);
    connect(m_audioManager, &AudioManager::durationChanged, this, &MainWindow::onAudioDurationChanged);
    connect(m_audioManager, &AudioManager::albumArtChanged, this, &MainWindow::onAlbumArtChanged);
    connect(m_audioManager, &AudioManager::trackFinished, this, &MainWindow::onTrackFinished);
    connect(m_audioManager, &AudioManager::metadataChanged, this, &MainWindow::onMetadataChanged);
    
    // Initialize playlist
    m_playlist = new Playlist(this);
    connect(m_playlist, &Playlist::playlistChanged, this, &MainWindow::onPlaylistChanged);
    connect(m_playlist, &Playlist::currentIndexChanged, this, &MainWindow::onCurrentIndexChanged);
    connect(m_playlist, &Playlist::shuffleChanged, this, &MainWindow::onShuffleChanged);
    
    // Initialize the window
    initializeWindow();
    setupBasicUI();
    
    qDebug() << "=== MainWindow Constructor END ===";
}

MainWindow::~MainWindow()
{
    qDebug() << "=== MainWindow Destructor ===";
    delete ui;
}

void MainWindow::initializeWindow()
{
    qDebug() << "Initializing window...";
    
    // Set window properties
    setWindowTitle("FLAC Player v1.0");
    setMinimumSize(700, 500);
    resize(900, 600);
    
    // Status bar message
    statusBar()->showMessage("  Load audio files!", 3000);
    
    qDebug() << "Window initialized:";
    qDebug() << "  - Title:" << windowTitle();
    qDebug() << "  - Size:" << size();
}

void MainWindow::setupBasicUI()
{
    qDebug() << "Setting up basic UI...";
    
    // Create central widget (this replaces the ui->centralwidget)
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main horizontal layout
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // Left side: Player controls
    QVBoxLayout *leftLayout = new QVBoxLayout();

    // Top area: album art + track title
    m_albumArtLabel = new QLabel(this);
    m_albumArtLabel->setFixedSize(320, 320);
    m_albumArtLabel->setStyleSheet("background-color:#f0f0f0; border:1px solid #bbb;");
    m_albumArtLabel->setAlignment(Qt::AlignCenter);
    m_albumArtLabel->setText("♪");
    m_albumArtLabel->setScaledContents(true); // Scale pixmap to fit label

    leftLayout->addWidget(m_albumArtLabel, 0, Qt::AlignHCenter);
    
    // Metadata display - use a container with size constraint
    QWidget *metadataContainer = new QWidget(this);
    metadataContainer->setMinimumHeight(120);
    QVBoxLayout *metadataLayout = new QVBoxLayout(metadataContainer);
    metadataLayout->setContentsMargins(5, 5, 5, 5);
    metadataLayout->setSpacing(2);
    
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin-top: 8px;");
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setMaximumHeight(40);
    
    m_artistLabel = new QLabel(this);
    m_artistLabel->setAlignment(Qt::AlignCenter);
    m_artistLabel->setStyleSheet("font-size: 12px; color: #555;");
    m_artistLabel->setMaximumHeight(20);
    
    m_albumLabel = new QLabel(this);
    m_albumLabel->setAlignment(Qt::AlignCenter);
    m_albumLabel->setStyleSheet("font-size: 11px; color: #666;");
    m_albumLabel->setMaximumHeight(20);
    
    m_yearLabel = new QLabel(this);
    m_yearLabel->setAlignment(Qt::AlignCenter);
    m_yearLabel->setStyleSheet("font-size: 10px; color: #888;");
    m_yearLabel->setMaximumHeight(15);
    
    m_audioInfoLabel = new QLabel("No audio file loaded", this);
    m_audioInfoLabel->setWordWrap(true);
    m_audioInfoLabel->setAlignment(Qt::AlignCenter);
    m_audioInfoLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");
    m_audioInfoLabel->setMaximumHeight(25);
    
    metadataLayout->addWidget(m_titleLabel);
    metadataLayout->addWidget(m_artistLabel);
    metadataLayout->addWidget(m_albumLabel);
    metadataLayout->addWidget(m_yearLabel);
    metadataLayout->addWidget(m_audioInfoLabel);
    metadataLayout->addStretch();
    
    leftLayout->addWidget(metadataContainer);

    // Seek bar with time labels
    QHBoxLayout *seekLayout = new QHBoxLayout();
    QLabel *elapsedLabel = new QLabel("00:00", this);
    QLabel *totalLabel = new QLabel("00:00", this);

    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 1000);
    m_seekSlider->setValue(0);

    seekLayout->addWidget(elapsedLabel);
    seekLayout->addWidget(m_seekSlider);
    seekLayout->addWidget(totalLabel);

    leftLayout->addLayout(seekLayout);

    // Keep references to elapsed/total labels for updates
    m_timeLabel = elapsedLabel;
    m_totalTimeLabel = totalLabel;

    // Controls bar
    QHBoxLayout *controlsLayout = new QHBoxLayout();

    m_prevButton = new QPushButton("⏮", this);
    m_playPauseButton = new QPushButton("▶", this);
    m_stopButton = new QPushButton("⏹", this);
    m_nextButton = new QPushButton("⏭", this);
    
    m_shuffleButton = new QPushButton("shuffle", this);
    m_shuffleButton->setCheckable(true);
    m_shuffleButton->setToolTip("Shuffle");
    m_shuffleButton->setMaximumWidth(50);

    // Volume and open
    QLabel *volIcon = new QLabel("🔊", this);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setMaximumWidth(150);

    m_openFileButton = new QPushButton("Open", this);
    m_infoButton = new QPushButton("i", this);
    m_infoButton->setMaximumWidth(40);
    m_infoButton->setToolTip("Audio Info");
    m_debugButton = new QPushButton("Debug", this);

    controlsLayout->addWidget(m_prevButton);
    controlsLayout->addWidget(m_playPauseButton);
    controlsLayout->addWidget(m_stopButton);
    controlsLayout->addWidget(m_nextButton);
    controlsLayout->addWidget(m_shuffleButton);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_infoButton);
    controlsLayout->addWidget(volIcon);
    controlsLayout->addWidget(m_volumeSlider);
    controlsLayout->addWidget(m_openFileButton);
    controlsLayout->addWidget(m_debugButton);

    leftLayout->addLayout(controlsLayout);

    // Status label at bottom
    m_statusLabel = new QLabel("Status: Ready", this);
    m_statusLabel->setStyleSheet("color: #444; padding: 6px;");
    leftLayout->addWidget(m_statusLabel);

    // Right side: Playlist
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    // Playlist header with name
    QHBoxLayout *playlistHeaderLayout = new QHBoxLayout();
    QLabel *playlistLabel = new QLabel("Playlist:", this);
    playlistLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    m_playlistNameLabel = new QLabel("Untitled Playlist", this);
    m_playlistNameLabel->setStyleSheet("padding: 4px; color: #666;");
    playlistHeaderLayout->addWidget(playlistLabel);
    playlistHeaderLayout->addWidget(m_playlistNameLabel);
    playlistHeaderLayout->addStretch();
    
    // Playlist management buttons
    QHBoxLayout *playlistButtonsLayout = new QHBoxLayout();
    m_newPlaylistButton = new QPushButton("New", this);
    m_loadPlaylistButton = new QPushButton("Load", this);
    m_savePlaylistButton = new QPushButton("Save", this);
    m_clearPlaylistButton = new QPushButton("Clear", this);
    
    m_newPlaylistButton->setMaximumWidth(60);
    m_loadPlaylistButton->setMaximumWidth(60);
    m_savePlaylistButton->setMaximumWidth(60);
    m_clearPlaylistButton->setMaximumWidth(60);
    
    playlistButtonsLayout->addWidget(m_newPlaylistButton);
    playlistButtonsLayout->addWidget(m_loadPlaylistButton);
    playlistButtonsLayout->addWidget(m_savePlaylistButton);
    playlistButtonsLayout->addWidget(m_clearPlaylistButton);
    playlistButtonsLayout->addStretch();
    
    m_playlistWidget = new QListWidget(this);
    m_playlistWidget->setMinimumWidth(300);
    m_playlistWidget->setAlternatingRowColors(true);
    
    rightLayout->addLayout(playlistHeaderLayout);
    rightLayout->addLayout(playlistButtonsLayout);
    rightLayout->addWidget(m_playlistWidget);

    // Add both sides to main layout
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 0);

    // Wire up signals
    connect(m_openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_infoButton, &QPushButton::clicked, this, &MainWindow::onShowAudioInfo);
    connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::onDebugInfo);

    connect(m_playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_prevButton, &QPushButton::clicked, this, &MainWindow::onPrevious);
    connect(m_nextButton, &QPushButton::clicked, this, &MainWindow::onNext);
    connect(m_shuffleButton, &QPushButton::toggled, this, &MainWindow::onShuffleToggled);
    connect(m_playlistWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
    
    // Playlist management connections
    connect(m_newPlaylistButton, &QPushButton::clicked, this, &MainWindow::onNewPlaylist);
    connect(m_loadPlaylistButton, &QPushButton::clicked, this, &MainWindow::onLoadPlaylist);
    connect(m_savePlaylistButton, &QPushButton::clicked, this, &MainWindow::onSavePlaylist);
    connect(m_clearPlaylistButton, &QPushButton::clicked, this, &MainWindow::onClearPlaylist);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_seekSlider, &QSlider::sliderPressed, [this]() { m_seekSliderPressed = true; });
    connect(m_seekSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekPositionChanged);
    connect(m_seekSlider, &QSlider::sliderReleased, [this]() {
        m_seekSliderPressed = false;
        onSeekPositionChanged(m_seekSlider->value());
    });

    // Update initial control states
    updatePlaybackControls();

    qDebug() << "Basic UI setup complete";
}

void MainWindow::onShowAudioInfo()
{
    qDebug() << "=== Show audio info requested ===";
    
    if (!m_audioManager->isFileOpen()) {
        QMessageBox::information(this, "No File", "Please open an audio file first!");
        return;
    }
    
    // Get metadata
    AudioMetadata metadata = m_audioManager->getMetadata();
    
    // Build detailed info string
    QString info = "═══════════════════════════════\n";
    info += "       AUDIO FILE DETAILS\n";
    info += "═══════════════════════════════\n\n";
    
    // Metadata section
    info += " METADATA:\n";
    info += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    info += QString("Title:    %1\n").arg(metadata.title.isEmpty() ? "N/A" : metadata.title);
    info += QString("Artist:   %1\n").arg(metadata.artist.isEmpty() ? "N/A" : metadata.artist);
    info += QString("Album:    %1\n").arg(metadata.album.isEmpty() ? "N/A" : metadata.album);
    info += QString("Year:     %1\n").arg(metadata.year.isEmpty() ? "N/A" : metadata.year);
    info += QString("Genre:    %1\n").arg(metadata.genre.isEmpty() ? "N/A" : metadata.genre);
    if (!metadata.comment.isEmpty()) {
        info += QString("Comment:  %1\n").arg(metadata.comment);
    }
    info += "\n";
    
    // Technical section
    info += " TECHNICAL INFO:\n";
    info += "─────────────────────────────\n";
    info += QString("Format:       %1\n").arg(m_audioManager->getFormatName());
    info += QString("Codec:        %1\n").arg(m_audioManager->getCodecName());
    info += QString("Sample Rate:  %1 Hz\n").arg(m_audioManager->getSampleRate());
    info += QString("Channels:     %1 (%2)\n")
        .arg(m_audioManager->getChannels())
        .arg(m_audioManager->getChannels() == 1 ? "Mono" : 
             m_audioManager->getChannels() == 2 ? "Stereo" :
             QString("%1 Channel").arg(m_audioManager->getChannels()));
    qint64 bitrate = m_audioManager->getBitrate();
    if (bitrate > 0) {
        info += QString("Bitrate:      %1 kbps\n").arg(bitrate / 1000);
    }
    info += QString("Duration:     %1 (%2 seconds)\n")
        .arg(formatTime(m_audioManager->getDuration()))
        .arg(m_audioManager->getDuration() / 1000000.0, 0, 'f', 2);
    info += "\n";
    
    // File section
    info += " FILE INFO:\n";
    info += "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    QString currentFile = m_playlist->current();
    if (!currentFile.isEmpty()) {
        QFileInfo fileInfo(currentFile);
        info += QString("Filename:     %1\n").arg(fileInfo.fileName());
        info += QString("Path:         %1\n").arg(fileInfo.absolutePath());
        info += QString("Size:         %1 MB\n").arg(fileInfo.size() / 1024.0 / 1024.0, 0, 'f', 2);
    }
    
    // Show in a dialog with monospace font
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Audio File Information");
    msgBox.setText(info);
    msgBox.setIcon(QMessageBox::Information);
    
    // Use monospace font for better alignment
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(9);
    msgBox.setFont(font);
    
    msgBox.exec();
}

// This shows debug information about your application
void MainWindow::onDebugInfo()
{
    qDebug() << "=== Debug info requested ===";
    
    QString info = QString(
        "=== QT APPLICATION DEBUG INFO ===\n\n"
        "Window Properties:\n"
        "  • Title: %1\n"
        "  • Size: %2 x %3 pixels\n"
        "  • Position: (%4, %5)\n"
        "  • Visible: %6\n\n"
        "Qt Information:\n"
        "  • Qt Version: %7\n"
        "  • Widget Count: %8\n\n"
        "  • Check the terminal for debug output!\n"
        "  • Try setting breakpoints\n"
        "  • Use qDebug() for logging"
    ).arg(windowTitle())
     .arg(width()).arg(height())
     .arg(x()).arg(y())
     .arg(isVisible() ? "Yes" : "No")
     .arg(QT_VERSION_STR)
     .arg(findChildren<QWidget*>().count());
    
    // Show in message box
    QMessageBox::information(this, "Debug Information", info);
    
    // Also print to console with detailed formatting
    qDebug().noquote() << "\n" << info;
}

// New FFmpeg-related slot functions


//file format changes needed 
void MainWindow::onOpenFile()
{
    qDebug() << "=== onOpenFile() called ===";
    
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        "Open Audio File(s)",
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        "Audio Files (*.flac *.mp3 *.wav *.ogg *.m4a);;FLAC Files (*.flac);;All Files (*)"
    );
    
    if (!fileNames.isEmpty()) {
        qDebug() << "Selected" << fileNames.count() << "file(s)";
        
        // Add files to playlist
        m_playlist->addFiles(fileNames);
        
        // If this is the first file(s) added, start playing
        if (m_playlist->count() == fileNames.count()) {
            loadTrackAtIndex(0);
        }
    } else {
        qDebug() << "No file selected";
    }
}



void MainWindow::onFileOpened(const QString &fileName)
{
    qDebug() << "=== File opened signal received:" << fileName << "===";
    
    // Update window title with filename
    setWindowTitle("FLAC Player v1.0 - " + fileName);
    
    m_statusLabel->setText("Status: File loaded - " + fileName);
    statusBar()->showMessage("Loaded: " + fileName, 5000);
    
    // Update audio info display
    if (m_audioManager->isFileOpen()) {
        m_audioInfoLabel->setText(m_audioManager->getFormatInfo());
    }
    
    // Update playback controls
    updatePlaybackControls();
}

void MainWindow::onFileClosed()
{
    qDebug() << "=== File closed signal received ===";
    
    // Reset window title
    setWindowTitle("FLAC Player v1.0");
    
    m_statusLabel->setText("Status: No file loaded");
    m_audioInfoLabel->setText("No audio file loaded");
    statusBar()->showMessage("File closed", 2000);
    
    // Clear album art
    if (m_albumArtLabel) {
        m_albumArtLabel->clear();
        m_albumArtLabel->setText("♪");
    }
    
    // Clear metadata labels
    m_titleLabel->clear();
    m_artistLabel->clear();
    m_albumLabel->clear();
    m_yearLabel->clear();
    
    // Reset playback controls
    m_duration = 0;
    m_timeLabel->setText("00:00 / 00:00");
    m_seekSlider->setValue(0);
    updatePlaybackControls();
}

void MainWindow::onAudioError(const QString &error)
{
    qDebug() << "=== Audio error:" << error << "===";
    
    QMessageBox::warning(this, "Audio Error", error);
    m_statusLabel->setText("Status: Error - " + error);
    statusBar()->showMessage("Error: " + error, 5000);
}

// Playback control slot functions
void MainWindow::onPlayPause()
{
    qDebug() << "=== Play/Pause button clicked ===";
    
    if (!m_audioManager->isFileOpen()) {
        QMessageBox::information(this, "No File", "Please open an audio file first!");
        return;
    }
    
    AudioManager::PlaybackState state = m_audioManager->getState();
    
    if (state == AudioManager::PlayingState) {
        m_audioManager->pause();
    } else {
        m_audioManager->play();
    }
}

void MainWindow::onStop()
{
    qDebug() << "=== Stop button clicked ===";
    m_audioManager->stop();
}

void MainWindow::onPrevious()
{
    qDebug() << "=== Previous button clicked ===";
    
    if (m_playlist->hasPrevious()) {
        QString prevFile = m_playlist->previous();
        if (!prevFile.isEmpty()) {
            m_audioManager->stop();
            m_audioManager->openFile(prevFile);
            m_audioManager->play();
        }
    }
}

void MainWindow::onNext()
{
    qDebug() << "=== Next button clicked ===";
    
    if (m_playlist->hasNext()) {
        QString nextFile = m_playlist->next();
        if (!nextFile.isEmpty()) {
            m_audioManager->stop();
            m_audioManager->openFile(nextFile);
            m_audioManager->play();
        }
    }
}

void MainWindow::onTrackFinished()
{
    qDebug() << "=== Track finished ===";
    
    // Auto-advance to next track if available
    if (m_playlist->hasNext()) {
        onNext();
    } else {
        qDebug() << "End of playlist reached";
        m_audioManager->stop();
    }
}

void MainWindow::onVolumeChanged(int value)
{
    qreal volume = value / 100.0; // Convert to 0.0-1.0 range
    qDebug() << "=== Volume changed to:" << volume << "===";
    m_audioManager->setVolume(volume);
}

void MainWindow::onSeekPositionChanged(int value)
{
    if (m_duration > 0) {
        qint64 position = (qint64(value) * m_duration) / 1000;
        qDebug() << "=== Seek to position:" << position << "===";
        m_audioManager->setPosition(position);
    }
}

void MainWindow::onAudioStateChanged(AudioManager::PlaybackState state)
{
    qDebug() << "=== Audio state changed to:" << state << "===";
    updatePlaybackControls();
    
    QString stateText;
    switch (state) {
    case AudioManager::StoppedState:
        stateText = "Stopped";
        break;
    case AudioManager::PlayingState:
        stateText = "Playing";
        break;
    case AudioManager::PausedState:
        stateText = "Paused";
        break;
    }
    
    statusBar()->showMessage("Playback: " + stateText, 2000);
}

void MainWindow::onAudioPositionChanged(qint64 position)
{
    // Update elapsed time display
    if (m_timeLabel) {
        m_timeLabel->setText(formatTime(position));
    }

    // Update seek slider (only if user is not dragging it)
    if (!m_seekSliderPressed && m_duration > 0) {
        int sliderValue = (int)((position * 1000) / m_duration);
        m_seekSlider->setValue(sliderValue);
    }
}

void MainWindow::onAudioDurationChanged(qint64 duration)
{
    qDebug() << "=== Duration changed to:" << duration << "microseconds ===";
    m_duration = duration;
    
    // Update elapsed / total time labels
    if (m_timeLabel) {
        m_timeLabel->setText(formatTime(0));
    }
    if (m_totalTimeLabel) {
        m_totalTimeLabel->setText(formatTime(duration));
    }

    // Reset seek slider
    m_seekSlider->setValue(0);
}

void MainWindow::onAlbumArtChanged(const QPixmap &albumArt)
{
    qDebug() << "=== Album art changed ===";
    
    if (!albumArt.isNull() && m_albumArtLabel) {
        // Scale the album art to fit the label while maintaining aspect ratio
        QPixmap scaled = albumArt.scaled(
            m_albumArtLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        m_albumArtLabel->setPixmap(scaled);
        qDebug() << "Album art displayed:" << albumArt.size();
    } else if (m_albumArtLabel) {
        // No album art - show placeholder
        m_albumArtLabel->clear();
        m_albumArtLabel->setText("♪");
        qDebug() << "No album art to display";
    }
}

void MainWindow::onMetadataChanged(const AudioMetadata &metadata)
{
    qDebug() << "=== Metadata changed ===";
    
    if (!metadata.isEmpty()) {
        // Display title
        if (!metadata.title.isEmpty()) {
            m_titleLabel->setText(metadata.title);
        } else {
            m_titleLabel->setText("Unknown Title");
        }
        
        // Display artist
        if (!metadata.artist.isEmpty()) {
            m_artistLabel->setText(metadata.artist);
        } else {
            m_artistLabel->setText("Unknown Artist");
        }
        
        // Display album
        if (!metadata.album.isEmpty()) {
            QString albumText = metadata.album;
            if (!metadata.year.isEmpty()) {
                albumText += " (" + metadata.year + ")";
            }
            m_albumLabel->setText(albumText);
        } else {
            m_albumLabel->setText("Unknown Album");
        }
        
        // Display genre/year separately if no album
        if (metadata.album.isEmpty() && !metadata.year.isEmpty()) {
            m_yearLabel->setText(metadata.year);
        } else if (!metadata.genre.isEmpty()) {
            m_yearLabel->setText(metadata.genre);
        } else {
            m_yearLabel->clear();
        }
    } else {
        m_titleLabel->setText("No metadata available");
        m_artistLabel->clear();
        m_albumLabel->clear();
        m_yearLabel->clear();
    }
}

void MainWindow::onPlaylistChanged()
{
    qDebug() << "=== Playlist changed ===";
    
    // Update the playlist widget
    m_playlistWidget->clear();
    
    QStringList files = m_playlist->getFiles();
    for (int i = 0; i < files.count(); ++i) {
        QFileInfo fileInfo(files[i]);
        QString displayName = QString("%1. %2").arg(i + 1).arg(fileInfo.fileName());
        QListWidgetItem *item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, files[i]); // Store full path
        m_playlistWidget->addItem(item);
    }
    
    // Highlight current track
    if (m_playlist->currentIndex() >= 0) {
        m_playlistWidget->setCurrentRow(m_playlist->currentIndex());
    }
    
    // Update prev/next button states
    m_prevButton->setEnabled(m_playlist->hasPrevious());
    m_nextButton->setEnabled(m_playlist->hasNext());
}

void MainWindow::onCurrentIndexChanged(int index)
{
    qDebug() << "=== Current index changed to:" << index << "===";
    
    if (index >= 0 && index < m_playlistWidget->count()) {
        m_playlistWidget->setCurrentRow(index);
    }
    
    // Update prev/next button states
    m_prevButton->setEnabled(m_playlist->hasPrevious());
    m_nextButton->setEnabled(m_playlist->hasNext());
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    int index = m_playlistWidget->row(item);
    qDebug() << "=== Playlist item double-clicked, index:" << index << "===";
    
    loadTrackAtIndex(index);
}

void MainWindow::onShuffleToggled(bool checked)
{
    qDebug() << "=== Shuffle toggled:" << checked << "===";
    m_playlist->setShuffle(checked);
}

void MainWindow::onShuffleChanged(bool enabled)
{
    qDebug() << "=== Shuffle changed:" << enabled << "===";
    
    // Update button state
    m_shuffleButton->setChecked(enabled);
    
    // Update button style to show active state
    if (enabled) {
        m_shuffleButton->setStyleSheet("background-color: #4CAF50; color: white;");
        statusBar()->showMessage("Shuffle enabled", 2000);
    } else {
        m_shuffleButton->setStyleSheet("");
        statusBar()->showMessage("Shuffle disabled", 2000);
    }
}

void MainWindow::onSavePlaylist()
{
    qDebug() << "=== Save playlist clicked ===";
    
    if (m_playlist->count() == 0) {
        QMessageBox::information(this, "Empty Playlist", "Cannot save an empty playlist.");
        return;
    }
    
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save Playlist",
        defaultPath + "/" + m_playlist->getName() + ".m3u",
        "M3U Playlist (*.m3u);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        if (m_playlist->saveToFile(fileName)) {
            statusBar()->showMessage("Playlist saved: " + fileName, 3000);
            QMessageBox::information(this, "Success", "Playlist saved successfully!");
        } else {
            QMessageBox::warning(this, "Error", "Failed to save playlist.");
        }
    }
}

void MainWindow::onLoadPlaylist()
{
    qDebug() << "=== Load playlist clicked ===";
    
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Load Playlist",
        defaultPath,
        "M3U Playlist (*.m3u);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        if (m_playlist->loadFromFile(fileName)) {
            m_playlistNameLabel->setText(m_playlist->getName());
            statusBar()->showMessage("Playlist loaded: " + m_playlist->getName(), 3000);
            
            // Auto-start playing the first track if available
            if (m_playlist->count() > 0) {
                loadTrackAtIndex(0);
            }
        } else {
            QMessageBox::warning(this, "Error", "Failed to load playlist.");
        }
    }
}

void MainWindow::onNewPlaylist()
{
    qDebug() << "=== New playlist clicked ===";
    
    if (m_playlist->count() > 0) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "New Playlist",
            "Current playlist will be cleared. Continue?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    bool ok;
    QString name = QInputDialog::getText(
        this,
        "New Playlist",
        "Enter playlist name:",
        QLineEdit::Normal,
        "My Playlist",
        &ok
    );
    
    if (ok && !name.isEmpty()) {
        m_audioManager->stop();
        m_playlist->clear();
        m_playlist->setName(name);
        m_playlistNameLabel->setText(name);
        statusBar()->showMessage("New playlist created: " + name, 2000);
    }
}

void MainWindow::onClearPlaylist()
{
    qDebug() << "=== Clear playlist clicked ===";
    
    if (m_playlist->count() == 0) {
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Clear Playlist",
        "Are you sure you want to clear the playlist?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_audioManager->stop();
        m_playlist->clear();
        statusBar()->showMessage("Playlist cleared", 2000);
    }
}

// Helper functions
void MainWindow::loadTrackAtIndex(int index)
{
    if (index < 0 || index >= m_playlist->count()) {
        return;
    }
    
    QString filePath = m_playlist->getFileAt(index);
    if (filePath.isEmpty()) {
        return;
    }
    
    qDebug() << "=== Loading track at index:" << index << "===";
    
    m_playlist->setCurrentIndex(index);
    m_audioManager->stop();
    
    if (m_audioManager->openFile(filePath)) {
        m_audioManager->play();
    }
}

void MainWindow::updatePlaybackControls()
{
    AudioManager::PlaybackState state = m_audioManager->getState();
    bool hasFile = m_audioManager->isFileOpen();
    
    // Update play/pause button
    if (state == AudioManager::PlayingState) {
        m_playPauseButton->setText("⏸");
    } else {
        m_playPauseButton->setText("▶");
    }
    
    // Enable/disable controls based on file availability
    m_playPauseButton->setEnabled(hasFile);
    m_stopButton->setEnabled(hasFile && state != AudioManager::StoppedState);
    m_seekSlider->setEnabled(hasFile);
    
    qDebug() << "Playback controls updated - hasFile:" << hasFile << "state:" << state;
}

QString MainWindow::formatTime(qint64 microseconds) const
{
    qint64 seconds = microseconds / 1000000;
    qint64 minutes = seconds / 60;
    seconds = seconds % 60;
    
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
