// Copyright 2020 Christopher A. Taylor

#include "kvm_encode.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Encode");


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

static void mmalCallback(MMAL_WRAPPER_T* /*encoder*/)
{
    vcos_semaphore_post(&m_encoder_sem);
}

static const char* mmalErrStr(int error)
{
    switch (error) {
    case MMAL_SUCCESS: return "MMAL_SUCCESS";
    case MMAL_ENOMEM: return "MMAL_ENOMEM [Out of memory]";
    case MMAL_ENOSPC: return "MMAL_ENOSPC [Out of resources]";
    case MMAL_EINVAL: return "MMAL_EINVAL [Invalid arg]";
    case MMAL_ENOSYS: return "MMAL_ENOSYS [Not implemented]";
    case MMAL_ENOENT: return "MMAL_ENOENT [No such file]";
    case MMAL_ENXIO: return "MMAL_ENXIO [No such device]";
    case MMAL_EIO: return "MMAL_EIO [IO error]";
    case MMAL_ESPIPE: return "MMAL_ESPIPE [Illegal Seek]";
    case MMAL_ECORRUPT: return "MMAL_ECORRUPT [Corrupt Data]";
    case MMAL_ENOTREADY: return "MMAL_ENOTREADY [Component Not Ready]";
    case MMAL_ECONFIG: return "MMAL_ECONFIG [Component Not Configured]";
    case MMAL_EISCONN: return "MMAL_EISCONN [Port Already Connected]";
    case MMAL_ENOTCONN: return "MMAL_ENOTCONN [Port Disconnected]";
    case MMAL_EAGAIN: return "MMAL_EAGAIN [Resource Temporarily Unavailable]";
    case MMAL_EFAULT: return "MMAL_EFAULT [Bad Address]";
    }
    return "Unknown";
}


//------------------------------------------------------------------------------
// MmalEncoder

bool MmalEncoder::Initialize(int width, int height, int input_encoding)
{
    ScopedFunction fail_scope([this]() {
        Shutdown();
    });

    Width = width;
    Height = height;

    if (!mmalInit()) {
        return false;
    }

    int r = mmal_wrapper_create(&Encoder, MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_create failed: ", mmalErrStr(r));
        return false;
    }

    PortIn = Encoder->input[0];
    Encoder->status = MMAL_SUCCESS;

    if (PortIn->is_enabled) {
        r = mmal_wrapper_port_disable(PortIn);
        if (r != MMAL_SUCCESS) {
            Logger.Warn("mmal_wrapper_port_disable PortIn failed: ", mmalErrStr(r));
        }
    }

    PortIn->format->type = MMAL_ES_TYPE_VIDEO;
    PortIn->format->encoding = input_encoding;
    PortIn->format->es->video.width = VCOS_ALIGN_UP(width, 32);
    PortIn->format->es->video.height = VCOS_ALIGN_UP(height, 16);
    PortIn->format->es->video.crop.x = 0;
    PortIn->format->es->video.crop.y = 0;
    PortIn->format->es->video.crop.width = width;
    PortIn->format->es->video.crop.height = height;
    PortIn->format->flags = MMAL_ES_FORMAT_FLAG_FRAMED;
    PortIn->buffer_size = 1000 * 1000; // 1 MB
    PortIn->buffer_num = PortIn->buffer_num_recommended * 4;

    Logger.Info("Buffer size: ", PortIn->buffer_size, " bytes");

    r = mmal_port_format_commit(PortIn);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_format_commit PortIn failed: ", mmalErrStr(r));
        return false;
    }

    // FIXME: How to enable zero copy input?  Seems like a big win
    r = mmal_port_parameter_set_boolean(PortIn, MMAL_PARAMETER_ZERO_COPY, MMAL_FALSE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortIn failed: ", mmalErrStr(r));
        return false;
    }

    PortOut = Encoder->output[0];

    if (PortOut->is_enabled) {
        r = mmal_wrapper_port_disable(PortOut);
        if (r != MMAL_SUCCESS) {
            Logger.Warn("mmal_wrapper_port_disable PortOut failed: ", mmalErrStr(r));
        }
    }

    PortOut->format->type = MMAL_ES_TYPE_VIDEO;
    PortOut->format->encoding = MMAL_ENCODING_H264;
    PortOut->format->encoding_variant = MMAL_ENCODING_VARIANT_H264_DEFAULT; // AnnexB
    PortOut->format->bitrate = Settings.Kbps * 1000;
    PortOut->format->es->video.frame_rate.num = Settings.Framerate;
    PortOut->format->es->video.frame_rate.den = 1;
    PortOut->buffer_size = PortOut->buffer_size_recommended * 4;
    PortOut->buffer_num = PortOut->buffer_num_recommended;

    r = mmal_port_format_commit(PortOut);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_format_commit PortOut failed: ", mmalErrStr(r));
        return false;
    }

    MMAL_PARAMETER_VIDEO_PROFILE_T profile{};
    profile.hdr.id = MMAL_PARAMETER_PROFILE;
    profile.hdr.size = sizeof(profile);
    // http://blog.mediacoderhq.com/h264-profiles-and-levels/
    // MMAL_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE
    // MMAL_VIDEO_PROFILE_H264_BASELINE
    // MMAL_VIDEO_PROFILE_H264_MAIN
    // MMAL_VIDEO_PROFILE_H264_HIGH
    profile.profile[0].profile = MMAL_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE;
    profile.profile[0].level = MMAL_VIDEO_LEVEL_H264_4; // Supports 1080p
    r = mmal_port_parameter_set(PortOut, &profile.hdr);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set PortOut MMAL_PARAMETER_PROFILE failed: ", mmalErrStr(r));
        return false;
    }

    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_ZERO_COPY failed: ", mmalErrStr(r));
        return false;
    }

    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_INTRAPERIOD, Settings.GopSize);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_INTRAPERIOD failed: ", mmalErrStr(r));
        return false;
    }

    // MMAL_VIDEO_NALUNITFORMAT_STARTCODES
    // MMAL_VIDEO_NALUNITFORMAT_NALUNITPERBUFFER
    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_NALUNITFORMAT, MMAL_VIDEO_NALUNITFORMAT_STARTCODES);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_NALUNITFORMAT failed: ", mmalErrStr(r));
        return false;
    }

    // Avoid multiple slices
    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_MINIMISE_FRAGMENTATION, MMAL_TRUE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_MINIMISE_FRAGMENTATION failed: ", mmalErrStr(r));
        return false;
    }

    // Setting the value to zero resets to the default (one slice per frame).
    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_MB_ROWS_PER_SLICE, 0);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_MB_ROWS_PER_SLICE failed: ", mmalErrStr(r));
        return false;
    }

    // Allow input to be modified
    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, MMAL_FALSE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT failed: ", mmalErrStr(r));
        return false;
    }

