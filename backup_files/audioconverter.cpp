#include "audioconverter.h"
#include <QDebug>
#include <QFile>
#include<memory>

//helper for managing AVFrame pointers with unique_ptr

namespace {
    struct AvFrameDeleter {
        void operator()(AVFrame *f) const {
            if (f) av_frame_free(&f);
        }
    };

    using FramePtr = std::unique_ptr<AVFrame, AvFrameDeleter>;
}

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

        // Set MP3 encoder options for proper compression
        AVDictionary *opts = nullptr;
        av_dict_set(&opts, "compression_level", "2", 0);  // Good quality/speed balance
        av_dict_set_int(&opts, "reservoir", 0, 0);  // Disable bit reservoir for better compatibility
        
        if (avcodec_open2(outputCodecCtx, outputCodec, &opts) < 0) {
            av_dict_free(&opts);
            errorMessage = "Failed to open output codec";
            break;
        }
        
        av_dict_free(&opts);

        if (avcodec_parameters_from_context(outputStream->codecpar, outputCodecCtx) < 0) {
            errorMessage = "Failed to copy output codec parameters";
            break;
        }

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

    emit conversionComplete(success, success ? "Conversion completed successfully" : errorMessage);
}

void AudioConverter::cancel()
{
    m_cancelled = true;
}

bool AudioConverter::openInputFile(const QString &inputPath, AVFormatContext **inputFormatCtx)
{
    if (avformat_open_input(inputFormatCtx, inputPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        return false;
    }

    if (avformat_find_stream_info(*inputFormatCtx, nullptr) < 0) {
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
        return false;
    }

    AVStream *outputStream = avformat_new_stream(*outputFormatCtx, nullptr);
    if (!outputStream) {
        return false;
    }

    if (!((*outputFormatCtx)->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&(*outputFormatCtx)->pb, outputPath.toUtf8().constData(), AVIO_FLAG_WRITE) < 0) {
            return false;
        }
    }

    return true;
}

