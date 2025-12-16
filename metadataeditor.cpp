#include "metadataeditor.h"
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>

MetadataEditor::MetadataEditor()
{
    // qDebug() << "[MetadataEditor] Constructor called";
}

MetadataEditor::~MetadataEditor()
{
    // qDebug() << "[MetadataEditor] Destructor called";
}



bool MetadataEditor::isValidFlacFile(const QString &filePath)
{
    // qDebug() << "[MetadataEditor] isValidFlacFile:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        // qDebug() << "[MetadataEditor] ERROR: Cannot open file";
        return false;
    }
    
    bool valid = readFlacHeader(file);
    // qDebug() << "[MetadataEditor] File is valid:" << valid;
    return valid;
}

FlacMetadata MetadataEditor::readMetadata(const QString &filePath)
{
    qDebug() << "[MetadataEditor] readMetadata called for:" << filePath;
    FlacMetadata metadata;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        qDebug() << "[MetadataEditor] ERROR: Cannot open file for reading";
        return metadata;
    }
    
    // Verify FLAC header
    if (!readFlacHeader(file)) {
        m_lastError = "Invalid FLAC file format";
        qDebug() << "[MetadataEditor] ERROR: Invalid FLAC header";
        return metadata;
    }
    qDebug() << "[MetadataEditor] FLAC header verified";
    
    // Read all metadata blocks
    QList<MetadataBlock> blocks = readMetadataBlocks(file);
    qDebug() << "[MetadataEditor] Read" << blocks.size() << "metadata blocks";
    
    // Parse each block type
    for (const MetadataBlock &block : blocks) {
        qDebug() << "[MetadataEditor] Block type:" << block.blockType 
                 << "Length:" << block.length 
                 << "IsLast:" << block.isLast;
        switch (block.blockType) {
            case BLOCK_TYPE_STREAMINFO: {
                FlacMetadata streamInfo = parseStreamInfo(block.data);
                metadata.sampleRate = streamInfo.sampleRate;
                metadata.channels = streamInfo.channels;
                metadata.bitsPerSample = streamInfo.bitsPerSample;
                metadata.totalSamples = streamInfo.totalSamples;
                break;
            }
            case BLOCK_TYPE_VORBIS_COMMENT: {
                QMap<QString, QString> comments = parseVorbisComment(block.data);
                metadata.title = comments.value("TITLE", "");
                metadata.artist = comments.value("ARTIST", "");
                metadata.album = comments.value("ALBUM", "");
                metadata.albumArtist = comments.value("ALBUMARTIST", comments.value("ALBUM ARTIST", ""));
                metadata.year = comments.value("DATE", comments.value("YEAR", ""));
                metadata.genre = comments.value("GENRE", "");
                metadata.trackNumber = comments.value("TRACKNUMBER", comments.value("TRACK", ""));
                metadata.comment = comments.value("COMMENT", comments.value("DESCRIPTION", ""));
                break;
            }
            case BLOCK_TYPE_PICTURE: {
                metadata.albumArt = parsePictureBlock(block.data);
                break;
            }
        }
    }
    
    file.close();
    m_lastError.clear();
    qDebug() << "[MetadataEditor] Successfully read metadata -" 
             << "Title:" << metadata.title 
             << "Artist:" << metadata.artist
             << "Album:" << metadata.album;
    return metadata;
}



