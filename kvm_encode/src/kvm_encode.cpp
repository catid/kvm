// Copyright 2020 Christopher A. Taylor

#include "kvm_encode.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Capture");


//------------------------------------------------------------------------------
// Tools

static bool m_sem_init = false;
static VCOS_SEMAPHORE_T m_encoder_sem{};

static bool mmalInit()
{
    if (m_sem_init) {
        return true;
    }
    m_sem_init = true;

    bcm_host_init();

    int r = vcos_semaphore_create(&m_encoder_sem, "encoder sem", 0);
    if (r != VCOS_SUCCESS) {
        Logger.Error("vcos_semaphore_create failed: ", r);
        return false;
    }

    return true;
}

static void mmalCallback(MMAL_WRAPPER_T* encoder)
{
   vcos_semaphore_post(&m_encoder_sem);
}


//------------------------------------------------------------------------------
// MmalEncoder

bool MmalEncoder::Initialize(int width, int height, int input_encoding, int kbps, int fps, int gop)
{
    ScopedFunction fail_scope([this]() {
        Shutdown();
    });

    Width = width;
    Height = height;

    if (!mmalInit()) {
        return false;
    }

    int r = mmal_wrapper_create(&Encoder, MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_create failed: ", r);
        return false;
    }

    PortIn = Encoder->input[0];
    Encoder->status = MMAL_SUCCESS;

    if (PortIn->is_enabled) {
        r = mmal_wrapper_port_disable(PortIn);
        if (r != MMAL_SUCCESS) {
            Logger.Warn("mmal_wrapper_port_disable PortIn failed: ", r);
        }
    }

    //PortIn->format->type = MMAL_ES_TYPE_VIDEO;
    PortIn->format->encoding = input_encoding;
    PortIn->format->es->video.width = VCOS_ALIGN_UP(width, 32);
    PortIn->format->es->video.height = VCOS_ALIGN_UP(height, 16);
    PortIn->format->es->video.crop.x = 0;
    PortIn->format->es->video.crop.y = 0;
    PortIn->format->es->video.crop.width = width;
    PortIn->format->es->video.crop.height = height;
    //PortIn->format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;
    PortIn->buffer_size = PortIn->buffer_size_recommended;
    PortIn->buffer_num = PortIn->buffer_num_recommended;

    r = mmal_port_format_commit(PortIn);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_format_commit PortIn failed: ", r);
        return false;
    }

    r = mmal_port_parameter_set_boolean(PortIn, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortIn failed: ", r);
        return false;
    }

    r = mmal_wrapper_port_enable(PortIn, MMAL_WRAPPER_FLAG_PAYLOAD_ALLOCATE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_port_enable failed: ", r);
        return false;
    }

    PortOut = Encoder->output[0];

    if (PortOut->is_enabled) {
        r = mmal_wrapper_port_disable(PortOut);
        if (r != MMAL_SUCCESS) {
            Logger.Warn("mmal_wrapper_port_disable PortOut failed: ", r);
        }
    }

    //PortOut->format->type = MMAL_ES_TYPE_VIDEO;
    PortOut->format->encoding = MMAL_ENCODING_H264;
    //PortOut->format->encoding_variant = MMAL_ENCODING_VARIANT_H264_DEFAULT; // AnnexB
    PortOut->format->bitrate = kbps;
    PortOut->format->es->video.frame_rate.num = fps;
    PortOut->format->es->video.frame_rate.den = 1;
    PortIn->buffer_size = PortIn->buffer_size_recommended;
    PortIn->buffer_num = PortIn->buffer_num_recommended;

    r = mmal_port_format_commit(PortOut);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_format_commit PortOut failed: ", r);
        return false;
    }

    MMAL_PARAMETER_VIDEO_PROFILE_T profile;
    profile.hdr.id = MMAL_PARAMETER_PROFILE;
    profile.hdr.size = sizeof(profile);
    // MMAL_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE
    // MMAL_VIDEO_PROFILE_H264_BASELINE
    // MMAL_VIDEO_PROFILE_H264_MAIN
    // MMAL_VIDEO_PROFILE_H264_HIGH
    profile.profile[0].profile = MMAL_VIDEO_PROFILE_H264_BASELINE;
    profile.profile[0].level = MMAL_VIDEO_LEVEL_H264_4; // Supports 1080p

    r = mmal_port_parameter_set(PortOut, &profile.hdr);
    if (r != MMAL_SUCCESS)	{
        Logger.Error("mmal_port_parameter_set profile failed: ", r);
        return false;
    }

    int fail = 0;

    // FIXME
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);

    // GOP size = 6
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_INTRAPERIOD, gop);

    // MMAL_VIDEO_ENCODER_RC_MODEL_JVT
    // MMAL_VIDEO_ENCODER_RC_MODEL_VOWIFI
    // MMAL_VIDEO_ENCODER_RC_MODEL_CBR
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_RC_MODEL, MMAL_VIDEO_ENCODER_RC_MODEL_CBR);
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_RATECONTROL, MMAL_VIDEO_RATECONTROL_CONSTANT);

    // MMAL_VIDEO_NALUNITFORMAT_STARTCODES
    // MMAL_VIDEO_NALUNITFORMAT_NALUNITPERBUFFER
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_NALUNITFORMAT, MMAL_VIDEO_NALUNITFORMAT_STARTCODES);

    // Avoid multiple slices
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_MINIMISE_FRAGMENTATION, MMAL_TRUE);

    // Setting the value to zero resets to the default (one slice per frame).
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_MB_ROWS_PER_SLICE, 0);

    // Not sure what this is
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_EEDE_ENABLE, 0);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_EEDE_LOSSRATE, 0);

    // Do not force I-frame
    //fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_REQUEST_I_FRAME, MMAL_FALSE);

    // Not sure how to do this
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_INTRA_REFRESH, MMAL_PARAMETER_VIDEO_INTRA_REFRESH_T);

    // Allow input to be modified
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, MMAL_FALSE);

    // Bitrate
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_BIT_RATE, kbps); // 4 Mbps

    // 30 FPS
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_FRAME_RATE, fps);

    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_INITIAL_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_QP_P, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_RC_SLICE_DQUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_FRAME_LIMIT_BITS, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_PEAK_RATE, 1);

    // Some kind of SVC?  Seems interesting
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_DROPPABLE_PFRAMES, MMAL_FALSE);

    // CABAC helps a lot if we can run it in real-time
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_DISABLE_CABAC, MMAL_FALSE);

    // P-frames only
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_LOW_LATENCY, MMAL_TRUE);

    // No AUDs
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_AU_DELIMITERS, MMAL_FALSE);

    // What is this?  Thought deblocking was only on the decoder.  Maybe it helps with JPEG decoded input?
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_DEBLOCK_IDC, 0);

    // MMAL_VIDEO_ENCODER_H264_MB_4x4_INTRA
    // MMAL_VIDEO_ENCODER_H264_MB_8x8_INTRA
    // MMAL_VIDEO_ENCODER_H264_MB_16x16_INTRA
    fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_MB_INTRA_MODE, MMAL_VIDEO_ENCODER_H264_MB_8x8_INTRA);

    // SPS PPS with frame please
    fail |= mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_HEADERS_WITH_FRAME, MMAL_TRUE);

    if (fail) {
        Logger.Error("mmal_port_parameter_set failed");
        return false;
    }

    r = mmal_wrapper_port_enable(PortOut, MMAL_WRAPPER_FLAG_PAYLOAD_ALLOCATE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_port_enable failed: ", r);
        return false;
    }

    Encoder->callback = mmalCallback;

    fail_scope.Cancel();
    return true;
}

