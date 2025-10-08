#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QAudioFormat> 
#include <filesystem>   //for file system operations
#include <thread>       //this is for multithreading
// Note: Removed QNetworkAccessManager and boost includes that may cause issues
// Add them back individually when needed

//remove the one you don't need later


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Debug: Print Qt and application info
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Application starting...";
    qDebug() << "Arguments:" << a.arguments();

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "flacplayer_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            qDebug() << "Loaded translation for:" << locale;
            break;
        }
    }

    // Test QAudioFormat and frequency
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    
    qDebug() << "Audio format created successfully";
    qDebug() << "Sample rate:" << format.sampleRate();
    qDebug() << "Channels:" << format.channelCount();

    qDebug() << "Creating main window...";
    MainWindow w;
    w.show();
    qDebug() << "Main window shown, entering event loop...";

    //filesystem operations
    std::filesystem::path testPath = std::filesystem::current_path();
    qDebug() << "Current working directory:" << QString::fromStdString(testPath.string());


    return a.exec();
}
