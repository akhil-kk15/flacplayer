#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "=== MainWindow Constructor START ===";
    
    ui->setupUi(this);
    
    // Initialize the window
    initializeWindow();
    setupUI();
    connectSignals();
    
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
    setWindowTitle("FLAC Player v1.0 - Debug Build");
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    // Status bar message
    statusBar()->showMessage("Ready - Debug Mode Active", 2000);
    
    qDebug() << "Window initialized:";
    qDebug() << "  - Title:" << windowTitle();
    qDebug() << "  - Size:" << size();
    qDebug() << "  - Minimum Size:" << minimumSize();
}

void MainWindow::setupUI()
{
    qDebug() << "Setting up UI components...";
    
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create title label
    QLabel *titleLabel = new QLabel("FLAC Player Debug Interface", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // Create buttons layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // Create buttons
    m_openButton = new QPushButton("Open FLAC File", this);
    m_playButton = new QPushButton("Play", this);
    m_stopButton = new QPushButton("Stop", this);
    m_debugButton = new QPushButton("Debug Info", this);
    
    // Add buttons to layout
    buttonLayout->addWidget(m_openButton);
    buttonLayout->addWidget(m_playButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_debugButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Create status label
    m_statusLabel = new QLabel("Status: Ready", this);
    m_statusLabel->setStyleSheet("border: 1px solid gray; padding: 5px; margin: 5px;");
    mainLayout->addWidget(m_statusLabel);
    
    mainLayout->addStretch();
    
    qDebug() << "UI setup complete";
}

void MainWindow::connectSignals()
{
    qDebug() << "Connecting signals and slots...";
    
    // Connect button signals to slots
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::onPlay);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::onDebugInfo);
    
    qDebug() << "Signal-slot connections established";
}

// Slot implementations
void MainWindow::onOpenFile()
{
    qDebug() << "=== onOpenFile() called ===";
    
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open FLAC File",
        QString(),
        "FLAC Files (*.flac);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        qDebug() << "Selected file:" << fileName;
        m_currentFile = fileName;
        m_statusLabel->setText("File loaded: " + QFileInfo(fileName).fileName());
        statusBar()->showMessage("File loaded: " + fileName, 3000);
    } else {
        qDebug() << "No file selected";
    }
}

void MainWindow::onPlay()
{
    qDebug() << "=== onPlay() called ===";
    
    if (m_currentFile.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select a FLAC file first!");
        qDebug() << "Warning: No file selected for playback";
        return;
    }
    
    qDebug() << "Starting playback of:" << m_currentFile;
    m_statusLabel->setText("Status: Playing - " + QFileInfo(m_currentFile).fileName());
    statusBar()->showMessage("Playing: " + m_currentFile, 3000);
}

void MainWindow::onStop()
{
    qDebug() << "=== onStop() called ===";
    
    m_statusLabel->setText("Status: Stopped");
    statusBar()->showMessage("Playback stopped", 2000);
    qDebug() << "Playback stopped";
}

void MainWindow::onDebugInfo()
{
    qDebug() << "=== onDebugInfo() called ===";
    
    QString info = QString(
        "=== DEBUG INFORMATION ===\n"
        "Window Title: %1\n"
        "Window Size: %2x%3\n"
        "Current File: %4\n"
        "Qt Version: %5\n"
        "Widget Count: %6\n"
        "=========================="
    ).arg(windowTitle())
     .arg(width())
     .arg(height())
     .arg(m_currentFile.isEmpty() ? "None" : m_currentFile)
     .arg(QT_VERSION_STR)
     .arg(findChildren<QWidget*>().count());
    
    QMessageBox::information(this, "Debug Information", info);
    
    // Also print to console
    qDebug().noquote() << info;
}