#if 0
    // TBD: Fails to start when this is enabled
    // Note that the encoder produces CAVLC output anyway so not sure why this setting exists
    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_DISABLE_CABAC, MMAL_TRUE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_VIDEO_ENCODE_H264_DISABLE_CABAC failed: ", mmalErrStr(r));
        return false;
    }
#endif

    // Every other P-frame can be dropped (nal_ref_idc = 0 for these frames)
    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_DROPPABLE_PFRAMES, MMAL_FALSE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_VIDEO_DROPPABLE_PFRAMES failed: ", mmalErrStr(r));
        return false;
    }

#if 0 // TBD: Does not seem to do anything to output?
    // Intra-refresh
    MMAL_PARAMETER_VIDEO_INTRA_REFRESH_T ifresh{};
    ifresh.hdr.id = MMAL_PARAMETER_VIDEO_INTRA_REFRESH;
    ifresh.hdr.size = sizeof(ifresh);
    // MMAL_VIDEO_INTRA_REFRESH_CYCLIC
    // MMAL_VIDEO_INTRA_REFRESH_ADAPTIVE
    // MMAL_VIDEO_INTRA_REFRESH_BOTH
    // MMAL_VIDEO_INTRA_REFRESH_CYCLIC_MROWS
    // MMAL_VIDEO_INTRA_REFRESH_PSEUDO_RAND
    ifresh.refresh_mode = MMAL_VIDEO_INTRA_REFRESH_CYCLIC;
    ifresh.air_mbs = 512;
    ifresh.air_ref = 1;
    ifresh.cir_mbs = 412;
    ifresh.pir_mbs = 312;
    r = mmal_port_parameter_set(PortOut, &ifresh.hdr);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set PortOut MMAL_PARAMETER_VIDEO_INTRA_REFRESH failed: ", mmalErrStr(r));
        return false;
    }
#endif

    // Bitrate
    // TBD: Not sure if this is used or not
    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_BIT_RATE, Settings.Kbps * 1000); // 4 Mbps
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_VIDEO_BIT_RATE failed: ", mmalErrStr(r));
        return false;
    }

    // Peak Bitrate
    // TBD: Not sure if this is used or not
    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_PEAK_RATE, Settings.Kbps * 1000); // 4 Mbps
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_VIDEO_ENCODE_PEAK_RATE failed: ", mmalErrStr(r));
        return false;
    }

