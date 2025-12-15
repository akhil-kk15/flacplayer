#include "audiomanager.h"
#include <QFileInfo>
#include <QAudioFormat>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudiosink.h>

// AudioBuffer implementation
AudioBuffer::AudioBuffer(QObject *parent)
    : QIODevice(parent), m_position(0)
{
    open(QIODevice::ReadOnly);
}

AudioBuffer::~AudioBuffer()
{
    close();
}

void AudioBuffer::appendData(const QByteArray &data)
{
    QMutexLocker locker(&m_mutex);
    m_buffer.append(data);
}

void AudioBuffer::clearBuffer()
{
    QMutexLocker locker(&m_mutex);
    m_buffer.clear();
    m_position = 0;
    // Reset QIODevice position
    seek(0);
}

bool AudioBuffer::hasData() const
{
    QMutexLocker locker(&m_mutex);
    return m_position < m_buffer.size();
}

qint64 AudioBuffer::readData(char *data, qint64 maxlen)
{
    QMutexLocker locker(&m_mutex);
    
    qint64 available = m_buffer.size() - m_position;
    qint64 readSize = qMin(maxlen, available);
    
    if (readSize > 0) {
        memcpy(data, m_buffer.constData() + m_position, readSize);
        m_position += readSize;
    }
    
    return readSize;
}

qint64 AudioBuffer::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1; // Read-only device
}

// AudioManager implementation
AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_codec(nullptr)
    , m_swrContext(nullptr)
    , m_audioStreamIndex(-1)
    , m_audioOutput(nullptr)
    , m_audioSink(nullptr)
    , m_audioBuffer(nullptr)
    , m_positionTimer(new QTimer(this))
    , m_decodeTimer(new QTimer(this))
    , m_state(StoppedState)
    , m_currentPosition(0)
    , m_volume(1.0)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_convertedData(nullptr)
    , m_convertedDataSize(0)
    , m_shouldDecode(0)
    , m_lastPacketPts(0)
    , m_playbackStartTime(0)
    , m_pausePosition(0)
{
    initializeFFmpeg();
    
    // Setup position timer
    connect(m_positionTimer, &QTimer::timeout, this, &AudioManager::updatePosition);
    m_positionTimer->setInterval(AudioConstants::POSITION_UPDATE_MS);
    
    // Setup decode timer
    connect(m_decodeTimer, &QTimer::timeout, this, &AudioManager::decodeAudio);
    m_decodeTimer->setInterval(AudioConstants::DECODE_TIMER_MS);
}

AudioManager::~AudioManager()
{
    stop();
    cleanup();
    cleanupDecoding();
}

void AudioManager::initializeFFmpeg()
{
}

bool AudioManager::openFile(const QString &filePath)
{
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
    setupAudioOutput();
    initializeDecoding();
    extractAlbumArt();
    extractMetadata();
    emit fileOpened(fileInfo.fileName());
    emit durationChanged(getDuration());
    return true;
}

void AudioManager::closeFile()
{
    if (isFileOpen()) {
        stop();
        cleanup();
        emit fileClosed();
    }
}

// Playback control methods
void AudioManager::play()
{
    if (!isFileOpen()) {
        return;
    }
    
    if (m_state == PausedState && m_audioSink) {
        m_audioSink->resume();
        m_state = PlayingState;
        m_positionTimer->start();
        m_decodeTimer->start();
        m_shouldDecode = true;
        emit stateChanged(m_state);
        return;
    }
    
    if (m_state == PlayingState) {
        return;
    }
    
    if (!m_audioSink || !m_audioBuffer) {
        return;
    }
    
    m_audioBuffer->clearBuffer();
    
    if (m_audioSink->state() != QAudio::ActiveState) {
        m_audioSink->start(m_audioBuffer);
    }
    
    // Start decoding
    m_shouldDecode.storeRelaxed(1);
    m_state = PlayingState;
    {
        QMutexLocker locker(&m_stateMutex);
        m_playbackStartTime = QDateTime::currentMSecsSinceEpoch();
        m_currentPosition = m_pausePosition; // Resume from pause position
    }
    m_positionTimer->start();
    m_decodeTimer->start();
    emit stateChanged(m_state);
}

