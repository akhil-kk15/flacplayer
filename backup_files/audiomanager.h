#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <QAudioOutput>
#include <QIODevice>
#include <QTimer>
#include <QBuffer>
#include <QByteArray>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QImage>
#include <QAtomicInt>

// Forward declarations for Qt 6
class QAudioSink;
class QMediaDevices;
class QAudioDevice;

// Constants
namespace AudioConstants {
    constexpr int AUDIO_BUFFER_LIMIT = 32768;      // 32KB buffer limit
    constexpr int DEFAULT_FRAME_SIZE = 4096;        // Default audio frame size
    constexpr int DECODE_TIMER_MS = 10;             // Decode timer interval in milliseconds
    constexpr int POSITION_UPDATE_MS = 100;         // Position update interval in milliseconds
}

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

struct AudioMetadata
{
    QString title;
    QString artist;
    QString album;
    QString year;
    QString genre;
    QString comment;
    
    bool isEmpty() const {
        return title.isEmpty() && artist.isEmpty() && album.isEmpty();
    }
};

class AudioBuffer : public QIODevice
{
    Q_OBJECT

public:
    explicit AudioBuffer(QObject *parent = nullptr);
    ~AudioBuffer();

    void appendData(const QByteArray &data);
    void clearBuffer();
    bool hasData() const;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QByteArray m_buffer;
    qint64 m_position;
    mutable QMutex m_mutex;
};

class AudioManager : public QObject
{
    Q_OBJECT

public:
    enum PlaybackState {
        StoppedState,
        PlayingState,
        PausedState
    };

    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager();

    // Audio file operations
    bool openFile(const QString &filePath);
    void closeFile();
    bool isFileOpen() const;
    
    // Playback control
    void play();
    void pause();
    void stop();
    void setVolume(qreal volume); // 0.0 to 1.0
    void setPosition(qint64 position); // in microseconds
    
    // Audio information
    QString getFileName() const;
    QString getFormatName() const;
    QString getCodecName() const;
    int getSampleRate() const;
    int getChannels() const;
    qint64 getBitrate() const;
    int64_t getDuration() const; // in microseconds
    int64_t getPosition() const; // in microseconds
    QString getFormatInfo() const;
    PlaybackState getState() const;
    qreal getVolume() const;
    QPixmap getAlbumArt() const;

    // Debug information
    void printFileInfo() const;
    
    // Metadata
    AudioMetadata getMetadata() const;

signals:
    void fileOpened(const QString &fileName);
    void fileClosed();
    void errorOccurred(const QString &error);
    void stateChanged(PlaybackState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void albumArtChanged(const QPixmap &albumArt);
    void trackFinished();
    void metadataChanged(const AudioMetadata &metadata);

private slots:
    void updatePosition();
    void onAudioStateChanged(QAudio::State state);
    void decodeAudio();

private:
    void extractMetadata();
    
private:
    // FFmpeg components
    AVFormatContext *m_formatContext;
    AVCodecContext *m_codecContext;
    const AVCodec *m_codec;
    SwrContext *m_swrContext;
    int m_audioStreamIndex;
    QString m_currentFile;
    
    // Qt Audio components
    QAudioOutput *m_audioOutput;
    QAudioSink *m_audioSink;
    AudioBuffer *m_audioBuffer;
    QTimer *m_positionTimer;
    QTimer *m_decodeTimer;
    
    // Playback state
    PlaybackState m_state;
    qint64 m_currentPosition;
    qreal m_volume;
    mutable QMutex m_stateMutex;  // Protects m_currentPosition and m_shouldDecode
    
    // Audio processing
    AVFrame *m_frame;
    AVPacket *m_packet;
    uint8_t *m_convertedData;
    int m_convertedDataSize;
    QAtomicInt m_shouldDecode;  // Thread-safe flag for decoding control
    qint64 m_lastPacketPts;     // Last packet PTS for accurate position tracking
    qint64 m_playbackStartTime; // Time when playback started (for calculating elapsed time)
    qint64 m_pausePosition;     // Position when paused
    QAtomicInt m_ignorePackets; // Ignore first few packets after seek (stale data)
    
    // Album art
    QPixmap m_albumArt;
    
    // Metadata
    AudioMetadata m_metadata;
    
    void cleanup();
    void initializeFFmpeg();
    void setupAudioOutput();
    void cleanupAudioOutput();
    void initializeDecoding();
    void cleanupDecoding();
    bool decodePacket();
    void setDecodingActive(bool active);
    void extractAlbumArt();
};

#endif // AUDIOMANAGER_H