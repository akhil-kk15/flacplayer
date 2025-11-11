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

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

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
    QString getCodecName() const;
    int getSampleRate() const;
    int getChannels() const;
    int64_t getDuration() const; // in microseconds
    int64_t getPosition() const; // in microseconds
    QString getFormatInfo() const;
    PlaybackState getState() const;
    qreal getVolume() const;
    QPixmap getAlbumArt() const;

    // Debug information
    void printFileInfo() const;

signals:
    void fileOpened(const QString &fileName);
    void fileClosed();
    void errorOccurred(const QString &error);
    void stateChanged(PlaybackState state);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void albumArtChanged(const QPixmap &albumArt);

private slots:
    void updatePosition();
    void onAudioStateChanged(QAudio::State state);
    void decodeAudio();

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
    AudioBuffer *m_audioBuffer;
    QTimer *m_positionTimer;
    QTimer *m_decodeTimer;
    
    // Playback state
    PlaybackState m_state;
    qint64 m_currentPosition;
    qreal m_volume;
    
    // Audio processing
    AVFrame *m_frame;
    AVPacket *m_packet;
    uint8_t *m_convertedData;
    int m_convertedDataSize;
    bool m_shouldDecode;
    
    // Album art
    QPixmap m_albumArt;
    
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