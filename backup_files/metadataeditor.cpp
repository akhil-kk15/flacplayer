#include "metadataeditor.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QGroupBox>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

MetadataEditor::MetadataEditor(const QString &filePath, QWidget *parent)
    : QDialog(parent)
    , m_filePath(filePath)
{
    setupUI();
    loadCurrentMetadata();
}

MetadataEditor::~MetadataEditor()
{
}

void MetadataEditor::setupUI()
{
    setWindowTitle("Edit Metadata");
    setMinimumSize(500, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // File info
    QFileInfo fileInfo(m_filePath);
    QLabel *fileLabel = new QLabel("<b>File:</b> " + fileInfo.fileName(), this);
    mainLayout->addWidget(fileLabel);
    
    // Metadata form
    QGroupBox *metadataGroup = new QGroupBox("Metadata Tags", this);
    QFormLayout *metaForm = new QFormLayout(metadataGroup);
    
    m_titleEdit = new QLineEdit(this);
    m_artistEdit = new QLineEdit(this);
    m_albumEdit = new QLineEdit(this);
    m_yearEdit = new QLineEdit(this);
    m_genreEdit = new QLineEdit(this);
    m_commentEdit = new QTextEdit(this);
    m_commentEdit->setMaximumHeight(100);
    
    metaForm->addRow("Title:", m_titleEdit);
    metaForm->addRow("Artist:", m_artistEdit);
    metaForm->addRow("Album:", m_albumEdit);
    metaForm->addRow("Year:", m_yearEdit);
    metaForm->addRow("Genre:", m_genreEdit);
    metaForm->addRow("Comment:", m_commentEdit);
    
    mainLayout->addWidget(metadataGroup);
    
    // Status label
    m_statusLabel = new QLabel(this);
    mainLayout->addWidget(m_statusLabel);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_saveButton = new QPushButton("Save", this);
    m_cancelButton = new QPushButton("Cancel", this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connections
    connect(m_saveButton, &QPushButton::clicked, this, &MetadataEditor::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &MetadataEditor::onCancel);
}

void MetadataEditor::loadCurrentMetadata()
{
    AVFormatContext *formatContext = nullptr;
    
    if (avformat_open_input(&formatContext, m_filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        m_statusLabel->setText("Failed to open file");
        return;
    }
    
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        avformat_close_input(&formatContext);
        m_statusLabel->setText("Failed to read stream info");
        return;
    }
    
    // Read existing metadata
    AVDictionaryEntry *tag = nullptr;
    
    tag = av_dict_get(formatContext->metadata, "title", nullptr, 0);
    if (tag) {
        m_metadata.title = QString::fromUtf8(tag->value);
        m_titleEdit->setText(m_metadata.title);
    }
    
    tag = av_dict_get(formatContext->metadata, "artist", nullptr, 0);
    if (tag) {
        m_metadata.artist = QString::fromUtf8(tag->value);
        m_artistEdit->setText(m_metadata.artist);
    }
    
    tag = av_dict_get(formatContext->metadata, "album", nullptr, 0);
    if (tag) {
        m_metadata.album = QString::fromUtf8(tag->value);
        m_albumEdit->setText(m_metadata.album);
    }
    
    tag = av_dict_get(formatContext->metadata, "date", nullptr, 0);
    if (tag) {
        m_metadata.year = QString::fromUtf8(tag->value);
        m_yearEdit->setText(m_metadata.year);
    }
    
    tag = av_dict_get(formatContext->metadata, "genre", nullptr, 0);
    if (tag) {
        m_metadata.genre = QString::fromUtf8(tag->value);
        m_genreEdit->setText(m_metadata.genre);
    }
    
    tag = av_dict_get(formatContext->metadata, "comment", nullptr, 0);
    if (tag) {
        m_metadata.comment = QString::fromUtf8(tag->value);
        m_commentEdit->setPlainText(m_metadata.comment);
    }
    
    avformat_close_input(&formatContext);
    m_statusLabel->setText("Metadata loaded");
}

bool MetadataEditor::saveMetadata()
{
    AVFormatContext *inputCtx = nullptr;
    AVFormatContext *outputCtx = nullptr;
    
    if (avformat_open_input(&inputCtx, m_filePath.toUtf8().constData(), nullptr, nullptr) != 0) {
        QMessageBox::critical(this, "Error", "Failed to open input file");
        return false;
    }
    
    if (avformat_find_stream_info(inputCtx, nullptr) < 0) {
        avformat_close_input(&inputCtx);
        QMessageBox::critical(this, "Error", "Failed to read stream info");
        return false;
    }
    
    QString tempPath = m_filePath + ".tmp";
    
    if (avformat_alloc_output_context2(&outputCtx, nullptr, nullptr, tempPath.toUtf8().constData()) < 0) {
        avformat_close_input(&inputCtx);
        QMessageBox::critical(this, "Error", "Failed to create output context");
        return false;
    }
    
    for (unsigned int i = 0; i < inputCtx->nb_streams; i++) {
        AVStream *inStream = inputCtx->streams[i];
        AVStream *outStream = avformat_new_stream(outputCtx, nullptr);
        
        if (!outStream) {
            avformat_close_input(&inputCtx);
            avformat_free_context(outputCtx);
            QMessageBox::critical(this, "Error", "Failed to create output stream");
            return false;
        }
        
        if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0) {
            avformat_close_input(&inputCtx);
            avformat_free_context(outputCtx);
            QMessageBox::critical(this, "Error", "Failed to copy codec parameters");
            return false;
        }
        outStream->codecpar->codec_tag = 0;
    }
    
    if (!m_metadata.title.isEmpty())
        av_dict_set(&outputCtx->metadata, "title", m_metadata.title.toUtf8().constData(), 0);
    if (!m_metadata.artist.isEmpty())
        av_dict_set(&outputCtx->metadata, "artist", m_metadata.artist.toUtf8().constData(), 0);
    if (!m_metadata.album.isEmpty())
        av_dict_set(&outputCtx->metadata, "album", m_metadata.album.toUtf8().constData(), 0);
    if (!m_metadata.year.isEmpty())
        av_dict_set(&outputCtx->metadata, "date", m_metadata.year.toUtf8().constData(), 0);
    if (!m_metadata.genre.isEmpty())
        av_dict_set(&outputCtx->metadata, "genre", m_metadata.genre.toUtf8().constData(), 0);
    if (!m_metadata.comment.isEmpty())
        av_dict_set(&outputCtx->metadata, "comment", m_metadata.comment.toUtf8().constData(), 0);
    
    if (!(outputCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputCtx->pb, tempPath.toUtf8().constData(), AVIO_FLAG_WRITE) < 0) {
            avformat_close_input(&inputCtx);
            avformat_free_context(outputCtx);
            QMessageBox::critical(this, "Error", "Failed to open output file");
            return false;
        }
    }
    
    if (avformat_write_header(outputCtx, nullptr) < 0) {
        if (!(outputCtx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&outputCtx->pb);
        avformat_close_input(&inputCtx);
        avformat_free_context(outputCtx);
        QMessageBox::critical(this, "Error", "Failed to write header");
        return false;
    }
    
    AVPacket *packet = av_packet_alloc();
    while (av_read_frame(inputCtx, packet) >= 0) {
        AVStream *inStream = inputCtx->streams[packet->stream_index];
        AVStream *outStream = outputCtx->streams[packet->stream_index];
        
        packet->pts = av_rescale_q_rnd(packet->pts, inStream->time_base, outStream->time_base, 
                                       static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet->dts = av_rescale_q_rnd(packet->dts, inStream->time_base, outStream->time_base, 
                                       static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet->duration = av_rescale_q(packet->duration, inStream->time_base, outStream->time_base);
        packet->pos = -1;
        
        if (av_interleaved_write_frame(outputCtx, packet) < 0) {
            av_packet_free(&packet);
            if (!(outputCtx->oformat->flags & AVFMT_NOFILE))
                avio_closep(&outputCtx->pb);
            avformat_close_input(&inputCtx);
            avformat_free_context(outputCtx);
            QMessageBox::critical(this, "Error", "Failed to write packet");
            return false;
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    
    av_write_trailer(outputCtx);
    
    if (!(outputCtx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&outputCtx->pb);
    avformat_close_input(&inputCtx);
    avformat_free_context(outputCtx);
    
    QFile::remove(m_filePath);
    if (!QFile::rename(tempPath, m_filePath)) {
        QMessageBox::critical(this, "Error", "Failed to replace original file");
        return false;
    }
    
    return true;
}

void MetadataEditor::onSave()
{
    m_metadata.title = m_titleEdit->text().trimmed();
    m_metadata.artist = m_artistEdit->text().trimmed();
    m_metadata.album = m_albumEdit->text().trimmed();
    m_metadata.year = m_yearEdit->text().trimmed();
    m_metadata.genre = m_genreEdit->text().trimmed();
    m_metadata.comment = m_commentEdit->toPlainText().trimmed();
    
    if (m_metadata.title.isEmpty() && m_metadata.artist.isEmpty()) {
        QMessageBox::warning(this, "Validation", "At least title or artist must be filled");
        return;
    }
    
    m_statusLabel->setText("Saving metadata...");
    
    if (saveMetadata()) {
        QMessageBox::information(this, "Success", "Metadata saved successfully!");
        accept();
    } else {
        m_statusLabel->setText("Failed to save metadata");
    }
}

void MetadataEditor::onCancel()
{
    reject();
}

AudioMetadata MetadataEditor::getMetadata() const
{
    return m_metadata;
}
