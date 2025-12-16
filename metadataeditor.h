#ifndef METADATAEDITOR_H
#define METADATAEDITOR_H

#include <QString>
#include <QMap>
#include <QByteArray>
#include <QImage>
#include <QFile>
#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>

/**
 * @struct FlacMetadata
 * @brief Container for FLAC file metadata
 */
struct FlacMetadata {
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString year;
    QString genre;
    QString trackNumber;
    QString comment;
    QImage albumArt;
    
    // Technical info (read-only)
    int sampleRate = 0;
    int channels = 0;
    int bitsPerSample = 0;
    quint64 totalSamples = 0;
    
    FlacMetadata() = default;
};

/**
 * @class MetadataEditor
 * @brief Custom FLAC metadata reader/writer
 * 
 * Reads and writes FLAC metadata blocks directly without external libraries.
 * Supports Vorbis Comments and Picture blocks.
 */
class MetadataEditor
{
public:
    MetadataEditor();
    ~MetadataEditor();
    
 
     //reads metadata from the given file path and returns a FlacMetadata struct
    FlacMetadata readMetadata(const QString &filePath);
     //checks for valid lossless file
bool isValidFlacFile(const QString &filePath);
    //writes metadata to the given file path from a FlacMetadata struct
bool writeMetadata(const QString &filePath, const FlacMetadata &metadata);
    //updating specific field in the metadata
bool updateField(const QString &filePath, const QString &fieldName, const QString &value);
    //updatinf album art in the metadata
bool updateAlbumArt(const QString &filePath, const QImage &image);
    //removing albumArt from the metaD of the file 
bool removeAlbumArt(const QString &filePath);
        //returns last error message
    QString lastError() const { return m_lastError; }
    
//internernal structures and methods
private:
    
    struct MetadataBlock {
        quint8 blockType;
        bool isLast;
        quint32 length;
        QByteArray data;
    };
    
    // Reading helpers
    bool readFlacHeader(QFile &file);
    QList<MetadataBlock> readMetadataBlocks(QFile &file);
    FlacMetadata parseStreamInfo(const QByteArray &data);
    QMap<QString, QString> parseVorbisComment(const QByteArray &data);
    QImage parsePictureBlock(const QByteArray &data);
    
    // Writing helpers
    bool writeFlacFile(const QString &filePath, const QList<MetadataBlock> &blocks, const QByteArray &audioData);
    QByteArray createVorbisCommentBlock(const FlacMetadata &metadata, const QMap<QString, QString> &extraFields);
    QByteArray createPictureBlock(const QImage &image);
    
    // Utility helpers
    quint32 readBigEndian24(const QByteArray &data, int offset);
    quint32 readBigEndian32(const QByteArray &data, int offset);
    quint64 readBigEndian64(const QByteArray &data, int offset);
    void writeBigEndian24(QByteArray &data, quint32 value);
    void writeBigEndian32(QByteArray &data, quint32 value);
    
    // member vars
    QString m_lastError;
    
    // FLAC metadata block types
    static const quint8 BLOCK_TYPE_STREAMINFO = 0;
    static const quint8 BLOCK_TYPE_PADDING = 1;
    static const quint8 BLOCK_TYPE_APPLICATION = 2;
    static const quint8 BLOCK_TYPE_SEEKTABLE = 3;
    static const quint8 BLOCK_TYPE_VORBIS_COMMENT = 4;
    static const quint8 BLOCK_TYPE_CUESHEET = 5;
    static const quint8 BLOCK_TYPE_PICTURE = 6;
};

//UI dialog for metadata editing
class MetadataEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MetadataEditorDialog(const QString &filePath, QWidget *parent = nullptr);
    ~MetadataEditorDialog();

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onLoadAlbumArtClicked();
    void onRemoveAlbumArtClicked();

private:
    void setupUI();
    void loadMetadata();
    void updateAlbumArtDisplay();

    QString m_filePath;
    MetadataEditor m_editor;
    FlacMetadata m_metadata;

    // UI elements
    QLineEdit *m_titleEdit;
    QLineEdit *m_artistEdit;
    QLineEdit *m_albumEdit;
    QLineEdit *m_albumArtistEdit;
    QLineEdit *m_yearEdit;
    QLineEdit *m_genreEdit;
    QLineEdit *m_trackNumberEdit;
    QTextEdit *m_commentEdit;
    QLabel *m_albumArtLabel;
    QLabel *m_fileInfoLabel;
    QPushButton *m_loadArtButton;
    QPushButton *m_removeArtButton;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
};

#endif // METADATAEDITOR_H
