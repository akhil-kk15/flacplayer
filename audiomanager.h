#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QString>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

class AudioManager : public QObject
{
    Q_OBJECT

public:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager();

    // Audio file operations
    bool openFile(const QString &filePath);
    void closeFile();
    bool isFileOpen() const;
    
    // Audio information
    QString getFileName() const;
    QString getCodecName() const;
    int getSampleRate() const;
    int getChannels() const;
    int64_t getDuration() const; // in microseconds
    QString getFormatInfo() const;

    // Debug information
    void printFileInfo() const;

signals:
    void fileOpened(const QString &fileName);
    void fileClosed();
    void errorOccurred(const QString &error);

private:
    AVFormatContext *m_formatContext;
    AVCodecContext *m_codecContext;
    const AVCodec *m_codec;
    int m_audioStreamIndex;
    QString m_currentFile;
    
    void cleanup();
    void initializeFFmpeg();
};

#endif // AUDIOMANAGER_H