#if 0
    // FIXME: Setting CBR seems to make it ignore the bitrate?
    // MMAL_VIDEO_ENCODER_RC_MODEL_JVT
    // MMAL_VIDEO_ENCODER_RC_MODEL_VOWIFI
    // MMAL_VIDEO_ENCODER_RC_MODEL_CBR
    MMAL_PARAMETER_VIDEO_ENCODE_RC_MODEL_T model{};
    model.hdr.id = MMAL_PARAMETER_VIDEO_ENCODE_RC_MODEL;
    model.hdr.size = sizeof(model);
    model.rc_model = MMAL_VIDEO_ENCODER_RC_MODEL_CBR;
    r = mmal_port_parameter_set(PortOut, &model.hdr);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set PortOut MMAL_PARAMETER_VIDEO_ENCODE_RC_MODEL failed: ", mmalErrStr(r));
        return false;
    }
#endif

#if 0 // Disabled because it does nothing
    // MMAL_VIDEO_RATECONTROL_DEFAULT
    // MMAL_VIDEO_RATECONTROL_VARIABLE - Default I think
    // MMAL_VIDEO_RATECONTROL_VARIABLE_SKIP_FRAMES - Seems to do nothing different
    // MMAL_VIDEO_RATECONTROL_CONSTANT - Unsupported, does not work
    // MMAL_VIDEO_RATECONTROL_CONSTANT_SKIP_FRAMES - Unsupported, does not work
    MMAL_PARAMETER_VIDEO_RATECONTROL_T rc{};
    rc.hdr.id = MMAL_PARAMETER_RATECONTROL;
    rc.hdr.size = sizeof(rc);
    rc.control = MMAL_VIDEO_RATECONTROL_CONSTANT_SKIP_FRAMES;
    r = mmal_port_parameter_set(PortOut, &rc.hdr);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set PortOut MMAL_PARAMETER_RATECONTROL failed: ", mmalErrStr(r));
        return false;
    }
#endif

    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_INITIAL_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_QP_P, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_RC_SLICE_DQUANT, 1);

    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, 16);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT failed: ", mmalErrStr(r));
        return false;
    }
    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, 34);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT failed: ", mmalErrStr(r));
        return false;
    }

    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_FRAME_LIMIT_BITS, 1000000);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_VIDEO_ENCODE_FRAME_LIMIT_BITS failed: ", mmalErrStr(r));
        return false;
    }

#if 0 // Attic:
    // Not sure what this is
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_EEDE_ENABLE, 0);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_EEDE_LOSSRATE, 0);

    MMAL_PARAMETER_FRAME_RATE_T rate{};
    rate.hdr.id = MMAL_PARAMETER_VIDEO_FRAME_RATE;
    rate.hdr.size = sizeof(rate);
    rate.frame_rate.num = fps;
    rate.frame_rate.den = 1;
    r = mmal_port_parameter_set(PortOut, &rate.hdr);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set PortOut MMAL_PARAMETER_VIDEO_FRAME_RATE failed: ", mmalErrStr(r));
        return false;
    }

    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_INITIAL_QUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_QP_P, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_RC_SLICE_DQUANT, 1);
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_FRAME_LIMIT_BITS, 1);

    // Causes it to hang
    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_LOW_LATENCY, MMAL_TRUE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_VIDEO_ENCODE_H264_AU_DELIMITERS failed: ", mmalErrStr(r));
        return false;
    }

    // What is this?  Thought deblocking was only on the decoder.  Maybe it helps with JPEG decoded input?
    //fail |= mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_DEBLOCK_IDC, 0);

    // MMAL_VIDEO_ENCODER_H264_MB_4x4_INTRA
    // MMAL_VIDEO_ENCODER_H264_MB_8x8_INTRA
    // MMAL_VIDEO_ENCODER_H264_MB_16x16_INTRA
    r = mmal_port_parameter_set_uint32(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_MB_INTRA_MODE, MMAL_VIDEO_ENCODER_H264_MB_8x8_INTRA);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_uint32 PortOut MMAL_PARAMETER_VIDEO_ENCODE_H264_MB_INTRA_MODE failed: ", mmalErrStr(r));
        return false;
    }

    // MMAL_PARAMETER_VIDEO_ENCODE_HEADERS_WITH_FRAME reports: Not implemented

