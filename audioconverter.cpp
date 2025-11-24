#include "audioconverter.h"
#include <QDebug>
#include <QFile>

AudioConverter::AudioConverter(QObject *parent)
    : QObject(parent)
    , m_cancelled(false)
{
}

AudioConverter::~AudioConverter()
{
}

void AudioConverter::convertToMP3(const QString &inputPath, const QString &outputPath, BitratePreset bitrate)
{
    qDebug() << "==========================================";
    qDebug() << "AUDIO CONVERTER: Starting conversion";
    qDebug() << "  Input:" << inputPath;
    qDebug() << "  Output:" << outputPath;
    qDebug() << "  Bitrate:" << bitrate / 1000 << "kbps";
    qDebug() << "==========================================";

    m_cancelled = false;
    emit conversionStarted();

    AVFormatContext *inputFormatCtx = nullptr;
    AVFormatContext *outputFormatCtx = nullptr;
    AVCodecContext *inputCodecCtx = nullptr;
    AVCodecContext *outputCodecCtx = nullptr;

    bool success = false;
    QString errorMessage;

    do {
        if (!openInputFile(inputPath, &inputFormatCtx)) {
            errorMessage = "Failed to open input file";
            break;
        }

        // Find audio stream
        int audioStreamIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioStreamIndex < 0) {
            errorMessage = "No audio stream found";
            break;
        }

        AVStream *inputStream = inputFormatCtx->streams[audioStreamIndex];
        
        // Create input codec context
        const AVCodec *inputCodec = avcodec_find_decoder(inputStream->codecpar->codec_id);
        if (!inputCodec) {
            errorMessage = "Input codec not found";
            break;
        }

        inputCodecCtx = avcodec_alloc_context3(inputCodec);
        if (!inputCodecCtx) {
            errorMessage = "Failed to allocate input codec context";
            break;
        }

        if (avcodec_parameters_to_context(inputCodecCtx, inputStream->codecpar) < 0) {
            errorMessage = "Failed to copy codec parameters";
            break;
        }

        if (avcodec_open2(inputCodecCtx, inputCodec, nullptr) < 0) {
            errorMessage = "Failed to open input codec";
            break;
        }

        qDebug() << "AUDIO CONVERTER: Input codec opened successfully";
        qDebug() << "  Codec:" << inputCodec->long_name;
        qDebug() << "  Sample Rate:" << inputCodecCtx->sample_rate;
        qDebug() << "  Channels:" << inputCodecCtx->ch_layout.nb_channels;

        if (!openOutputFile(outputPath, &outputFormatCtx, inputCodecCtx, bitrate)) {
            errorMessage = "Failed to create output file";
            break;
        }

        // Find output stream
        AVStream *outputStream = outputFormatCtx->streams[0];
        
        // Create output codec context
        const AVCodec *outputCodec = avcodec_find_encoder(AV_CODEC_ID_MP3);
        if (!outputCodec) {
            errorMessage = "MP3 encoder not found";
            break;
        }

        outputCodecCtx = avcodec_alloc_context3(outputCodec);
        if (!outputCodecCtx) {
            errorMessage = "Failed to allocate output codec context";
            break;
        }

        outputCodecCtx->bit_rate = bitrate;
        outputCodecCtx->sample_rate = inputCodecCtx->sample_rate;
        av_channel_layout_copy(&outputCodecCtx->ch_layout, &inputCodecCtx->ch_layout);
        
        // Get supported sample format (use first supported format)
        outputCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; // MP3 typically uses floating-point planar
        
        outputStream->time_base = AVRational{1, inputCodecCtx->sample_rate};

        if (outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
            outputCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if (avcodec_open2(outputCodecCtx, outputCodec, nullptr) < 0) {
            errorMessage = "Failed to open output codec";
            break;
        }

        if (avcodec_parameters_from_context(outputStream->codecpar, outputCodecCtx) < 0) {
            errorMessage = "Failed to copy output codec parameters";
            break;
        }

        qDebug() << "AUDIO CONVERTER: Output codec configured";
        qDebug() << "  Codec:" << outputCodec->long_name;
        qDebug() << "  Bitrate:" << outputCodecCtx->bit_rate / 1000 << "kbps";

        // Copy metadata
        copyMetadata(inputFormatCtx, outputFormatCtx);

        // Write output file header
        if (avformat_write_header(outputFormatCtx, nullptr) < 0) {
            errorMessage = "Failed to write output file header";
            break;
        }

        // Convert audio
        if (!convertAudio(inputFormatCtx, outputFormatCtx, inputCodecCtx, outputCodecCtx)) {
            errorMessage = m_cancelled ? "Conversion cancelled" : "Conversion failed";
            break;
        }

        // Write output file trailer
        av_write_trailer(outputFormatCtx);

        success = true;
        qDebug() << "AUDIO CONVERTER: Conversion completed successfully";

    } while (false);

    // Cleanup
    if (inputCodecCtx) {
        avcodec_free_context(&inputCodecCtx);
    }
    if (outputCodecCtx) {
        avcodec_free_context(&outputCodecCtx);
    }
    if (inputFormatCtx) {
        avformat_close_input(&inputFormatCtx);
    }
    if (outputFormatCtx) {
        if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&outputFormatCtx->pb);
        }
        avformat_free_context(outputFormatCtx);
    }

    qDebug() << "==========================================";
    emit conversionComplete(success, success ? "Conversion completed successfully" : errorMessage);
}