bool MetadataEditor::writeMetadata(const QString &filePath, const FlacMetadata &metadata)
{
    qDebug() << "[MetadataEditor] writeMetadata called for:" << filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file for reading: " + filePath;
        qDebug() << "[MetadataEditor] ERROR: Cannot open file for writing";
        return false;
    }
    
    // Verify FLAC header
    if (!readFlacHeader(file)) {
        m_lastError = "Invalid FLAC file format";
        file.close();
        return false;
    }
    
    // Read existing metadata and audio data
    QList<MetadataBlock> blocks = readMetadataBlocks(file);
    QByteArray audioData = file.readAll();
    file.close();
    
    // Update or create Vorbis Comment block
    bool hasVorbisComment = false;
    QMap<QString, QString> extraFields;
    
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].blockType == BLOCK_TYPE_VORBIS_COMMENT) {
            // Preserve extra fields not in our structure
            QMap<QString, QString> existing = parseVorbisComment(blocks[i].data);
            for (auto it = existing.constBegin(); it != existing.constEnd(); ++it) {
                QString key = it.key().toUpper();
                if (key != "TITLE" && key != "ARTIST" && key != "ALBUM" && 
                    key != "ALBUMARTIST" && key != "ALBUM ARTIST" && 
                    key != "DATE" && key != "YEAR" && key != "GENRE" && 
                    key != "TRACKNUMBER" && key != "TRACK" && 
                    key != "COMMENT" && key != "DESCRIPTION") {
                    extraFields[it.key()] = it.value();
                }
            }
            
            // Replace with new data
            blocks[i].data = createVorbisCommentBlock(metadata, extraFields);
            blocks[i].length = blocks[i].data.size();
            hasVorbisComment = true;
            break;
        }
    }
    
    // Add Vorbis Comment if it doesn't exist
    if (!hasVorbisComment) {
        MetadataBlock vorbisBlock;
        vorbisBlock.blockType = BLOCK_TYPE_VORBIS_COMMENT;
        vorbisBlock.isLast = false;
        vorbisBlock.data = createVorbisCommentBlock(metadata, extraFields);
        vorbisBlock.length = vorbisBlock.data.size();
        
        // Insert after STREAMINFO (which should be first)
        if (!blocks.isEmpty()) {
            blocks.insert(1, vorbisBlock);
        } else {
            blocks.append(vorbisBlock);
        }
    }
    
    // Update or remove Picture block
    bool hasPicture = false;
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].blockType == BLOCK_TYPE_PICTURE) {
            if (!metadata.albumArt.isNull()) {
                // Replace with new image
                blocks[i].data = createPictureBlock(metadata.albumArt);
                blocks[i].length = blocks[i].data.size();
            } else {
                // Remove picture block
                blocks.removeAt(i);
                --i;
            }
            hasPicture = true;
            break;
        }
    }
    
    // Add Picture block if needed and doesn't exist
    if (!hasPicture && !metadata.albumArt.isNull()) {
        MetadataBlock pictureBlock;
        pictureBlock.blockType = BLOCK_TYPE_PICTURE;
        pictureBlock.isLast = false;
        pictureBlock.data = createPictureBlock(metadata.albumArt);
        pictureBlock.length = pictureBlock.data.size();
        blocks.append(pictureBlock);
    }
    
    // Mark last block
    if (!blocks.isEmpty()) {
        for (int i = 0; i < blocks.size(); ++i) {
            blocks[i].isLast = (i == blocks.size() - 1);
        }
    }
    
    // Write updated file
    return writeFlacFile(filePath, blocks, audioData);
}

bool MetadataEditor::updateField(const QString &filePath, const QString &fieldName, const QString &value)
{
    FlacMetadata metadata = readMetadata(filePath);
    
    QString upperField = fieldName.toUpper();
    if (upperField == "TITLE") {
        metadata.title = value;
    } else if (upperField == "ARTIST") {
        metadata.artist = value;
    } else if (upperField == "ALBUM") {
        metadata.album = value;
    } else if (upperField == "ALBUMARTIST" || upperField == "ALBUM ARTIST") {
        metadata.albumArtist = value;
    } else if (upperField == "DATE" || upperField == "YEAR") {
        metadata.year = value;
    } else if (upperField == "GENRE") {
        metadata.genre = value;
    } else if (upperField == "TRACKNUMBER" || upperField == "TRACK") {
        metadata.trackNumber = value;
    } else if (upperField == "COMMENT" || upperField == "DESCRIPTION") {
        metadata.comment = value;
    } else {
        m_lastError = "Unknown field: " + fieldName;
        return false;
    }
    
    return writeMetadata(filePath, metadata);
}

bool MetadataEditor::updateAlbumArt(const QString &filePath, const QImage &image)
{
    FlacMetadata metadata = readMetadata(filePath);
    metadata.albumArt = image;
    return writeMetadata(filePath, metadata);
}

bool MetadataEditor::removeAlbumArt(const QString &filePath)
{
    FlacMetadata metadata = readMetadata(filePath);
    metadata.albumArt = QImage(); // Null image
    return writeMetadata(filePath, metadata);
}