void AudioManager::pause()
{
    if (m_state != PlayingState) {
        return;
    }
    
    m_shouldDecode.storeRelaxed(0);
    m_decodeTimer->stop();
    if (m_audioSink) {
        m_audioSink->suspend();
    }
    {
        QMutexLocker locker(&m_stateMutex);
        m_pausePosition = m_currentPosition;
    }
    m_state = PausedState;
    m_positionTimer->stop();
    emit stateChanged(m_state);
}

void AudioManager::stop()
{
    if (m_state == StoppedState) {
        return;
    }
    
    m_shouldDecode.storeRelaxed(0);
    m_decodeTimer->stop();
    m_positionTimer->stop();
    if (m_audioSink) {
        m_audioSink->stop();
    }
    if (m_audioBuffer) {
        m_audioBuffer->clearBuffer();
    }
    // Reset to beginning of file
    if (m_formatContext) {
        av_seek_frame(m_formatContext, m_audioStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
        if (m_codecContext) {
            avcodec_flush_buffers(m_codecContext);
        }
    }
    {
        QMutexLocker locker(&m_stateMutex);
        m_currentPosition = 0;
        m_lastPacketPts = 0;
        m_playbackStartTime = 0;
        m_pausePosition = 0;
    }
    m_state = StoppedState;
    emit stateChanged(m_state);
    emit positionChanged(m_currentPosition);
}

void AudioManager::setVolume(qreal volume)
{
    m_volume = qBound(0.0, volume, 1.0);
    
    if (m_audioOutput) {
        m_audioOutput->setVolume(m_volume);
    }
}

void AudioManager::setPosition(qint64 position)
{
    if (!isFileOpen() || m_audioStreamIndex < 0) {
        return;
    }
    
    // Clamp position to valid range
    qint64 duration = getDuration();
    position = qBound(0LL, position, duration);
    
    // Convert microseconds to stream timebase
    AVStream *stream = m_formatContext->streams[m_audioStreamIndex];
    int64_t seekTarget = av_rescale_q(position, AV_TIME_BASE_Q, stream->time_base);
    
    // Seek in the stream
    if (av_seek_frame(m_formatContext, m_audioStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD) < 0) {
        qDebug() << "ERROR: Failed to seek to position:" << position;
        return;
    }
    
    // Flush codec buffers to discard old packets
    if (m_codecContext) {
        avcodec_flush_buffers(m_codecContext);
    }
    
    // Clear audio buffer
    if (m_audioBuffer) {
        m_audioBuffer->clearBuffer();
    }
    // Ignore next few packets after seek
    m_ignorePackets.storeRelaxed(10);
    // Update current position
    {
        QMutexLocker locker(&m_stateMutex);
        m_currentPosition = position;
        m_pausePosition = position;
        m_lastPacketPts = seekTarget;  // Update PTS tracking
        // Reset playback start time for accurate position updates
        if (m_state == PlayingState) {
            m_playbackStartTime = QDateTime::currentMSecsSinceEpoch();
        }
    }
    
    emit positionChanged(m_currentPosition);
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

QString AudioManager::getFormatName() const
{
    if (!isFileOpen() || !m_formatContext || !m_formatContext->iformat) return QString();
    return QString(m_formatContext->iformat->long_name);
}

QString AudioManager::getCodecName() const
{
    if (!m_codec) return QString();
    return QString(m_codec->long_name);
}

qint64 AudioManager::getBitrate() const
{
    if (!isFileOpen() || !m_formatContext) return 0;
    return m_formatContext->bit_rate;
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

int64_t AudioManager::getPosition() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_currentPosition;
}

AudioManager::PlaybackState AudioManager::getState() const
{
    return m_state;
}

qreal AudioManager::getVolume() const
{
    return m_volume;
}

QPixmap AudioManager::getAlbumArt() const
{
    return m_albumArt;
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

void AudioManager::cleanup()
{
    cleanupAudioOutput();
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

// New helper methods
void AudioManager::setupAudioOutput()
{
    if (!isFileOpen()) {
        return;
    }
    
    cleanupAudioOutput();
    
    QAudioFormat format;
    format.setSampleRate(getSampleRate());
    format.setChannelCount(getChannels());
    format.setSampleFormat(QAudioFormat::Int16);
    
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        qDebug() << "ERROR: No audio output device available";
        emit errorOccurred("No audio output device available");
        return;
    }
    
    m_audioOutput = new QAudioOutput(device, this);
    m_audioOutput->setVolume(m_volume);
    
    m_audioSink = new QAudioSink(format, this);
    connect(m_audioSink, &QAudioSink::stateChanged, this, &AudioManager::onAudioStateChanged);
    
    m_audioBuffer = new AudioBuffer(this);
}

void AudioManager::cleanupAudioOutput()
{
    if (m_audioSink) {
        m_audioSink->stop();
        m_audioSink->deleteLater();
        m_audioSink = nullptr;
    }
    
    if (m_audioOutput) {
        m_audioOutput->deleteLater();
        m_audioOutput = nullptr;
    }
    
    if (m_audioBuffer) {
        m_audioBuffer->deleteLater();
        m_audioBuffer = nullptr;
    }
}

void AudioManager::updatePosition()
{
    if (m_state == PlayingState) {
        QMutexLocker locker(&m_stateMutex);
        if (m_playbackStartTime > 0) {
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            qint64 elapsedMs = currentTime - m_playbackStartTime;
            m_currentPosition = m_pausePosition + (elapsedMs * 1000);
        }
        // Don't exceed duration
        qint64 duration = getDuration();
        if (duration > 0 && m_currentPosition >= duration) {
            m_currentPosition = duration;
            locker.unlock();
            stop();
            return;
        }
        locker.unlock();
        emit positionChanged(m_currentPosition);
    }
}

void AudioManager::onAudioStateChanged(QAudio::State state)
{
    switch (state) {
    case QAudio::StoppedState:
        if (m_state == PlayingState) {
            if (m_audioSink && m_audioSink->error() != QAudio::NoError) {
                QString error = QString("Audio playback error: %1").arg(m_audioSink->error());
                emit errorOccurred(error);
            }
            stop();
        }
        break;
    case QAudio::ActiveState:
        break;
    case QAudio::SuspendedState:
        break;
    case QAudio::IdleState:
        // If decoding is stopped (EOF reached) and buffer is empty, track is finished
        if (!m_shouldDecode && m_state == PlayingState) {
            stop();
            emit trackFinished();
        }
        break;
    }
}

// New audio decoding methods
void AudioManager::initializeDecoding()
{
    if (!m_codecContext) {
        return;
    }
    
    // Allocate frame and packet
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    
    if (!m_frame || !m_packet) {
        qDebug() << "Failed to allocate frame or packet";
        return;
    }
    
    // Setup resampler for converting to 16-bit PCM
    int ret = swr_alloc_set_opts2(
        &m_swrContext,
        &m_codecContext->ch_layout,     // Output channel layout
        AV_SAMPLE_FMT_S16,              // Output sample format (16-bit)
        getSampleRate(),                // Output sample rate
        &m_codecContext->ch_layout,     // Input channel layout
        m_codecContext->sample_fmt,     // Input sample format
        m_codecContext->sample_rate,    // Input sample rate
        0, nullptr
    );
    
    if (ret < 0 || swr_init(m_swrContext) < 0) {
        qDebug() << "Failed to initialize resampler";
        return;
    }
    // Allocate buffer for converted audio
    m_convertedDataSize = av_samples_get_buffer_size(nullptr, getChannels(), AudioConstants::DEFAULT_FRAME_SIZE, AV_SAMPLE_FMT_S16, 1);
    m_convertedData = (uint8_t*)av_malloc(m_convertedDataSize);
}

void AudioManager::cleanupDecoding()
{
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    
    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }
    
    if (m_convertedData) {
        av_free(m_convertedData);
        m_convertedData = nullptr;
    }
}

void AudioManager::decodeAudio()
{
    if (m_shouldDecode.loadRelaxed() == 0 || !m_formatContext || !m_codecContext || !m_audioBuffer) {
        return;
    }
    // Don't decode if buffer is too full
    if (m_audioBuffer->hasData() && m_audioBuffer->bytesAvailable() > AudioConstants::AUDIO_BUFFER_LIMIT) {
        return;
    }
    // Try to decode some packets
    for (int i = 0; i < 5; ++i) {
        if (!decodePacket()) {
            break;
        }
    }
}

bool AudioManager::decodePacket()
{
    if (!m_formatContext || !m_codecContext || !m_packet || !m_frame) {
        return false;
    }
    
    // Read packet from file
    int ret = av_read_frame(m_formatContext, m_packet);
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            avcodec_send_packet(m_codecContext, nullptr);
            setDecodingActive(false);
        } else {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, sizeof(errbuf));
            qDebug() << "Error reading frame:" << errbuf;
        }
        return false;
    }
    // Skip non-audio packets
    if (m_packet->stream_index != m_audioStreamIndex) {
        av_packet_unref(m_packet);
        return true;
    }
    // Ignore packets immediately after seek
    if (m_ignorePackets.loadRelaxed() > 0) {
        m_ignorePackets.fetchAndSubRelaxed(1);
        av_packet_unref(m_packet);
        return true;
    }
    // Track packet PTS for position calculation
    if (m_packet->pts != AV_NOPTS_VALUE) {
        QMutexLocker locker(&m_stateMutex);
        m_lastPacketPts = m_packet->pts;
    }
    // Send packet to decoder
    ret = avcodec_send_packet(m_codecContext, m_packet);
    av_packet_unref(m_packet);
    
    if (ret < 0) {
        qDebug() << "Error sending packet to decoder";
        return false;
    }
    // Receive decoded frames
    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            qDebug() << "Error receiving frame from decoder";
            return false;
        }
        // Convert audio to target format
        if (m_swrContext && m_convertedData) {
            int converted_samples = swr_convert(
                m_swrContext,
                &m_convertedData, m_convertedDataSize / (getChannels() * 2),
                (const uint8_t**)m_frame->data, m_frame->nb_samples
            );
            if (converted_samples > 0) {
                int converted_size = converted_samples * getChannels() * 2;
                QByteArray audioData((const char*)m_convertedData, converted_size);
                m_audioBuffer->appendData(audioData);
            }
        }
        av_frame_unref(m_frame);
    }
    return true;
}