void AudioConverter::cancel()
{
    m_cancelled = true;
    qDebug() << "AUDIO CONVERTER: Cancellation requested";
}

bool AudioConverter::openInputFile(const QString &inputPath, AVFormatContext **inputFormatCtx)
{
    if (avformat_open_input(inputFormatCtx, inputPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        qDebug() << "AUDIO CONVERTER: Failed to open input file";
        return false;
    }

    if (avformat_find_stream_info(*inputFormatCtx, nullptr) < 0) {
        qDebug() << "AUDIO CONVERTER: Failed to find stream info";
        avformat_close_input(inputFormatCtx);
        return false;
    }

    return true;
}

bool AudioConverter::openOutputFile(const QString &outputPath, AVFormatContext **outputFormatCtx,
                                     AVCodecContext *inputCodecCtx, BitratePreset bitrate)
{
    avformat_alloc_output_context2(outputFormatCtx, nullptr, nullptr, outputPath.toUtf8().constData());
    if (!*outputFormatCtx) {
        qDebug() << "AUDIO CONVERTER: Failed to create output context";
        return false;
    }

    AVStream *outputStream = avformat_new_stream(*outputFormatCtx, nullptr);
    if (!outputStream) {
        qDebug() << "AUDIO CONVERTER: Failed to create output stream";
        return false;
    }

    if (!((*outputFormatCtx)->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&(*outputFormatCtx)->pb, outputPath.toUtf8().constData(), AVIO_FLAG_WRITE) < 0) {
            qDebug() << "AUDIO CONVERTER: Failed to open output file";
            return false;
        }
    }

    return true;
}

