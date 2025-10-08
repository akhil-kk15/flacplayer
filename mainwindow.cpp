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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_audioManager(nullptr)
{
    qDebug() << "=== MainWindow Constructor START ===";
    
    ui->setupUi(this);
    
    // Initialize audio manager
    m_audioManager = new AudioManager(this);
    connect(m_audioManager, &AudioManager::fileOpened, this, &MainWindow::onFileOpened);
    connect(m_audioManager, &AudioManager::fileClosed, this, &MainWindow::onFileClosed);
    connect(m_audioManager, &AudioManager::errorOccurred, this, &MainWindow::onAudioError);
    
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
    setWindowTitle("FLAC Player v1.0 - Now with FFmpeg!");
    setMinimumSize(700, 500);
    resize(900, 600);
    
    // Status bar message
    statusBar()->showMessage("FFmpeg FLAC Player - Ready to load audio files!", 3000);
    
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
    QLabel *welcomeLabel = new QLabel("🎵 FFmpeg FLAC Player", this);
    welcomeLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px; color: #2c3e50;");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(welcomeLabel);
    
    // Add buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_openFileButton = new QPushButton("📁 Open Audio File", this);
    m_testButton = new QPushButton("🔧 Button Check", this);
    m_debugButton = new QPushButton("🐛 Show Debug Info", this);
    
    buttonLayout->addWidget(m_openFileButton);
    buttonLayout->addWidget(m_testButton);
    buttonLayout->addWidget(m_debugButton);
    buttonLayout->addStretch(); // This pushes buttons to the left
    
    layout->addLayout(buttonLayout);
    
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
    
    qDebug() << "Basic UI setup complete";
}

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
        QMessageBox::information(this, "Congratulations!", 
            "You've clicked the button 5 times! You're learning Qt!");
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
}

void MainWindow::onFileClosed()
{
    qDebug() << "=== File closed signal received ===";
    
    m_statusLabel->setText("Status: No file loaded");
    m_audioInfoLabel->setText("No audio file loaded");
    statusBar()->showMessage("File closed", 2000);
}

void MainWindow::onAudioError(const QString &error)
{
    qDebug() << "=== Audio error:" << error << "===";
    
    QMessageBox::warning(this, "Audio Error", error);
    m_statusLabel->setText("Status: Error - " + error);
    statusBar()->showMessage("Error: " + error, 5000);
}