bool MetadataEditor::readFlacHeader(QFile &file)
{
    // FLAC files start with "fLaC" (0x664C6143)
    QByteArray header = file.read(4);
    if (header.size() != 4) {
        return false;
    }
    
    return (header[0] == 'f' && header[1] == 'L' && 
            header[2] == 'a' && header[3] == 'C');
}

QList<MetadataEditor::MetadataBlock> MetadataEditor::readMetadataBlocks(QFile &file)
{
    QList<MetadataBlock> blocks;
    
    bool isLastBlock = false;
    while (!isLastBlock && !file.atEnd()) {
        // Read block header (4 bytes)
        QByteArray header = file.read(4);
        if (header.size() != 4) {
            break;
        }
        
        MetadataBlock block;
        
        // First byte: bit 7 = last block flag, bits 6-0 = block type
        block.isLast = (static_cast<quint8>(header[0]) & 0x80) != 0;
        block.blockType = static_cast<quint8>(header[0]) & 0x7F;
        isLastBlock = block.isLast;
        
        // Next 3 bytes: block length (big-endian 24-bit)
        block.length = readBigEndian24(header, 1);
        
        // Read block data
        block.data = file.read(block.length);
        if (block.data.size() != static_cast<int>(block.length)) {
            qWarning() << "Failed to read complete metadata block";
            break;
        }
        
        blocks.append(block);
    }
    
    return blocks;
}

FlacMetadata MetadataEditor::parseStreamInfo(const QByteArray &data)
{
    FlacMetadata metadata;
    
    if (data.size() < 34) {
        return metadata; // Invalid STREAMINFO
    }
    
    // Bytes 10-13: Sample rate (20 bits), channels (3 bits), bits per sample (5 bits)
    quint32 word = readBigEndian32(data, 10);
    metadata.sampleRate = (word >> 12) & 0xFFFFF;
    metadata.channels = ((word >> 9) & 0x7) + 1;
    metadata.bitsPerSample = ((word >> 4) & 0x1F) + 1;
    
    // Bytes 13-17: Total samples (36 bits, lower 4 bits of byte 13, all of 14-17)
    quint64 totalSamples = (static_cast<quint64>(word & 0xF) << 32);
    totalSamples |= readBigEndian32(data, 14);
    metadata.totalSamples = totalSamples;
    
    return metadata;
}

QMap<QString, QString> MetadataEditor::parseVorbisComment(const QByteArray &data)
{
    QMap<QString, QString> comments;
    
    if (data.size() < 8) {
        return comments; // Too small
    }
    
    int offset = 0;
    
    // Vendor string length (little-endian 32-bit)
    quint32 vendorLength = static_cast<quint8>(data[offset]) |
                          (static_cast<quint8>(data[offset + 1]) << 8) |
                          (static_cast<quint8>(data[offset + 2]) << 16) |
                          (static_cast<quint8>(data[offset + 3]) << 24);
    offset += 4;
    
    // Skip vendor string
    offset += vendorLength;
    
    if (offset + 4 > data.size()) {
        return comments;
    }
    
    // Number of comments (little-endian 32-bit)
    quint32 commentCount = static_cast<quint8>(data[offset]) |
                          (static_cast<quint8>(data[offset + 1]) << 8) |
                          (static_cast<quint8>(data[offset + 2]) << 16) |
                          (static_cast<quint8>(data[offset + 3]) << 24);
    offset += 4;
    
    // Parse each comment
    for (quint32 i = 0; i < commentCount; ++i) {
        if (offset + 4 > data.size()) {
            break;
        }
        
        // Comment length (little-endian 32-bit)
        quint32 commentLength = static_cast<quint8>(data[offset]) |
                               (static_cast<quint8>(data[offset + 1]) << 8) |
                               (static_cast<quint8>(data[offset + 2]) << 16) |
                               (static_cast<quint8>(data[offset + 3]) << 24);
        offset += 4;
        
        if (offset + commentLength > static_cast<quint32>(data.size())) {
            break;
        }
        
        // Comment string (UTF-8)
        QString comment = QString::fromUtf8(data.mid(offset, commentLength));
        offset += commentLength;
        
        // Parse "KEY=VALUE" format
        int equalPos = comment.indexOf('=');
        if (equalPos > 0) {
            QString key = comment.left(equalPos);
            QString value = comment.mid(equalPos + 1);
            comments[key] = value;
        }
    }
    
    return comments;
}

