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
    
    // Add a welcome label
    QLabel *welcomeLabel = new QLabel(" FLAC Player", this);
    welcomeLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px; color: #2c3e50;");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(welcomeLabel);
    
    // Add buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_openFileButton = new QPushButton(" Open Audio File", this);
    m_testButton = new QPushButton(" test button", this);
    m_debugButton = new QPushButton(" Show Debug Info", this);
    
    buttonLayout->addWidget(m_openFileButton);
    buttonLayout->addWidget(m_testButton);
     buttonLayout->addWidget(m_debugButton);
    buttonLayout->addStretch(); // This pushes buttons to the left
    
    layout->addLayout(buttonLayout);
    
    // Add playback controls
    QHBoxLayout *playbackLayout = new QHBoxLayout();
    
    m_playPauseButton = new QPushButton("▶ Play", this);
    m_stopButton = new QPushButton("⏹ Stop", this);
    
    // Volume control
    QLabel *volumeLabel = new QLabel(" Volume:", this);
    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setMaximumWidth(150);
    
    playbackLayout->addWidget(m_playPauseButton);
    playbackLayout->addWidget(m_stopButton);
    playbackLayout->addStretch();
    playbackLayout->addWidget(volumeLabel);
    playbackLayout->addWidget(m_volumeSlider);
    
    layout->addLayout(playbackLayout);
    
    // Add seek/progress controls
    QHBoxLayout *progressLayout = new QHBoxLayout();
    
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setMinimumWidth(100);
    
    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 1000);
    m_seekSlider->setValue(0);
    
    progressLayout->addWidget(m_timeLabel);
    progressLayout->addWidget(m_seekSlider);
    
    layout->addLayout(progressLayout);
    
    // Add status label
    m_statusLabel = new QLabel("Status: Ready to load audio files!", this);
    m_statusLabel->setStyleSheet("border: 1px solid gray; color: black; padding: 5px; background-color: lightgray;");
    layout->addWidget(m_statusLabel);
    
    // Add audio info label
    m_audioInfoLabel = new QLabel("No audio file loaded", this);
    m_audioInfoLabel->setStyleSheet("border: 1px solid #3498db; color: #2c3e50; padding: 10px; background-color: #ecf0f1; font-family: monospace;");
    layout->addWidget(m_audioInfoLabel);
    
    layout->addStretch(); // This pushes everything to the top
    
    // Connect signals to slots (this is how Qt handles events!)
    connect(m_openFileButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_testButton, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::onDebugInfo);
    
    // Connect playback control signals
    connect(m_playPauseButton, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_seekSlider, &QSlider::sliderPressed, [this]() { m_seekSliderPressed = true; });
    connect(m_seekSlider, &QSlider::sliderReleased, [this]() { 
        m_seekSliderPressed = false;
        onSeekPositionChanged(m_seekSlider->value());
    });
    
    // Initialize playback controls state
    updatePlaybackControls();
    
    qDebug() << "Basic UI setup complete";
}

//Add new buttons here, play, pause, stop, repeat, shuffle. 

// void::MainWindow::mybutton(){
// QPushButton *newButton = new QPushButton("play", this);
// buttonLayout->addWidget(newButton);
// connect(newButton, &QPushButton::clicked, this, &MainWindow::mySlot);

// }
// This is called when the "Button Check" button is pressed
void MainWindow::onButtonClicked()
{
    qDebug() << "=== Test button clicked! ===";
    
    static int clickCount = 0;
    clickCount++;
    
    QString message = QString("Button clicked %1 times!").arg(clickCount);
    m_statusLabel->setText(message);
    statusBar()->showMessage(message, 2000);
    
    qDebug() << "Click count:" << clickCount;
    
    if (clickCount == 5) {
        QMessageBox::information(this, "pop up test!", 
            "You've clicked the button 5 times, change this later for developer settings");
    }
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
    // Update time display
    QString timeText = formatTime(position);
    if (m_duration > 0) {
        timeText += " / " + formatTime(m_duration);
    } else {
        timeText += " / 00:00";
    }
    m_timeLabel->setText(timeText);
    
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
    
    // Update time display
    QString timeText = formatTime(0) + " / " + formatTime(duration);
    m_timeLabel->setText(timeText);
    
    // Reset seek slider
    m_seekSlider->setValue(0);
}

// Helper functions
void MainWindow::updatePlaybackControls()
{
    AudioManager::PlaybackState state = m_audioManager->getState();
    bool hasFile = m_audioManager->isFileOpen();
    
    // Update play/pause button
    if (state == AudioManager::PlayingState) {
        m_playPauseButton->setText(" Pause");
    } else {
        m_playPauseButton->setText(" Play");
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
