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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "=== MainWindow Constructor START ===";
    
    ui->setupUi(this);
    
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
    setWindowTitle("FLAC Player v1.0 - Learning Qt");
    setMinimumSize(600, 400);
    resize(800, 500);
    
    // Status bar message
    statusBar()->showMessage("Welcome to Qt! Ready to learn...", 3000);
    
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
    QLabel *welcomeLabel = new QLabel("Welcome to Qt Development!", this);
    welcomeLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px; color: blue;");
    layout->addWidget(welcomeLabel);
    
    // Add buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_testButton = new QPushButton("Click Me!", this);
    m_debugButton = new QPushButton("Show Debug Info", this);
    
    buttonLayout->addWidget(m_testButton);
    buttonLayout->addWidget(m_debugButton);
    buttonLayout->addStretch(); // This pushes buttons to the left
    
    layout->addLayout(buttonLayout);
    
    // Add status label
    m_statusLabel = new QLabel("Status: Ready to learn Qt!", this);
    m_statusLabel->setStyleSheet("border: 1px solid gray; padding: 5px; background-color: lightgray;");
    layout->addWidget(m_statusLabel);
    
    layout->addStretch(); // This pushes everything to the top
    
    // Connect signals to slots (this is how Qt handles events!)
    connect(m_testButton, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    connect(m_debugButton, &QPushButton::clicked, this, &MainWindow::onDebugInfo);
    
    qDebug() << "Basic UI setup complete";
}

// This is called when the "Click Me!" button is pressed
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
            "You've clicked the button 5 times! You're learning Qt! 🎉");
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
        "Learning Tips:\n"
        "  • Check the terminal for debug output!\n"
        "  • Try setting breakpoints in VS Code\n"
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