QImage MetadataEditor::parsePictureBlock(const QByteArray &data)
{
    if (data.size() < 32) {
        return QImage(); // Too small
    }
    
    int offset = 0;
    
    // Picture type (4 bytes, big-endian) - skip
    offset += 4;
    
    // MIME type length (4 bytes, big-endian)
    quint32 mimeLength = readBigEndian32(data, offset);
    offset += 4;
    
    // Skip MIME type string
    offset += mimeLength;
    
    if (offset >= data.size()) {
        return QImage();
    }
    
    // Description length (4 bytes, big-endian)
    quint32 descLength = readBigEndian32(data, offset);
    offset += 4;
    
    // Skip description
    offset += descLength;
    
    if (offset + 20 > data.size()) {
        return QImage();
    }
    
    // Skip width, height, color depth, indexed colors (16 bytes)
    offset += 16;
    
    // Picture data length (4 bytes, big-endian)
    quint32 pictureLength = readBigEndian32(data, offset);
    offset += 4;
    
    if (offset + pictureLength > static_cast<quint32>(data.size())) {
        return QImage();
    }
    
    // Load image from data
    QByteArray imageData = data.mid(offset, pictureLength);
    QImage image;
    image.loadFromData(imageData);
    
    return image;
}


bool MetadataEditor::writeFlacFile(const QString &filePath, const QList<MetadataBlock> &blocks, const QByteArray &audioData)
{
    qDebug() << "[MetadataEditor] writeFlacFile: Writing" << blocks.size() << "blocks and" << audioData.size() << "bytes of audio";
    // Create temporary file
    QString tempPath = filePath + ".tmp";
    QFile tempFile(tempPath);
    
    if (!tempFile.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create temporary file: " + tempPath;
        qDebug() << "[MetadataEditor] ERROR: Cannot create temp file";
        return false;
    }
    
    // Write FLAC header
    tempFile.write("fLaC", 4);
    
    // Write metadata blocks
    for (const MetadataBlock &block : blocks) {
        qDebug() << "[MetadataEditor] Writing block - Type:" << block.blockType 
                 << "Length:" << block.length 
                 << "IsLast:" << block.isLast
                 << "Data size:" << block.data.size();
        
        // Block header (4 bytes)
        QByteArray header(4, 0);
        
        // First byte: last block flag and type
        header[0] = static_cast<char>(block.blockType | (block.isLast ? 0x80 : 0x00));
        
        // Next 3 bytes: length (big-endian 24-bit)
        header[1] = static_cast<char>((block.length >> 16) & 0xFF);
        header[2] = static_cast<char>((block.length >> 8) & 0xFF);
        header[3] = static_cast<char>(block.length & 0xFF);
        
        tempFile.write(header);
        tempFile.write(block.data);
    }
    
    // Write audio data
    tempFile.write(audioData);
    tempFile.close();
    
    // Replace original file with temporary file
    QFile::remove(filePath);
    if (!QFile::rename(tempPath, filePath)) {
        m_lastError = "Failed to replace original file";
        qDebug() << "[MetadataEditor] ERROR: Failed to replace original file";
        QFile::remove(tempPath);
        return false;
    }
    
    qDebug() << "[MetadataEditor] Successfully wrote FLAC file";
    m_lastError.clear();
    return true;
}