void MmalEncoder::Shutdown()
{
    if (!Encoder) {
        mmal_wrapper_destroy(Encoder);
        Encoder = nullptr;
    }
}

uint8_t* MmalEncoder::Encode(const std::shared_ptr<Frame>& frame, int& bytes)
{
    if (!Encoder) {
        const int kbps = 4000;
        const int fps = 30;
        const int gop = 6;
        int input_encoding = 0;
        if (frame->Format == PixelFormat::YUV422P) {
            input_encoding = MMAL_ENCODING_I422;
        } else if (frame->Format == PixelFormat::YUV420P) {
            input_encoding = MMAL_ENCODING_I420;
        } else if (frame->Format == PixelFormat::YUYV) {
            input_encoding = MMAL_ENCODING_YUYV;
        } else if (frame->Format == PixelFormat::NV12) {
            input_encoding = MMAL_ENCODING_NV12;
        } else if (frame->Format == PixelFormat::RGB24) {
            input_encoding = MMAL_ENCODING_RGB24;
        } else {
            Logger.Error("Unsupported format");
            return nullptr;
        }

        // FIXME: REMOVE THIS!
        input_encoding = MMAL_ENCODING_I420;

        if (!Initialize(frame->Width, frame->Height, input_encoding, kbps, fps, gop)) {
            Logger.Error("Initialize failed");
            return nullptr;
        }
    }

    Data.resize(0);

    int r;
    bool eos = false, sent = false;

    while (!eos)
    {
        Logger.Info("PortOut");

        MMAL_BUFFER_HEADER_T* out = nullptr;
        while (mmal_wrapper_buffer_get_empty(PortOut, &out, 0) == MMAL_SUCCESS) {
            r = mmal_port_send_buffer(PortOut, out);
            if (r != MMAL_SUCCESS) {
                Logger.Error("mmal_port_send_buffer failed: ", r);
                return nullptr;
            }
        }

        Logger.Info("PortIn");

        MMAL_BUFFER_HEADER_T* in = nullptr;
        if (!sent && mmal_wrapper_buffer_get_empty(PortIn, &in, 0) == MMAL_SUCCESS) {
            in->data = frame->Planes[0];
            in->length = in->alloc_size = Width * Height * 2;
            in->offset = 0;
            in->flags = MMAL_BUFFER_HEADER_FLAG_EOS;
            r = mmal_port_send_buffer(PortIn, in);
            if (r != MMAL_SUCCESS) {
                Logger.Error("mmal_port_send_buffer failed: ", r);
                return nullptr;
            }
            sent = true;
        }

        r = mmal_wrapper_buffer_get_full(PortOut, &out, 0);
        if (r == MMAL_EAGAIN) {
            vcos_semaphore_wait(&m_encoder_sem);
            continue;
        }
        if (r != MMAL_SUCCESS) {
            Logger.Error("vcos_semaphore_wait failed: ", r);
            return nullptr;
        }

        if ((out->flags & MMAL_BUFFER_HEADER_FLAG_EOS) != 0) {
            eos = true;
        }

        size_t previous_bytes = Data.size();
        Data.resize(previous_bytes + out->length);
        memcpy(Data.data() + previous_bytes, out->data, out->length);

        mmal_buffer_header_release(out);
    }

    r = mmal_port_flush(PortOut);
    if (r != MMAL_SUCCESS) {
        Logger.Warn("mmal_port_flush failed: ", r);
    }

    bytes = (int)Data.size();
    return Data.data();
}


} // namespace kvm