bool AudioConverter::convertAudio(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx,
                                   AVCodecContext *inputCodecCtx, AVCodecContext *outputCodecCtx)
{
    SwrContext *swrCtx = nullptr;
    AVAudioFifo *fifo = nullptr;
    AVPacket *inputPacket = av_packet_alloc();
    AVPacket *outputPacket = av_packet_alloc();
    AVFrame *inputFrame = av_frame_alloc();
    AVFrame *outputFrame = av_frame_alloc();

    if (!inputPacket || !outputPacket || !inputFrame || !outputFrame) {
        // Cleanup any allocated resources
        if (inputPacket) av_packet_free(&inputPacket);
        if (outputPacket) av_packet_free(&outputPacket);
        if (inputFrame) av_frame_free(&inputFrame);
        if (outputFrame) av_frame_free(&outputFrame);
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
        // Cleanup allocated resources
        av_frame_free(&inputFrame);
        av_frame_free(&outputFrame);
        av_packet_free(&inputPacket);
        av_packet_free(&outputPacket);
        if (swrCtx) swr_free(&swrCtx);
        return false;
    }

    // Create audio FIFO buffer to handle variable frame sizes
    fifo = av_audio_fifo_alloc(outputCodecCtx->sample_fmt,
                                outputCodecCtx->ch_layout.nb_channels,
                                outputCodecCtx->frame_size * 2);
    if (!fifo) {
        av_frame_free(&inputFrame);
        av_frame_free(&outputFrame);
        av_packet_free(&inputPacket);
        av_packet_free(&outputPacket);
        swr_free(&swrCtx);
        return false;
    }

    int64_t totalDuration = inputFormatCtx->duration;
    int64_t currentPts = 0;

    // Find audio stream index
    int audioStreamIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    int packetCount = 0;
    int frameCount = 0;
    int encodedPacketCount = 0;

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
                av_frame_unref(inputFrame);
                continue;
            }

            // Convert/resample audio
            int frame_count = swr_convert(swrCtx,
                                          outputFrame->data, outputFrame->nb_samples,
                                          (const uint8_t **)inputFrame->data, inputFrame->nb_samples);

            av_frame_unref(inputFrame);

            if (frame_count < 0) {
                av_frame_unref(outputFrame);
                continue;
            }

            if (frame_count == 0) {
                continue;
            }

            outputFrame->nb_samples = frame_count;

            // Add resampled samples to FIFO
            if (av_audio_fifo_write(fifo, (void**)outputFrame->data, frame_count) < frame_count) {
                av_frame_unref(outputFrame);
                continue;
            }

            // Encode frames from FIFO when we have enough samples
            int encoder_frame_size = outputCodecCtx->frame_size;
            if (encoder_frame_size <= 0) {
                encoder_frame_size = ConverterConstants::MP3_DEFAULT_FRAME_SIZE;
            }

            while (av_audio_fifo_size(fifo) >= encoder_frame_size) {
                AVFrame *encFrame = av_frame_alloc();
                encFrame->nb_samples = encoder_frame_size;
                encFrame->format = outputCodecCtx->sample_fmt;
                av_channel_layout_copy(&encFrame->ch_layout, &outputCodecCtx->ch_layout);
                encFrame->sample_rate = outputCodecCtx->sample_rate;
                encFrame->pts = currentPts;
                currentPts += encoder_frame_size;

                if (av_frame_get_buffer(encFrame, 0) < 0) {
                    av_frame_free(&encFrame);
                    break;
                }

                // Read samples from FIFO
                if (av_audio_fifo_read(fifo, (void**)encFrame->data, encoder_frame_size) < encoder_frame_size) {
                    av_frame_free(&encFrame);
                    break;
                }

                // Send frame to encoder
                ret = avcodec_send_frame(outputCodecCtx, encFrame);
                av_frame_free(&encFrame);

                if (ret < 0) {
                    break;
                }

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

            // Update progress
            if (totalDuration > 0) {
                int percentage = (inputPacket->pts * 100) / totalDuration;
                emit progressUpdated(qBound(0, percentage, 100));
            }
        }

        av_packet_unref(inputPacket);
    }

    if (!m_cancelled) {
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
                    // Add to FIFO
                    av_audio_fifo_write(fifo, (void**)outputFrame->data, frame_count);
                }
            }
            av_frame_unref(inputFrame);
        }

        // Flush resampler - get buffered samples
        while (true) {
            av_frame_unref(outputFrame);
            outputFrame->format = outputCodecCtx->sample_fmt;
            av_channel_layout_copy(&outputFrame->ch_layout, &outputCodecCtx->ch_layout);
            outputFrame->sample_rate = outputCodecCtx->sample_rate;
            outputFrame->nb_samples = outputCodecCtx->frame_size > 0 ? outputCodecCtx->frame_size : 1600;

            if (av_frame_get_buffer(outputFrame, 0) < 0) {
                break;
            }

            int frame_count = swr_convert(swrCtx, outputFrame->data, outputFrame->nb_samples, nullptr, 0);
            if (frame_count <= 0) {
                break;
            }

            outputFrame->nb_samples = frame_count;
            // Add to FIFO
            av_audio_fifo_write(fifo, (void**)outputFrame->data, frame_count);
        }

        // Encode all remaining samples in FIFO
        int encoder_frame_size = outputCodecCtx->frame_size;
        if (encoder_frame_size <= 0) {
            encoder_frame_size = ConverterConstants::MP3_DEFAULT_FRAME_SIZE;
        }

        while (av_audio_fifo_size(fifo) > 0) {
            int samples_in_fifo = av_audio_fifo_size(fifo);
            int samples_to_encode = samples_in_fifo >= encoder_frame_size ? encoder_frame_size : samples_in_fifo;

            AVFrame *encFrame = av_frame_alloc();
            encFrame->nb_samples = samples_to_encode;
            encFrame->format = outputCodecCtx->sample_fmt;
            av_channel_layout_copy(&encFrame->ch_layout, &outputCodecCtx->ch_layout);
            encFrame->sample_rate = outputCodecCtx->sample_rate;
            encFrame->pts = currentPts;
            currentPts += samples_to_encode;

            if (av_frame_get_buffer(encFrame, 0) >= 0) {
                if (av_audio_fifo_read(fifo, (void**)encFrame->data, samples_to_encode) == samples_to_encode) {
                    avcodec_send_frame(outputCodecCtx, encFrame);
                    int ret;
                    while ((ret = avcodec_receive_packet(outputCodecCtx, outputPacket)) >= 0) {
                        encodedPacketCount++;
                        outputPacket->stream_index = 0;
                        av_packet_rescale_ts(outputPacket, outputCodecCtx->time_base,
                                             outputFormatCtx->streams[0]->time_base);
                        av_interleaved_write_frame(outputFormatCtx, outputPacket);
                        av_packet_unref(outputPacket);
                    }
                }
            }
            av_frame_free(&encFrame);
        }

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
    }

    // Cleanup
    if (fifo) {
        av_audio_fifo_free(fifo);
    }
    av_frame_free(&inputFrame);
    av_frame_free(&outputFrame);
    av_packet_free(&inputPacket);
    av_packet_free(&outputPacket);
    swr_free(&swrCtx);

    emit progressUpdated(100);
    return !m_cancelled;
}

//qatomic integer for cancellation flag
//fix this later 

// QAtomicInteger<bool> m_cancelled;

// bool AudioConverter::convertAudio(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx,
//                                   AVCodecContext *inputCodecCtx, AVCodecContext *outputCodecCtx);
// {
//     SwrPtr    swrCtx(nullptr);
//     PacketPtr inputPacket(av_packet_alloc());
//     PacketPtr outputPacket(av_packet_alloc());
//     FramePtr  inputFrame(av_frame_alloc());
//     FramePtr  outputFrame(av_frame_alloc());

//     if (!inputPacket || !outputPacket || !inputFrame || !outputFrame) {
//         qDebug() << "AUDIO CONVERTER: Failed to allocate frames/packets";
//         return false;  // All freed automatically
//     }

//     SwrContext *rawSwr = nullptr;
//     if (swr_alloc_set_opts2(&rawSwr,
//                             &outputCodecCtx->ch_layout,
//                             outputCodecCtx->sample_fmt,
//                             outputCodecCtx->sample_rate,
//                             &inputCodecCtx->ch_layout,
//                             inputCodecCtx->sample_fmt,
//                             inputCodecCtx->sample_rate,
//                             0, nullptr) < 0) {
//         qDebug() << "AUDIO CONVERTER: Failed to allocate resampler";
//         return false;
//     }
//     swrCtx.reset(rawSwr);

//     if (swr_init(swrCtx.get()) < 0) {
//         qDebug() << "AUDIO CONVERTER: Failed to initialize resampler";
//         return false;
//     }

void AudioConverter::copyMetadata(AVFormatContext *inputFormatCtx, AVFormatContext *outputFormatCtx)
{
    AVDictionaryEntry *tag = nullptr;
    while ((tag = av_dict_get(inputFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        av_dict_set(&outputFormatCtx->metadata, tag->key, tag->value, 0);
    }
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
