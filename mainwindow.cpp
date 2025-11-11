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
    
    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Top area: album art + track title
    m_albumArtLabel = new QLabel(this);
    m_albumArtLabel->setFixedSize(320, 320);
    m_albumArtLabel->setStyleSheet("background-color:#f0f0f0; border:1px solid #bbb;");
    m_albumArtLabel->setAlignment(Qt::AlignCenter);
    m_albumArtLabel->setText("♪");
    m_albumArtLabel->setScaledContents(true); // Scale pixmap to fit label

    m_audioInfoLabel = new QLabel("No audio file loaded", this);
    m_audioInfoLabel->setWordWrap(true);
    m_audioInfoLabel->setAlignment(Qt::AlignCenter);
    m_audioInfoLabel->setStyleSheet("font-weight: bold; margin-top: 8px;");

    QVBoxLayout *topLayout = new QVBoxLayout();
    topLayout->addWidget(m_albumArtLabel, 0, Qt::AlignHCenter);
    topLayout->addWidget(m_audioInfoLabel);

    layout->addLayout(topLayout);

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

    layout->addLayout(seekLayout);

    // Keep references to elapsed/total labels for updates
    m_timeLabel = elapsedLabel;
    m_totalTimeLabel = totalLabel;

    // Controls bar
    QHBoxLayout *controlsLayout = new QHBoxLayout();

    QPushButton *prevButton = new QPushButton("⏮", this);
    m_playPauseButton = new QPushButton("▶", this);
    m_stopButton = new QPushButton("⏹", this);
    QPushButton *nextButton = new QPushButton("⏭", this);

    // Volume and open
    QLabel *volIcon = new QLabel("🔊", this);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setMaximumWidth(150);

    m_openFileButton = new QPushButton("Open", this);
    m_debugButton = new QPushButton("Debug", this);

    controlsLayout->addWidget(prevButton);
    controlsLayout->addWidget(m_playPauseButton);
    controlsLayout->addWidget(m_stopButton);
    controlsLayout->addWidget(nextButton);
    controlsLayout->addStretch();
    controlsLayout->addWidget(volIcon);
    controlsLayout->addWidget(m_volumeSlider);
    controlsLayout->addWidget(m_openFileButton);
    controlsLayout->addWidget(m_debugButton);

    layout->addLayout(controlsLayout);

    // Status label at bottom
    m_statusLabel = new QLabel("Status: Ready", this);
    m_statusLabel->setStyleSheet("color: #444; padding: 6px;");
    layout->addWidget(m_statusLabel);

    // Wire up signals
    connect(m_openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::onDebugInfo);

    connect(m_playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(prevButton, &QPushButton::clicked, [this]() { /* future: prev track */ });
    connect(nextButton, &QPushButton::clicked, [this]() { /* future: next track */ });

    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_seekSlider, &QSlider::sliderPressed, [this]() { m_seekSliderPressed = true; });
    connect(m_seekSlider, &QSlider::sliderReleased, [this]() {
        m_seekSliderPressed = false;
        onSeekPositionChanged(m_seekSlider->value());
    });

    // Keep elapsed/total labels in sync with internal time label
    connect(this, &MainWindow::destroyed, this, []() {}); // placeholder to avoid unused warnings

    // Update initial control states
    updatePlaybackControls();

    qDebug() << "Basic UI setup complete";
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
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Audio File",
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        "Audio Files (*.flac *.mp3 *.wav *.ogg *.m4a);;FLAC Files (*.flac);;All Files (*)"
    );
    


    if (!fileName.isEmpty()) {
        qDebug() << "Selected file:" << fileName;
        
        // Try to open the file with FFmpeg
        if (m_audioManager->openFile(fileName)) {
            qDebug() << "File opened successfully via FFmpeg";
        } else {
            qDebug() << "Failed to open file via FFmpeg";
        }
    } else {
        qDebug() << "No file selected";
    }
}



void MainWindow::onFileOpened(const QString &fileName)
{
    qDebug() << "=== File opened signal received:" << fileName << "===";
    
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
    
    m_statusLabel->setText("Status: No file loaded");
    m_audioInfoLabel->setText("No audio file loaded");
    statusBar()->showMessage("File closed", 2000);
    
    // Clear album art
    if (m_albumArtLabel) {
        m_albumArtLabel->clear();
        m_albumArtLabel->setText("♪");
    }
    
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

// Helper functions
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
