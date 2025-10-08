#include "audiomanager.h"
#include <QFileInfo>

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_codec(nullptr)
    , m_audioStreamIndex(-1)
{
    qDebug() << "=== AudioManager Constructor ===";
    initializeFFmpeg();
}

AudioManager::~AudioManager()
{
    qDebug() << "=== AudioManager Destructor ===";
    cleanup();
}

void AudioManager::initializeFFmpeg()
{
    qDebug() << "Initializing FFmpeg...";
    
    qDebug() << "FFmpeg initialized successfully";
    qDebug() << "FFmpeg version:" << av_version_info();
}

bool AudioManager::openFile(const QString &filePath)
{
    qDebug() << "=== Opening audio file:" << filePath << "===";
    
    // Close any previously opened file
    closeFile();
    
    // Check if file exists
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QString error = QString("File does not exist: %1").arg(filePath);
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        return false;
    }
    
    // Allocate format context
    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) {
        QString error = "Could not allocate format context";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        return false;
    }
    
    // Open the file
    if (avformat_open_input(&m_formatContext, filePath.toLocal8Bit().data(), nullptr, nullptr) < 0) {
        QString error = QString("Could not open file: %1").arg(filePath);
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    // Find stream information
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        QString error = "Could not find stream information";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    // Find audio stream
    m_audioStreamIndex = -1;
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            break;
        }
    }
    
    if (m_audioStreamIndex == -1) {
        QString error = "No audio stream found in file";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    // Get codec parameters
    AVCodecParameters *codecParameters = m_formatContext->streams[m_audioStreamIndex]->codecpar;
    
    // Find decoder
    m_codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!m_codec) {
        QString error = "Codec not found";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext) {
        QString error = "Could not allocate codec context";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    // Copy codec parameters to context
    if (avcodec_parameters_to_context(m_codecContext, codecParameters) < 0) {
        QString error = "Could not copy codec parameters";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    // Open codec
    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0) {
        QString error = "Could not open codec";
        qDebug() << "ERROR:" << error;
        emit errorOccurred(error);
        cleanup();
        return false;
    }
    
    m_currentFile = filePath;
    
    qDebug() << "File opened successfully!";
    printFileInfo();
    
    emit fileOpened(fileInfo.fileName());
    return true;
}

void AudioManager::closeFile()
{
    if (isFileOpen()) {
        qDebug() << "=== Closing audio file ===";
        cleanup();
        emit fileClosed();
    }
}

bool AudioManager::isFileOpen() const
{
    return m_formatContext != nullptr;
}

QString AudioManager::getFileName() const
{
    if (!isFileOpen()) return QString();
    return QFileInfo(m_currentFile).fileName();
}

QString AudioManager::getCodecName() const
{
    if (!m_codec) return QString();
    return QString(m_codec->long_name);
}

int AudioManager::getSampleRate() const
{
    if (!m_codecContext) return 0;
    return m_codecContext->sample_rate;
}

int AudioManager::getChannels() const
{
    if (!m_codecContext) return 0;
#if LIBAVUTIL_VERSION_MAJOR >= 57
    return m_codecContext->ch_layout.nb_channels;
#else
    return m_codecContext->channels;
#endif
}

int64_t AudioManager::getDuration() const
{
    if (!m_formatContext) return 0;
    return m_formatContext->duration;
}

QString AudioManager::getFormatInfo() const
{
    if (!isFileOpen()) return QString();
    
    return QString(
        "File: %1\n"
        "Format: %2\n"
        "Codec: %3\n"
        "Sample Rate: %4 Hz\n"
        "Channels: %5\n"
        "Duration: %6 seconds\n"
        "Bitrate: %7 kbps"
    ).arg(getFileName())
     .arg(m_formatContext->iformat->long_name)
     .arg(getCodecName())
     .arg(getSampleRate())
     .arg(getChannels())
     .arg(getDuration() / 1000000.0, 0, 'f', 2)
     .arg(m_formatContext->bit_rate / 1000);
}

void AudioManager::printFileInfo() const
{
    if (!isFileOpen()) {
        qDebug() << "No file is currently open";
        return;
    }
    
    qDebug() << "=== AUDIO FILE INFORMATION ===";
    qDebug() << "File:" << getFileName();
    qDebug() << "Format:" << m_formatContext->iformat->long_name;
    qDebug() << "Codec:" << getCodecName();
    qDebug() << "Sample Rate:" << getSampleRate() << "Hz";
    qDebug() << "Channels:" << getChannels();
    qDebug() << "Duration:" << (getDuration() / 1000000.0) << "seconds";
    qDebug() << "Bitrate:" << (m_formatContext->bit_rate / 1000) << "kbps";
    qDebug() << "===============================";
}

void AudioManager::cleanup()
{
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
    
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }
    
    m_codec = nullptr;
    m_audioStreamIndex = -1;
    m_currentFile.clear();
}