void AudioManager::setDecodingActive(bool active)
{
    m_shouldDecode.storeRelaxed(active ? 1 : 0);
    if (active) {
        if (!m_decodeTimer->isActive()) {
            m_decodeTimer->start(AudioConstants::DECODE_TIMER_MS);
        }
    } else {
        m_decodeTimer->stop();
    }
}

void AudioManager::extractAlbumArt()
{
    if (!m_formatContext) {
        return;
    }
    
    m_albumArt = QPixmap();
    // Look for attached picture stream
    for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
        AVStream *stream = m_formatContext->streams[i];
        // Check if this is an attached picture
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket *pkt = &stream->attached_pic;
            
            if (pkt->data && pkt->size > 0) {
                // Create QImage from raw packet data
                QImage image = QImage::fromData(pkt->data, pkt->size);
                
                if (!image.isNull()) {
                    // Convert to QPixmap and store
                    m_albumArt = QPixmap::fromImage(image);
                    emit albumArtChanged(m_albumArt);
                    return;
                }
            }
        }
    }
}

void AudioManager::extractMetadata()
{
    if (!m_formatContext) {
        return;
    }
    
    m_metadata = AudioMetadata();
    AVDictionary *metadata = m_formatContext->metadata;
    
    if (metadata) {
        AVDictionaryEntry *tag = nullptr;
        // Extract title
        tag = av_dict_get(metadata, "title", nullptr, 0);
        if (tag && tag->value) {
            m_metadata.title = QString::fromUtf8(tag->value);
        }
        // Extract artist
        tag = av_dict_get(metadata, "artist", nullptr, 0);
        if (tag && tag->value) {
            m_metadata.artist = QString::fromUtf8(tag->value);
        }
        // Extract album
        tag = av_dict_get(metadata, "album", nullptr, 0);
        if (tag && tag->value) {
            m_metadata.album = QString::fromUtf8(tag->value);
        }
        // Extract year/date
        tag = av_dict_get(metadata, "date", nullptr, 0);
        if (!tag) {
            tag = av_dict_get(metadata, "year", nullptr, 0);
        }
        if (tag && tag->value) {
            m_metadata.year = QString::fromUtf8(tag->value);
        }
        // Extract genre
        tag = av_dict_get(metadata, "genre", nullptr, 0);
        if (tag && tag->value) {
            m_metadata.genre = QString::fromUtf8(tag->value);
        }
        // Extract comment
        tag = av_dict_get(metadata, "comment", nullptr, 0);
        if (tag && tag->value) {
            m_metadata.comment = QString::fromUtf8(tag->value);
        }
    }
    
    emit metadataChanged(m_metadata);
}

AudioMetadata AudioManager::getMetadata() const
{
    return m_metadata;
}