#endif // Attic

    // No AUDs
    r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_AU_DELIMITERS, MMAL_FALSE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_VIDEO_ENCODE_H264_AU_DELIMITERS failed: ", mmalErrStr(r));
        return false;
    }

    Encoder->callback = mmalCallback;

    r = mmal_wrapper_port_enable(PortIn, MMAL_WRAPPER_FLAG_PAYLOAD_ALLOCATE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_port_enable PortIn failed: ", mmalErrStr(r));
        return false;
    }

    r = mmal_wrapper_port_enable(PortOut, MMAL_WRAPPER_FLAG_PAYLOAD_ALLOCATE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_port_enable PortOut failed: ", mmalErrStr(r));
        return false;
    }

    fail_scope.Cancel();
    return true;
}

void MmalEncoder::Shutdown()
{
    if (Encoder) {
        // Disable the port so we do not hang on shutdown
        int r = mmal_wrapper_port_disable(PortIn);
        if (r != MMAL_SUCCESS) {
            Logger.Warn("mmal_wrapper_port_disable PortIn failed: ", mmalErrStr(r));
        }
        r = mmal_wrapper_port_disable(PortOut);
        if (r != MMAL_SUCCESS) {
            Logger.Warn("mmal_wrapper_port_disable PortOut failed: ", mmalErrStr(r));
        }

        mmal_wrapper_destroy(Encoder);
        Encoder = nullptr;
    }
}

uint8_t* MmalEncoder::Encode(const std::shared_ptr<Frame>& frame, bool force_keyframe, int& bytes)
{
    if (!Encoder) {
        int input_encoding = 0;
        if (frame->Format == PixelFormat::YUV420P) {
            input_encoding = MMAL_ENCODING_I420;
        } else if (frame->Format == PixelFormat::NV12) {
            input_encoding = MMAL_ENCODING_NV12;
        } else if (frame->Format == PixelFormat::RGB24) {
            input_encoding = MMAL_ENCODING_RGB24;
        } else {
            // Note: YUV422 is not supported by this hardware (Tested!)
            // Also verified that YUYV format is not supported.
            Logger.Error("Unsupported format");
            return nullptr;
        }
        if (!Initialize(frame->Width, frame->Height, input_encoding)) {
            Logger.Error("Initialize failed");
            return nullptr;
        }

        Logger.Info("MMAL encoder initialized");
    }

    int r;

    if (force_keyframe) {
        r = mmal_port_parameter_set_boolean(PortOut, MMAL_PARAMETER_VIDEO_REQUEST_I_FRAME, MMAL_TRUE);
        if (r != MMAL_SUCCESS) {
            Logger.Error("mmal_port_parameter_set_boolean PortOut MMAL_PARAMETER_VIDEO_REQUEST_I_FRAME failed: ", mmalErrStr(r));
            return nullptr;
        }
    }

    Data.resize(0);

    bool eos = false, sent = false;

    while (!eos)
    {
        MMAL_BUFFER_HEADER_T* out = nullptr;
        while (mmal_wrapper_buffer_get_empty(PortOut, &out, 0) == MMAL_SUCCESS) {
            r = mmal_port_send_buffer(PortOut, out);
            if (r != MMAL_SUCCESS) {
                Logger.Error("mmal_port_send_buffer PortOut failed: ", mmalErrStr(r));
                return nullptr;
            }
        }

        MMAL_BUFFER_HEADER_T* in = nullptr;
        if (!sent && mmal_wrapper_buffer_get_empty(PortIn, &in, 0) == MMAL_SUCCESS) {
            in->data = frame->Planes[0];
            if (frame->Format == PixelFormat::YUV420P) {
                in->length = in->alloc_size = Width * Height * 3 / 2;
            } else if (frame->Format == PixelFormat::RGB24) {
                in->length = in->alloc_size = Width * Height * 3;
            }
            in->offset = 0;
            in->flags = MMAL_BUFFER_HEADER_FLAG_EOS; // Required
            r = mmal_port_send_buffer(PortIn, in);
            if (r != MMAL_SUCCESS) {
                Logger.Error("mmal_port_send_buffer PortIn failed: ", mmalErrStr(r));
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
            Logger.Error("mmal_wrapper_buffer_get_full failed: ", mmalErrStr(r));
            return nullptr;
        }

        if ((out->flags & MMAL_BUFFER_HEADER_FLAG_EOS) != 0) {
            eos = true;
        }

        //Logger.Info("Read video bytes: ", out->length);

        size_t previous_bytes = Data.size();
        Data.resize(previous_bytes + out->length);
        memcpy(Data.data() + previous_bytes, out->data, out->length);

        mmal_buffer_header_release(out);
    }

    r = mmal_port_flush(PortOut);
    if (r != MMAL_SUCCESS) {
        Logger.Warn("mmal_port_flush failed: ", mmalErrStr(r));
    }

    bytes = (int)Data.size();
    return Data.data();
}


} // namespace kvm