QByteArray MetadataEditor::createVorbisCommentBlock(const FlacMetadata &metadata, const QMap<QString, QString> &extraFields)
{
    QByteArray block;
    
    // Vendor string
    QString vendor = "Flac Player v2.0";
    QByteArray vendorUtf8 = vendor.toUtf8();
    quint32 vendorLength = vendorUtf8.size();
    
    // Write vendor length (little-endian)
    block.append(static_cast<char>(vendorLength & 0xFF));
    block.append(static_cast<char>((vendorLength >> 8) & 0xFF));
    block.append(static_cast<char>((vendorLength >> 16) & 0xFF));
    block.append(static_cast<char>((vendorLength >> 24) & 0xFF));
    
    // Write vendor string
    block.append(vendorUtf8);
    
    // Build comment list
    QList<QPair<QString, QString>> comments;
    
    if (!metadata.title.isEmpty()) comments.append({"TITLE", metadata.title});
    if (!metadata.artist.isEmpty()) comments.append({"ARTIST", metadata.artist});
    if (!metadata.album.isEmpty()) comments.append({"ALBUM", metadata.album});
    if (!metadata.albumArtist.isEmpty()) comments.append({"ALBUMARTIST", metadata.albumArtist});
    if (!metadata.year.isEmpty()) comments.append({"DATE", metadata.year});
    if (!metadata.genre.isEmpty()) comments.append({"GENRE", metadata.genre});
    if (!metadata.trackNumber.isEmpty()) comments.append({"TRACKNUMBER", metadata.trackNumber});
    if (!metadata.comment.isEmpty()) comments.append({"COMMENT", metadata.comment});
    
    // Add extra fields
    for (auto it = extraFields.constBegin(); it != extraFields.constEnd(); ++it) {
        comments.append({it.key(), it.value()});
    }
    
    // Write comment count (little-endian)
    quint32 commentCount = comments.size();
    block.append(static_cast<char>(commentCount & 0xFF));
    block.append(static_cast<char>((commentCount >> 8) & 0xFF));
    block.append(static_cast<char>((commentCount >> 16) & 0xFF));
    block.append(static_cast<char>((commentCount >> 24) & 0xFF));
    
    // Write each comment
    for (const auto &comment : comments) {
        QString commentStr = comment.first + "=" + comment.second;
        QByteArray commentUtf8 = commentStr.toUtf8();
        quint32 commentLength = commentUtf8.size();
        
        // Write comment length (little-endian)
        block.append(static_cast<char>(commentLength & 0xFF));
        block.append(static_cast<char>((commentLength >> 8) & 0xFF));
        block.append(static_cast<char>((commentLength >> 16) & 0xFF));
        block.append(static_cast<char>((commentLength >> 24) & 0xFF));
        
        // Write comment string
        block.append(commentUtf8);
    }
    
    return block;
}

QByteArray MetadataEditor::createPictureBlock(const QImage &image)
{
    QByteArray block;
    
    // Convert image to PNG
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    
    // Picture type (3 = front cover, big-endian 32-bit)
    writeBigEndian32(block, 3);
    
    // MIME type
    QByteArray mimeType = "image/png";
    writeBigEndian32(block, mimeType.size());
    block.append(mimeType);
    
    // Description (empty)
    writeBigEndian32(block, 0);
    
    // Width (big-endian 32-bit)
    writeBigEndian32(block, image.width());
    
    // Height (big-endian 32-bit)
    writeBigEndian32(block, image.height());
    
    // Color depth (32-bit RGBA)
    writeBigEndian32(block, 32);
    
    // Number of indexed colors (0 for non-indexed)
    writeBigEndian32(block, 0);
    
    // Picture data length
    writeBigEndian32(block, imageData.size());
    
    // Picture data
    block.append(imageData);
    
    return block;
}


quint32 MetadataEditor::readBigEndian24(const QByteArray &data, int offset)
{
    return (static_cast<quint8>(data[offset + 1]) << 16) |
           (static_cast<quint8>(data[offset + 2]) << 8) |
           static_cast<quint8>(data[offset + 3]);
}

quint32 MetadataEditor::readBigEndian32(const QByteArray &data, int offset)
{
    return (static_cast<quint8>(data[offset]) << 24) |
           (static_cast<quint8>(data[offset + 1]) << 16) |
           (static_cast<quint8>(data[offset + 2]) << 8) |
           static_cast<quint8>(data[offset + 3]);
}

