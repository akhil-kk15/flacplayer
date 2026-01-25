#include "audiomanager.h"
#include "ui_metadataeditor.h"
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QDebug>
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
    qDebug() << "[MetadataEditor] Metadata to write:";
    qDebug() << "  Title:" << metadata.title;
    qDebug() << "  Artist:" << metadata.artist;
    qDebug() << "  Album:" << metadata.album;
    qDebug() << "  AlbumArtist:" << metadata.albumArtist;
    qDebug() << "  Year:" << metadata.year;
    qDebug() << "  Genre:" << metadata.genre;
    qDebug() << "  TrackNumber:" << metadata.trackNumber;
    qDebug() << "  Comment:" << metadata.comment;
    qDebug() << "  AlbumArt isNull:" << metadata.albumArt.isNull();
    if (!metadata.albumArt.isNull()) {
        qDebug() << "  AlbumArt size:" << metadata.albumArt.width() << "x" << metadata.albumArt.height();
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file for reading: " + filePath;
        qDebug() << "[MetadataEditor] ERROR: Cannot open file for writing";
        return false;
    }
    qDebug() << "[MetadataEditor] File opened successfully for reading, size:" << file.size();
    
    // Verify FLAC header
    if (!readFlacHeader(file)) {
        m_lastError = "Invalid FLAC file format";
        qDebug() << "[MetadataEditor] ERROR: Invalid FLAC header";
        file.close();
        return false;
    }
    qDebug() << "[MetadataEditor] FLAC header verified successfully";
    
    // Read existing metadata and audio data
    qDebug() << "[MetadataEditor] Reading existing metadata blocks...";
    QList<MetadataBlock> blocks = readMetadataBlocks(file);
    qDebug() << "[MetadataEditor] Read" << blocks.size() << "metadata blocks";
    
    qint64 audioDataStartPos = file.pos();
    QByteArray audioData = file.readAll();
    qDebug() << "[MetadataEditor] Read" << audioData.size() << "bytes of audio data starting at position" << audioDataStartPos;
    file.close();
    
    // Update or create Vorbis Comment block
    bool hasVorbisComment = false;
    QMap<QString, QString> extraFields;
    
    qDebug() << "[MetadataEditor] Processing Vorbis Comment block...";
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].blockType == BLOCK_TYPE_VORBIS_COMMENT) {
            qDebug() << "[MetadataEditor] Found existing Vorbis Comment block at index" << i;
            qDebug() << "[MetadataEditor] Existing block size:" << blocks[i].data.size();
            // Preserve extra fields not in our structure
            QMap<QString, QString> existing = parseVorbisComment(blocks[i].data);
            qDebug() << "[MetadataEditor] Parsed" << existing.size() << "existing comments";
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
            qDebug() << "[MetadataEditor] Creating new Vorbis Comment block with" << extraFields.size() << "extra fields";
            blocks[i].data = createVorbisCommentBlock(metadata, extraFields);
            blocks[i].length = blocks[i].data.size();
            qDebug() << "[MetadataEditor] New Vorbis Comment block size:" << blocks[i].length;
            hasVorbisComment = true;
            break;
        }
    }
    
    // Add Vorbis Comment if it doesn't exist
    if (!hasVorbisComment) {
        qDebug() << "[MetadataEditor] No existing Vorbis Comment block, creating new one";
        MetadataBlock vorbisBlock;
        vorbisBlock.blockType = BLOCK_TYPE_VORBIS_COMMENT;
        vorbisBlock.isLast = false;
        vorbisBlock.data = createVorbisCommentBlock(metadata, extraFields);
        vorbisBlock.length = vorbisBlock.data.size();
        qDebug() << "[MetadataEditor] New Vorbis Comment block created, size:" << vorbisBlock.length;
        
        // Insert after STREAMINFO (which should be first)
        if (!blocks.isEmpty()) {
            qDebug() << "[MetadataEditor] Inserting Vorbis Comment block at position 1";
            blocks.insert(1, vorbisBlock);
        } else {
            qDebug() << "[MetadataEditor] WARNING: No existing blocks, appending Vorbis Comment";
            blocks.append(vorbisBlock);
        }
    }
    
    // Update or remove Picture block
    qDebug() << "[MetadataEditor] Processing Picture block...";
    bool hasPicture = false;
    for (int i = 0; i < blocks.size(); ++i) {
        if (blocks[i].blockType == BLOCK_TYPE_PICTURE) {
            qDebug() << "[MetadataEditor] Found existing Picture block at index" << i;
            qDebug() << "[MetadataEditor] Existing picture block size:" << blocks[i].data.size();
            if (!metadata.albumArt.isNull()) {
                // Replace with new image
                qDebug() << "[MetadataEditor] Replacing with new album art";
                blocks[i].data = createPictureBlock(metadata.albumArt);
                blocks[i].length = blocks[i].data.size();
                qDebug() << "[MetadataEditor] New picture block size:" << blocks[i].length;
            } else {
                // Remove picture block
                qDebug() << "[MetadataEditor] Removing picture block (no album art)";
                blocks.removeAt(i);
                --i;
            }
            hasPicture = true;
            break;
        }
    }
    
    // Add Picture block if needed and doesn't exist
    if (!hasPicture && !metadata.albumArt.isNull()) {
        qDebug() << "[MetadataEditor] No existing Picture block, creating new one";
        MetadataBlock pictureBlock;
        pictureBlock.blockType = BLOCK_TYPE_PICTURE;
        pictureBlock.isLast = false;
        pictureBlock.data = createPictureBlock(metadata.albumArt);
        pictureBlock.length = pictureBlock.data.size();
        qDebug() << "[MetadataEditor] New picture block size:" << pictureBlock.length;
        blocks.append(pictureBlock);
    }
    
    // Mark last block
    qDebug() << "[MetadataEditor] Marking last block flag...";
    if (!blocks.isEmpty()) {
        for (int i = 0; i < blocks.size(); ++i) {
            blocks[i].isLast = (i == blocks.size() - 1);
            qDebug() << "[MetadataEditor] Block" << i << "- Type:" << blocks[i].blockType 
                     << "Length:" << blocks[i].length << "IsLast:" << blocks[i].isLast;
        }
    }
    
    // Write updated file
    qDebug() << "[MetadataEditor] Calling writeFlacFile with" << blocks.size() << "blocks and" 
             << audioData.size() << "bytes of audio";
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
    qDebug() << "[MetadataEditor] Original file path:" << filePath;
    
    // Create temporary file
    QString tempPath = filePath + ".tmp";
    qDebug() << "[MetadataEditor] Creating temporary file:" << tempPath;
    QFile tempFile(tempPath);
    
    if (!tempFile.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create temporary file: " + tempPath;
        qDebug() << "[MetadataEditor] ERROR: Cannot create temp file";
        return false;
    }
    qDebug() << "[MetadataEditor] Temp file opened successfully";
    
    // Write FLAC header
    qDebug() << "[MetadataEditor] Writing FLAC header (fLaC)";
    qint64 headerWritten = tempFile.write("fLaC", 4);
    qDebug() << "[MetadataEditor] Header bytes written:" << headerWritten;
    
    // Write metadata blocks
    qDebug() << "[MetadataEditor] Writing" << blocks.size() << "metadata blocks...";
    qint64 totalMetadataWritten = 4; // header
    for (int idx = 0; idx < blocks.size(); ++idx) {
        const MetadataBlock &block = blocks[idx];
        qDebug() << "[MetadataEditor] Writing block" << idx << "- Type:" << block.blockType 
                 << "Length:" << block.length 
                 << "IsLast:" << block.isLast
                 << "Data size:" << block.data.size();
        
        // Validate block data
        if (block.data.size() != static_cast<int>(block.length)) {
            qDebug() << "[MetadataEditor] WARNING: Block data size mismatch! Data size:" 
                     << block.data.size() << "but length field says:" << block.length;
        }
        
        // Block header (4 bytes)
        QByteArray header(4, 0);
        
        // First byte: last block flag and type
        header[0] = static_cast<char>(block.blockType | (block.isLast ? 0x80 : 0x00));
        
        // Next 3 bytes: length (big-endian 24-bit)
        header[1] = static_cast<char>((block.length >> 16) & 0xFF);
        header[2] = static_cast<char>((block.length >> 8) & 0xFF);
        header[3] = static_cast<char>(block.length & 0xFF);
        
        qDebug() << "[MetadataEditor] Block header bytes:" 
                 << "[0]=" << QString::number(static_cast<quint8>(header[0]), 16)
                 << "[1]=" << QString::number(static_cast<quint8>(header[1]), 16)
                 << "[2]=" << QString::number(static_cast<quint8>(header[2]), 16)
                 << "[3]=" << QString::number(static_cast<quint8>(header[3]), 16);
        
        qint64 headerBytesWritten = tempFile.write(header);
        qint64 dataBytesWritten = tempFile.write(block.data);
        qDebug() << "[MetadataEditor] Block" << idx << "written - Header:" << headerBytesWritten 
                 << "bytes, Data:" << dataBytesWritten << "bytes";
        totalMetadataWritten += headerBytesWritten + dataBytesWritten;
    }
    qDebug() << "[MetadataEditor] Total metadata written:" << totalMetadataWritten << "bytes";
    
    // Write audio data
    qDebug() << "[MetadataEditor] Writing audio data (" << audioData.size() << "bytes)...";
    qint64 audioBytesWritten = tempFile.write(audioData);
    qDebug() << "[MetadataEditor] Audio bytes written:" << audioBytesWritten;
    
    qint64 totalFileSize = tempFile.size();
    tempFile.close();
    qDebug() << "[MetadataEditor] Temp file closed, total size:" << totalFileSize;
    
    // Verify temp file integrity before replacing original
    qDebug() << "[MetadataEditor] Validating temp file integrity...";
    QFile verifyFile(tempPath);
    if (!verifyFile.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open temp file for verification";
        qDebug() << "[MetadataEditor] ERROR: Cannot verify temp file";
        QFile::remove(tempPath);
        return false;
    }
    
    // Verify file size is reasonable
    qint64 verifySize = verifyFile.size();
    qDebug() << "[MetadataEditor] Temp file verification - Size:" << verifySize;
    if (verifySize < 42) { // Minimum FLAC file size (header + STREAMINFO)
        m_lastError = "Temp file too small to be valid FLAC";
        qDebug() << "[MetadataEditor] ERROR: Temp file too small";
        verifyFile.close();
        QFile::remove(tempPath);
        return false;
    }
    
    // Verify FLAC header
    if (!readFlacHeader(verifyFile)) {
        m_lastError = "Temp file has invalid FLAC header";
        qDebug() << "[MetadataEditor] ERROR: Temp file failed FLAC header validation";
        verifyFile.close();
        QFile::remove(tempPath);
        return false;
    }
    verifyFile.close();
    qDebug() << "[MetadataEditor] Temp file validation successful";
    
    // Replace original file with temporary file (atomic on most systems)
    qDebug() << "[MetadataEditor] Replacing original file with validated temp file...";
    if (!QFile::remove(filePath)) {
        qDebug() << "[MetadataEditor] WARNING: Could not remove original file (may not exist)";
    }
    
    if (!QFile::rename(tempPath, filePath)) {
        m_lastError = "Failed to replace original file - original may be lost!";
        qDebug() << "[MetadataEditor] ERROR: Failed to rename temp file to original";
        // Temp file still exists, user can manually recover
        return false;
    }
    
    qDebug() << "[MetadataEditor] Successfully wrote FLAC file";
    qDebug() << "[MetadataEditor] Final file path:" << filePath;
    
    // Verify final file
    QFile finalFile(filePath);
    if (finalFile.open(QIODevice::ReadOnly)) {
        qint64 finalSize = finalFile.size();
        qDebug() << "[MetadataEditor] Final file size:" << finalSize;
        finalFile.close();
    }
    
    m_lastError.clear();
    return true;
}

