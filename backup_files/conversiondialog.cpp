#include "conversiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

// ConversionDialog implementation

ConversionDialog::ConversionDialog(const QString &inputFile, QWidget *parent)
    : QDialog(parent)
    , m_inputFile(inputFile)//stores input file path
    , m_workerThread(nullptr)// initializing worker thread pointer to nullptr
    , m_worker(nullptr) //worker null
    , m_converting(false) //flag to indicate conversion state
{
    setupUI();
    setWindowTitle("Convert to MP3");
    resize(500, 250);
}

ConversionDialog::~ConversionDialog()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
    }
}

void ConversionDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Input file display
    QFormLayout *formLayout = new QFormLayout();
    
    QFileInfo inputInfo(m_inputFile);
    m_inputLabel = new QLabel(inputInfo.fileName());
    m_inputLabel->setStyleSheet("font-weight: bold;");
    formLayout->addRow("Input File:", m_inputLabel);
    
    // Output file selection
    QHBoxLayout *outputLayout = new QHBoxLayout();
    m_outputEdit = new QLineEdit();
    m_outputEdit->setText(generateOutputPath(m_inputFile));
    m_browseButton = new QPushButton("Browse...");
    outputLayout->addWidget(m_outputEdit);
    outputLayout->addWidget(m_browseButton);
    formLayout->addRow("Output File:", outputLayout);
    
    // Bitrate selection
    m_bitrateCombo = new QComboBox();
    m_bitrateCombo->addItem("128 kbps", AudioConverter::Bitrate_128);
    m_bitrateCombo->addItem("192 kbps", AudioConverter::Bitrate_192);
    m_bitrateCombo->addItem("256 kbps", AudioConverter::Bitrate_256);
    m_bitrateCombo->addItem("320 kbps", AudioConverter::Bitrate_320);
    m_bitrateCombo->setCurrentIndex(3); // Default to 320 kbps
    formLayout->addRow("Bitrate:", m_bitrateCombo);
    
    mainLayout->addLayout(formLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100); 
    m_progressBar->setValue(0);//intial value 0
    mainLayout->addWidget(m_progressBar);
    

    
    // Status label
    m_statusLabel = new QLabel("Ready to convert");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_convertButton = new QPushButton("Convert");
    m_convertButton->setDefault(true);
    m_convertButton->setMinimumWidth(100);
    
    m_cancelButton = new QPushButton("Close");
    m_cancelButton->setMinimumWidth(100);
    
    buttonLayout->addWidget(m_convertButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_browseButton, &QPushButton::clicked, this, &ConversionDialog::onBrowseClicked);
    connect(m_convertButton, &QPushButton::clicked, this, &ConversionDialog::onConvertClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ConversionDialog::onCancelClicked);
}

QString ConversionDialog::generateOutputPath(const QString &inputPath)
{
    QFileInfo fileInfo(inputPath);
    QString baseName = fileInfo.baseName();
    QString dirPath = fileInfo.absolutePath();
    
    return dirPath + "/" + baseName + ".mp3";
}

void ConversionDialog::onBrowseClicked()
{
    QString outputPath = QFileDialog::getSaveFileName(
        this,
        "Save MP3 File",
        m_outputEdit->text(),
        "MP3 Files (*.mp3);;All Files (*)"
    );
    
    if (!outputPath.isEmpty()) {
        m_outputEdit->setText(outputPath);
    }
}

void ConversionDialog::onConvertClicked()
{
    if (m_converting) {
        return;
    }
    
    QString outputPath = m_outputEdit->text().trimmed();
    if (outputPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please specify an output file path");
        return;
    }
    
    // Check if output file already exists
    if (QFile::exists(outputPath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "File Exists",
            "Output file already exists. Overwrite?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply != QMessageBox::Yes) {
            return;
        }
        
        QFile::remove(outputPath);
    }
    
    m_converting = true;
    m_progressBar->setValue(0);
    m_statusLabel->setText("Converting...");
    m_convertButton->setEnabled(false);
    m_browseButton->setEnabled(false);
    m_outputEdit->setEnabled(false);
    m_bitrateCombo->setEnabled(false);
    m_cancelButton->setText("Cancel");
    
    // Get selected bitrate
    AudioConverter::BitratePreset bitrate = 
        static_cast<AudioConverter::BitratePreset>(m_bitrateCombo->currentData().toInt());
    
    // Create worker thread
    m_workerThread = new QThread();
    m_worker = new AudioConverterWorker(m_inputFile, outputPath, bitrate);
    m_worker->moveToThread(m_workerThread);
    
    connect(m_workerThread, &QThread::started, m_worker, &AudioConverterWorker::process);
    connect(m_worker, &AudioConverterWorker::progressUpdated, this, &ConversionDialog::onProgressUpdated);
    connect(m_worker, &AudioConverterWorker::finished, this, &ConversionDialog::onConversionFinished);
    connect(m_worker, &AudioConverterWorker::finished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &AudioConverterWorker::deleteLater);
    
    m_workerThread->start();
}

void ConversionDialog::onCancelClicked()
{
    if (m_converting) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Cancel Conversion",
            "Are you sure you want to cancel the conversion?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::Yes) {
            m_statusLabel->setText("Cancelling...");
            m_cancelButton->setEnabled(false);
            // The worker will be cancelled via the AudioConverter
        }
    } else {
        reject();
    }
}

void ConversionDialog::onProgressUpdated(int percentage)
{
    m_progressBar->setValue(percentage);
    m_statusLabel->setText(QString("Converting... %1%").arg(percentage));
}

void ConversionDialog::onConversionFinished(bool success, const QString &message)
{
    m_converting = false;
    m_convertButton->setEnabled(true);
    m_browseButton->setEnabled(true);
    m_outputEdit->setEnabled(true);
    m_bitrateCombo->setEnabled(true);
    m_cancelButton->setText("Close");
    m_cancelButton->setEnabled(true);
    
    if (success) {
        m_progressBar->setValue(100);
        m_statusLabel->setText("Conversion completed successfully!");
        
        QMessageBox::information(this, "Success", message);
        accept();
    } else {
        m_progressBar->setValue(0);
        m_statusLabel->setText("Conversion failed");
        QMessageBox::critical(this, "Error", message);
    }
    
    // Cleanup thread
    if (m_workerThread) {
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
        m_worker = nullptr;
    }
}