quint64 MetadataEditor::readBigEndian64(const QByteArray &data, int offset)
{
    return (static_cast<quint64>(static_cast<quint8>(data[offset])) << 56) |
           (static_cast<quint64>(static_cast<quint8>(data[offset + 1])) << 48) |
           (static_cast<quint64>(static_cast<quint8>(data[offset + 2])) << 40) |
           (static_cast<quint64>(static_cast<quint8>(data[offset + 3])) << 32) |
           (static_cast<quint64>(static_cast<quint8>(data[offset + 4])) << 24) |
           (static_cast<quint64>(static_cast<quint8>(data[offset + 5])) << 16) |
           (static_cast<quint64>(static_cast<quint8>(data[offset + 6])) << 8) |
           static_cast<quint64>(static_cast<quint8>(data[offset + 7]));
}

void MetadataEditor::writeBigEndian24(QByteArray &data, quint32 value)
{
    data.append(static_cast<char>((value >> 16) & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
}

void MetadataEditor::writeBigEndian32(QByteArray &data, quint32 value)
{
    data.append(static_cast<char>((value >> 24) & 0xFF));
    data.append(static_cast<char>((value >> 16) & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
}


MetadataEditorDialog::MetadataEditorDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(filePath)
{
    // qDebug() << "[MetadataEditorDialog] Constructor called for:" << filePath;
    // qDebug() << "[MetadataEditorDialog] Setting up UI...";
    setupUI();
    // qDebug() << "[MetadataEditorDialog] UI setup complete, loading metadata...";
    loadMetadata();
    // qDebug() << "[MetadataEditorDialog] Metadata loaded, dialog ready";
}

MetadataEditorDialog::~MetadataEditorDialog()
{
}

void MetadataEditorDialog::setupUI()
{
    setWindowTitle("Edit Metadata");
    resize(600, 700);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // File info label
    QFileInfo fileInfo(m_filePath);
    m_fileInfoLabel = new QLabel(QString("<b>File:</b> %1").arg(fileInfo.fileName()));
    mainLayout->addWidget(m_fileInfoLabel);
    
    // Create form for text fields
    QGroupBox *textGroup = new QGroupBox("Track Information");
    QFormLayout *formLayout = new QFormLayout();
    
    m_titleEdit = new QLineEdit();
    formLayout->addRow("Title:", m_titleEdit);
    
    m_artistEdit = new QLineEdit();
    formLayout->addRow("Artist:", m_artistEdit);
    
    m_albumEdit = new QLineEdit();
    formLayout->addRow("Album:", m_albumEdit);
    
    m_albumArtistEdit = new QLineEdit();
    formLayout->addRow("Album Artist:", m_albumArtistEdit);
    
    m_yearEdit = new QLineEdit();
    m_yearEdit->setMaxLength(4);
    formLayout->addRow("Year:", m_yearEdit);
    
    m_genreEdit = new QLineEdit();
    formLayout->addRow("Genre:", m_genreEdit);
    
    m_trackNumberEdit = new QLineEdit();
    m_trackNumberEdit->setMaxLength(3);
    formLayout->addRow("Track Number:", m_trackNumberEdit);
    
    m_commentEdit = new QTextEdit();
    m_commentEdit->setMaximumHeight(60);
    formLayout->addRow("Comment:", m_commentEdit);
    
    textGroup->setLayout(formLayout);
    mainLayout->addWidget(textGroup);
    
    // Album art section
    QGroupBox *artGroup = new QGroupBox("Album Art");
    QVBoxLayout *artLayout = new QVBoxLayout();
    
    m_albumArtLabel = new QLabel("No album art");
    m_albumArtLabel->setAlignment(Qt::AlignCenter);
    m_albumArtLabel->setMinimumSize(200, 200);
    m_albumArtLabel->setMaximumSize(200, 200);
    m_albumArtLabel->setStyleSheet("QLabel { border: 1px solid #666; background-color: #222; }");
    m_albumArtLabel->setScaledContents(false);
    artLayout->addWidget(m_albumArtLabel, 0, Qt::AlignCenter);
    
    QHBoxLayout *artButtonLayout = new QHBoxLayout();
    m_loadArtButton = new QPushButton("Load Image");
    m_removeArtButton = new QPushButton("Remove");
    artButtonLayout->addWidget(m_loadArtButton);
    artButtonLayout->addWidget(m_removeArtButton);
    artLayout->addLayout(artButtonLayout);
    
    artGroup->setLayout(artLayout);
    mainLayout->addWidget(artGroup);
    
    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_saveButton = new QPushButton("Save Changes");
    m_saveButton->setDefault(true);
    m_cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_saveButton, &QPushButton::clicked, this, &MetadataEditorDialog::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &MetadataEditorDialog::onCancelClicked);
    connect(m_loadArtButton, &QPushButton::clicked, this, &MetadataEditorDialog::onLoadAlbumArtClicked);
    connect(m_removeArtButton, &QPushButton::clicked, this, &MetadataEditorDialog::onRemoveAlbumArtClicked);
}

void MetadataEditorDialog::loadMetadata()
{
    // qDebug() << "[MetadataEditorDialog] loadMetadata called";
    m_metadata = m_editor.readMetadata(m_filePath);
    
    if (!m_editor.lastError().isEmpty()) {
        // qDebug() << "[MetadataEditorDialog] ERROR:" << m_editor.lastError();
        QMessageBox::warning(this, "Error", 
            "Failed to read metadata: " + m_editor.lastError());
        return;
    }
    // qDebug() << "[MetadataEditorDialog] Metadata read successfully";
    
    // Populate fields
    m_titleEdit->setText(m_metadata.title);
    m_artistEdit->setText(m_metadata.artist);
    m_albumEdit->setText(m_metadata.album);
    m_albumArtistEdit->setText(m_metadata.albumArtist);
    m_yearEdit->setText(m_metadata.year);
    m_genreEdit->setText(m_metadata.genre);
    m_trackNumberEdit->setText(m_metadata.trackNumber);
    m_commentEdit->setPlainText(m_metadata.comment);
    
    // Update file info with technical details
    QString info = QString("<b>File:</b> %1<br>").arg(QFileInfo(m_filePath).fileName());
    if (m_metadata.sampleRate > 0) {
        info += QString("<b>Format:</b> %1 Hz, %2 bit, %3 channels")
            .arg(m_metadata.sampleRate)
            .arg(m_metadata.bitsPerSample)
            .arg(m_metadata.channels);
    }
    m_fileInfoLabel->setText(info);
    
    updateAlbumArtDisplay();
}

void MetadataEditorDialog::updateAlbumArtDisplay()
{
    if (!m_metadata.albumArt.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(m_metadata.albumArt);
        QPixmap scaled = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_albumArtLabel->setPixmap(scaled);
        m_removeArtButton->setEnabled(true);
    } else {
        m_albumArtLabel->clear();
        m_albumArtLabel->setText("No album art");
        m_removeArtButton->setEnabled(false);
    }
}

void MetadataEditorDialog::onSaveClicked()
{
    // Update metadata structure
    m_metadata.title = m_titleEdit->text();
    m_metadata.artist = m_artistEdit->text();
    m_metadata.album = m_albumEdit->text();
    m_metadata.albumArtist = m_albumArtistEdit->text();
    m_metadata.year = m_yearEdit->text();
    m_metadata.genre = m_genreEdit->text();
    m_metadata.trackNumber = m_trackNumberEdit->text();
    m_metadata.comment = m_commentEdit->toPlainText();
    
    // Write to file
    if (m_editor.writeMetadata(m_filePath, m_metadata)) {
        QMessageBox::information(this, "Success", "Metadata saved successfully!");
        accept();
    } else {
        QMessageBox::critical(this, "Error", 
            "Failed to save metadata: " + m_editor.lastError());
    }
}

void MetadataEditorDialog::onCancelClicked()
{
    reject();
}

void MetadataEditorDialog::onLoadAlbumArtClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Album Art",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp);;All Files (*)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QImage image(fileName);
    if (image.isNull()) {
        QMessageBox::warning(this, "Error", "Failed to load image file.");
        return;
    }
    
    // Resize if too large (keep under 1000x1000)
    if (image.width() > 1000 || image.height() > 1000) {
        image = image.scaled(1000, 1000, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    m_metadata.albumArt = image;
    updateAlbumArtDisplay();
}

void MetadataEditorDialog::onRemoveAlbumArtClicked()
{
    m_metadata.albumArt = QImage();
    updateAlbumArtDisplay();
}