QByteArray MetadataEditor::createVorbisCommentBlock(const FlacMetadata &metadata, const QMap<QString, QString> &extraFields)
{
    qDebug() << "[MetadataEditor] createVorbisCommentBlock called";
    QByteArray block;
    
    // Vendor string
    QString vendor = "Flac Player v2.0";
    QByteArray vendorUtf8 = vendor.toUtf8();
    quint32 vendorLength = vendorUtf8.size();
    qDebug() << "[MetadataEditor] Vendor string:" << vendor << "length:" << vendorLength;
    
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
    qDebug() << "[MetadataEditor] Total comments to write:" << commentCount;
    block.append(static_cast<char>(commentCount & 0xFF));
    block.append(static_cast<char>((commentCount >> 8) & 0xFF));
    block.append(static_cast<char>((commentCount >> 16) & 0xFF));
    block.append(static_cast<char>((commentCount >> 24) & 0xFF));
    
    // Write each comment
    for (const auto &comment : comments) {
        qDebug() << "[MetadataEditor] Writing comment:" << comment.first << "=" << comment.second;
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
    
    qDebug() << "[MetadataEditor] Vorbis Comment block created, total size:" << block.size();
    return block;
}

QByteArray MetadataEditor::createPictureBlock(const QImage &image)
{
    qDebug() << "[MetadataEditor] createPictureBlock called";
    qDebug() << "[MetadataEditor] Image size:" << image.width() << "x" << image.height();
    QByteArray block;
    
    // Convert image to PNG
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    bool saveSuccess = image.save(&buffer, "PNG");
    buffer.close();
    qDebug() << "[MetadataEditor] Image converted to PNG, success:" << saveSuccess 
             << "size:" << imageData.size() << "bytes";
    
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
    
    qDebug() << "[MetadataEditor] Picture block created, total size:" << block.size();
    return block;
}


quint32 MetadataEditor::readBigEndian24(const QByteArray &data, int offset)
{
    return (static_cast<quint8>(data[offset]) << 16) |
           (static_cast<quint8>(data[offset + 1]) << 8) |
           static_cast<quint8>(data[offset + 2]);
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
    , ui(new Ui::MetadataEditorDialog)
    , m_filePath(filePath)
{
    qDebug() << "[MetadataEditorDialog] Constructor called for:" << filePath;
    ui->setupUi(this);
    qDebug() << "[MetadataEditorDialog] UI setup complete, loading metadata...";
    loadMetadata();
    qDebug() << "[MetadataEditorDialog] Metadata loaded, dialog ready";
}

MetadataEditorDialog::~MetadataEditorDialog()
{
    delete ui;
}

void MetadataEditorDialog::loadMetadata()
{
    qDebug() << "[MetadataEditorDialog] loadMetadata called for:" << m_filePath;
    
    // Validate file exists
    if (!QFile::exists(m_filePath)) {
        QMessageBox::critical(this, "Error", "File does not exist: " + m_filePath);
        return;
    }
    
    m_metadata = m_editor.readMetadata(m_filePath);
    
    if (!m_editor.lastError().isEmpty()) {
        qDebug() << "[MetadataEditorDialog] ERROR:" << m_editor.lastError();
        QMessageBox::warning(this, "Error", 
            "Failed to read metadata: " + m_editor.lastError());
        return;
    }
    qDebug() << "[MetadataEditorDialog] Metadata read successfully";
    
    // Populate fields
    ui->titleEdit->setText(m_metadata.title);
    ui->artistEdit->setText(m_metadata.artist);
    ui->albumEdit->setText(m_metadata.album);
    ui->albumArtistEdit->setText(m_metadata.albumArtist);
    ui->yearEdit->setText(m_metadata.year);
    ui->genreEdit->setText(m_metadata.genre);
    ui->trackNumberEdit->setText(m_metadata.trackNumber);
    ui->commentEdit->setPlainText(m_metadata.comment);
    
    // Update file info with technical details
    QString info = QString("<b>File:</b> %1<br>").arg(QFileInfo(m_filePath).fileName());
    if (m_metadata.sampleRate > 0) {
        info += QString("<b>Format:</b> %1 Hz, %2 bit, %3 channels")
            .arg(m_metadata.sampleRate)
            .arg(m_metadata.bitsPerSample)
            .arg(m_metadata.channels);
    }
    ui->fileInfoLabel->setText(info);
    
    updateAlbumArtDisplay();
}

void MetadataEditorDialog::updateAlbumArtDisplay()
{
    if (!m_metadata.albumArt.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(m_metadata.albumArt);
        QPixmap scaled = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->albumArtLabel->setPixmap(scaled);
        ui->removeArtButton->setEnabled(true);
    } else {
        ui->albumArtLabel->clear();
        ui->albumArtLabel->setText("No album art");
        ui->removeArtButton->setEnabled(false);
    }
}

void MetadataEditorDialog::onSaveClicked()
{
    // Update metadata structure
    m_metadata.title = ui->titleEdit->text();
    m_metadata.artist = ui->artistEdit->text();
    m_metadata.album = ui->albumEdit->text();
    m_metadata.albumArtist = ui->albumArtistEdit->text();
    m_metadata.year = ui->yearEdit->text();
    m_metadata.genre = ui->genreEdit->text();
    m_metadata.trackNumber = ui->trackNumberEdit->text();
    m_metadata.comment = ui->commentEdit->toPlainText();
    
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
