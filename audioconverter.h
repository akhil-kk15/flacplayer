#ifndef AUDIOCONVERTER_H
#define AUDIOCONVERTER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QAtomicInteger>


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
}

// Constants
namespace ConverterConstants {
    constexpr int MP3_DEFAULT_FRAME_SIZE = 1152;  // Standard MP3 frame size
}




class AudioConverter : public QObject
{
    Q_OBJECT

public:
    enum BitratePreset {
        Bitrate_128 = 128000,
        Bitrate_192 = 192000,
        Bitrate_256 = 256000,
        Bitrate_320 = 320000
    };

    explicit AudioConverter(QObject *parent = nullptr);
    ~AudioConverter();

    void convertToMP3(const QString &inputPath, const QString &outputPath, BitratePreset bitrate);
    void cancel();

signals:
    void progressUpdated(int percentage);
    void conversionComplete(bool success, const QString &message);
    void conversionStarted();

private:
    bool m_cancelled;
    
    bool openInputFile(const QString &inputPath, AVFormatContext **inputFormatCtx);
    bool openOutputFile(const QString &outputPath, AVFormatContext **outputFormatCtx, 
                        AVCodecContext *inputCodecCtx, BitratePreset bitrate);
    bool convertAudio(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx,
                      AVCodecContext *inputCodecCtx, AVCodecContext *outputCodecCtx);
    void copyMetadata(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx);
};

class AudioConverterWorker : public QObject
{
    Q_OBJECT

public:
    AudioConverterWorker(const QString &inputPath, const QString &outputPath, 
                         AudioConverter::BitratePreset bitrate);

public slots:
    void process();

signals:
    void progressUpdated(int percentage);
    void finished(bool success, const QString &message);

private:
    QString m_inputPath;
    QString m_outputPath;
    AudioConverter::BitratePreset m_bitrate;
    AudioConverter *m_converter;
};







#endif // AUDIOCONVERTER_H