bool AudioConverter::convertAudio(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx,
                                   AVCodecContext *inputCodecCtx, AVCodecContext *outputCodecCtx)
{
    SwrContext *swrCtx = nullptr;
    AVPacket *inputPacket = av_packet_alloc();
    AVPacket *outputPacket = av_packet_alloc();
    AVFrame *inputFrame = av_frame_alloc();
    AVFrame *outputFrame = av_frame_alloc();

    if (!inputPacket || !outputPacket || !inputFrame || !outputFrame) {
        qDebug() << "AUDIO CONVERTER: Failed to allocate frames/packets";
        return false;
    }

    // Setup resampler
    swr_alloc_set_opts2(&swrCtx,
                        &outputCodecCtx->ch_layout,
                        outputCodecCtx->sample_fmt,
                        outputCodecCtx->sample_rate,
                        &inputCodecCtx->ch_layout,
                        inputCodecCtx->sample_fmt,
                        inputCodecCtx->sample_rate,
                        0, nullptr);

    if (!swrCtx || swr_init(swrCtx) < 0) {
        qDebug() << "AUDIO CONVERTER: Failed to initialize resampler";
        return false;
    }

    int64_t totalDuration = inputFormatCtx->duration;
    int64_t currentPts = 0;

    // Find audio stream index
    int audioStreamIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    int packetCount = 0;
    int frameCount = 0;
    int encodedPacketCount = 0;
    qDebug() << "AUDIO CONVERTER: Starting main conversion loop";
    qDebug() << "  Total duration:" << totalDuration << "microseconds";

    while (av_read_frame(inputFormatCtx, inputPacket) >= 0) {
        packetCount++;
        if (m_cancelled) {
            av_packet_unref(inputPacket);
            break;
        }

        if (inputPacket->stream_index != audioStreamIndex) {
            av_packet_unref(inputPacket);
            continue;
        }

        if (avcodec_send_packet(inputCodecCtx, inputPacket) < 0) {
            av_packet_unref(inputPacket);
            continue;
        }

        while (avcodec_receive_frame(inputCodecCtx, inputFrame) >= 0) {
            frameCount++;
            
            if (m_cancelled) {
                av_frame_unref(inputFrame);
                break;
            }

            // Unref output frame before reuse
            av_frame_unref(outputFrame);

            // Calculate required output samples
            int dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swrCtx, inputCodecCtx->sample_rate) + inputFrame->nb_samples,
                outputCodecCtx->sample_rate,
                inputCodecCtx->sample_rate,
                AV_ROUND_UP);
            
            outputFrame->format = outputCodecCtx->sample_fmt;
            av_channel_layout_copy(&outputFrame->ch_layout, &outputCodecCtx->ch_layout);
            outputFrame->sample_rate = outputCodecCtx->sample_rate;
            outputFrame->nb_samples = dst_nb_samples;

            int ret = av_frame_get_buffer(outputFrame, 0);
            if (ret < 0) {
                qDebug() << "AUDIO CONVERTER: Failed to allocate output frame buffer, error:" << ret;
                av_frame_unref(inputFrame);
                continue;
            }

            // Convert/resample audio
            int frame_count = swr_convert(swrCtx,
                                          outputFrame->data, outputFrame->nb_samples,
                                          (const uint8_t **)inputFrame->data, inputFrame->nb_samples);

            av_frame_unref(inputFrame);

            if (frame_count < 0) {
                qDebug() << "AUDIO CONVERTER: Resampling failed, error:" << frame_count;
                av_frame_unref(outputFrame);
                continue;
            }

            if (frame_count == 0) {
                continue;
            }

            outputFrame->nb_samples = frame_count;
            outputFrame->pts = currentPts;
            currentPts += frame_count;

            // MP3 encoder expects exactly frame_size samples (1152 for MP3)
            // If we have more, we need to split into multiple frames
            int encoder_frame_size = outputCodecCtx->frame_size;
            if (encoder_frame_size <= 0) {
                encoder_frame_size = 1152; // Default MP3 frame size
            }

            int samples_sent = 0;
            while (samples_sent < frame_count) {
                AVFrame *encoderFrame = av_frame_alloc();
                encoderFrame->format = outputCodecCtx->sample_fmt;
                av_channel_layout_copy(&encoderFrame->ch_layout, &outputCodecCtx->ch_layout);
                encoderFrame->sample_rate = outputCodecCtx->sample_rate;
                
                int samples_to_send = qMin(encoder_frame_size, frame_count - samples_sent);
                encoderFrame->nb_samples = samples_to_send;
                encoderFrame->pts = outputFrame->pts + samples_sent;
                
                if (av_frame_get_buffer(encoderFrame, 0) >= 0) {
                    // Copy samples from outputFrame to encoderFrame
                    int channels = outputCodecCtx->ch_layout.nb_channels;
                    int bytes_per_sample = av_get_bytes_per_sample(outputCodecCtx->sample_fmt);
                    
                    if (av_sample_fmt_is_planar(outputCodecCtx->sample_fmt)) {
                        // Planar format - each channel in separate plane
                        for (int ch = 0; ch < channels; ch++) {
                            memcpy(encoderFrame->data[ch],
                                   outputFrame->data[ch] + (samples_sent * bytes_per_sample),
                                   samples_to_send * bytes_per_sample);
                        }
                    } else {
                        // Interleaved format
                        memcpy(encoderFrame->data[0],
                               outputFrame->data[0] + (samples_sent * channels * bytes_per_sample),
                               samples_to_send * channels * bytes_per_sample);
                    }
                    
                    // Send frame to encoder
                    ret = avcodec_send_frame(outputCodecCtx, encoderFrame);
                    if (ret >= 0) {
                        // Receive encoded packets
                        while (ret >= 0) {
                            ret = avcodec_receive_packet(outputCodecCtx, outputPacket);
                            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                                break;
                            }
                            if (ret < 0) {
                                break;
                            }

                            encodedPacketCount++;
                            outputPacket->stream_index = 0;
                            av_packet_rescale_ts(outputPacket, outputCodecCtx->time_base, 
                                                 outputFormatCtx->streams[0]->time_base);
                            
                            av_interleaved_write_frame(outputFormatCtx, outputPacket);
                            av_packet_unref(outputPacket);
                        }
                    }
                }
                
                av_frame_free(&encoderFrame);
                samples_sent += samples_to_send;
            }

            // Update progress
            if (totalDuration > 0) {
                int percentage = (inputPacket->pts * 100) / totalDuration;
                emit progressUpdated(qBound(0, percentage, 100));
            }
        }

        av_packet_unref(inputPacket);
    }

    qDebug() << "AUDIO CONVERTER: Main loop complete";
    qDebug() << "  Packets read:" << packetCount;
    qDebug() << "  Frames decoded:" << frameCount;
    qDebug() << "  Packets encoded:" << encodedPacketCount;

    if (!m_cancelled) {
        qDebug() << "AUDIO CONVERTER: Flushing decoder...";
        // Flush decoder - get any remaining frames
        avcodec_send_packet(inputCodecCtx, nullptr);
        while (avcodec_receive_frame(inputCodecCtx, inputFrame) >= 0) {
            av_frame_unref(outputFrame);
            
            int dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swrCtx, inputCodecCtx->sample_rate) + inputFrame->nb_samples,
                outputCodecCtx->sample_rate,
                inputCodecCtx->sample_rate,
                AV_ROUND_UP);
            
            outputFrame->format = outputCodecCtx->sample_fmt;
            av_channel_layout_copy(&outputFrame->ch_layout, &outputCodecCtx->ch_layout);
            outputFrame->sample_rate = outputCodecCtx->sample_rate;
            outputFrame->nb_samples = dst_nb_samples;

            if (av_frame_get_buffer(outputFrame, 0) >= 0) {
                int frame_count = swr_convert(swrCtx,
                                              outputFrame->data, outputFrame->nb_samples,
                                              (const uint8_t **)inputFrame->data, inputFrame->nb_samples);
                
                if (frame_count > 0) {
                    outputFrame->nb_samples = frame_count;
                    outputFrame->pts = currentPts;
                    currentPts += frame_count;
                    
                    avcodec_send_frame(outputCodecCtx, outputFrame);
                    while (avcodec_receive_packet(outputCodecCtx, outputPacket) >= 0) {
                        encodedPacketCount++;
                        outputPacket->stream_index = 0;
                        av_packet_rescale_ts(outputPacket, outputCodecCtx->time_base,
                                             outputFormatCtx->streams[0]->time_base);
                        av_interleaved_write_frame(outputFormatCtx, outputPacket);
                        av_packet_unref(outputPacket);
                    }
                }
            }
            av_frame_unref(inputFrame);
        }
        qDebug() << "  Decoder flush: added" << (encodedPacketCount - packetCount) << "packets";

        qDebug() << "AUDIO CONVERTER: Flushing resampler...";
        int resamplerPackets = encodedPacketCount;
        // Flush resampler - get buffered samples
        while (true) {
            av_frame_unref(outputFrame);
            outputFrame->format = outputCodecCtx->sample_fmt;
            av_channel_layout_copy(&outputFrame->ch_layout, &outputCodecCtx->ch_layout);
            outputFrame->sample_rate = outputCodecCtx->sample_rate;
            outputFrame->nb_samples = outputCodecCtx->frame_size > 0 ? outputCodecCtx->frame_size : 1152;

            if (av_frame_get_buffer(outputFrame, 0) < 0) {
                break;
            }

            int frame_count = swr_convert(swrCtx, outputFrame->data, outputFrame->nb_samples, nullptr, 0);
            if (frame_count <= 0) {
                break;
            }

            outputFrame->nb_samples = frame_count;
            outputFrame->pts = currentPts;
            currentPts += frame_count;

            avcodec_send_frame(outputCodecCtx, outputFrame);
            while (avcodec_receive_packet(outputCodecCtx, outputPacket) >= 0) {
                encodedPacketCount++;
                outputPacket->stream_index = 0;
                av_packet_rescale_ts(outputPacket, outputCodecCtx->time_base,
                                     outputFormatCtx->streams[0]->time_base);
                av_interleaved_write_frame(outputFormatCtx, outputPacket);
                av_packet_unref(outputPacket);
            }
        }
        qDebug() << "  Resampler flush: added" << (encodedPacketCount - resamplerPackets) << "packets";

        qDebug() << "AUDIO CONVERTER: Flushing encoder...";
        int encoderFlushStart = encodedPacketCount;
        // Flush encoder - get remaining packets
        avcodec_send_frame(outputCodecCtx, nullptr);
        int ret;
        while ((ret = avcodec_receive_packet(outputCodecCtx, outputPacket)) >= 0) {
            encodedPacketCount++;
            outputPacket->stream_index = 0;
            av_packet_rescale_ts(outputPacket, outputCodecCtx->time_base,
                                 outputFormatCtx->streams[0]->time_base);
            av_interleaved_write_frame(outputFormatCtx, outputPacket);
            av_packet_unref(outputPacket);
        }
        qDebug() << "  Encoder flush: added" << (encodedPacketCount - encoderFlushStart) << "packets";
        qDebug() << "AUDIO CONVERTER: Total packets written:" << encodedPacketCount;
    }

    // Cleanup
    av_frame_free(&inputFrame);
    av_frame_free(&outputFrame);
    av_packet_free(&inputPacket);
    av_packet_free(&outputPacket);
    swr_free(&swrCtx);

    emit progressUpdated(100);
    return !m_cancelled;
}

void AudioConverter::copyMetadata(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx)
{
    AVDictionaryEntry *tag = nullptr;
    while ((tag = av_dict_get(inputFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        av_dict_set(&outputFormatCtx->metadata, tag->key, tag->value, 0);
    }

    qDebug() << "AUDIO CONVERTER: Metadata copied";
}

// Worker implementation
AudioConverterWorker::AudioConverterWorker(const QString &inputPath, const QString &outputPath,
                                           AudioConverter::BitratePreset bitrate)
    : m_inputPath(inputPath)
    , m_outputPath(outputPath)
    , m_bitrate(bitrate)
    , m_converter(new AudioConverter(this))
{
    connect(m_converter, &AudioConverter::progressUpdated, this, &AudioConverterWorker::progressUpdated);
    connect(m_converter, &AudioConverter::conversionComplete, this, &AudioConverterWorker::finished);
}

void AudioConverterWorker::process()
{
    m_converter->convertToMP3(m_inputPath, m_outputPath, m_bitrate);
}
