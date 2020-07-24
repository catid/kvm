// Copyright 2020 Christopher A. Taylor

#include "kvm_encode.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Capture");


//------------------------------------------------------------------------------
// Tools

static void mmalCallback(MMAL_WRAPPER_T* encoder)
{
}


//------------------------------------------------------------------------------
// MmalEncoder

bool MmalEncoder::Initialize()
{
    bcm_host_init();

    int r = mmal_wrapper_create(&Encoder, MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_create failed: ", r);
        return false;
    }

    MMAL_PORT_T* portIn = Encoder->input[0];
    Encoder->status = MMAL_SUCCESS;

    if (portIn->is_enabled) {
        r = mmal_wrapper_port_disable(portIn);
        if (r != MMAL_SUCCESS) {
            Logger.Error("mmal_wrapper_port_disable portIn failed: ", r);
            return false;
        }
    }

    portIn->format->encoding = INPUT_ENC;
    portIn->format->es->video.width = VCOS_ALIGN_UP(WIDTH, 32);
    portIn->format->es->video.height = VCOS_ALIGN_UP(HEIGHT, 16);
    portIn->format->es->video.crop.x = 0;
    portIn->format->es->video.crop.y = 0;
    portIn->format->es->video.crop.width = WIDTH;
    portIn->format->es->video.crop.height = HEIGHT;

    r = mmal_port_format_commit(portIn);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_format_commit portIn failed: ", r);
        return false;
    }

    // FIXME
    mmal_port_parameter_set_uint32(portIn, MMAL_PARAMETER_ZERO_COPY, 1);

    portIn->buffer_size = portIn->buffer_size_recommended;
    portIn->buffer_num = portIn->buffer_num_recommended;

    r = mmal_wrapper_port_enable(portIn, MMAL_WRAPPER_FLAG_PAYLOAD_ALLOCATE);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_wrapper_port_enable failed: ", r);
        return false;
    }

    MMAL_PORT_T* portOut = encoder->output[0];

    if (portOut->is_enabled) {
        r = mmal_wrapper_port_disable(portOut);
        if (r != MMAL_SUCCESS) {
            Logger.Error("mmal_wrapper_port_disable portOut failed: ", r);
            return false;
        }
    }

    portOut->format->encoding = MMAL_ENCODING_H264;

    r = mmal_port_format_commit(portOut);
    if (r != MMAL_SUCCESS) {
        Logger.Error("mmal_port_format_commit portOut failed: ", r);
        return false;
    }

    // FIXME
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_ZERO_COPY, 1);

    // MMAL_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE
    // MMAL_VIDEO_PROFILE_H264_BASELINE
    // MMAL_VIDEO_PROFILE_H264_MAIN
    // MMAL_VIDEO_PROFILE_H264_HIGH
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_PROFILE, MMAL_VIDEO_PROFILE_H264_MAIN);

    // GOP size = 6
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_INTRAPERIOD, 6);

    // MMAL_VIDEO_ENCODER_RC_MODEL_JVT
    // MMAL_VIDEO_ENCODER_RC_MODEL_VOWIFI
    // MMAL_VIDEO_ENCODER_RC_MODEL_CBR
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_RATECONTROL, MMAL_VIDEO_ENCODER_RC_MODEL_CBR);

    // MMAL_VIDEO_NALUNITFORMAT_STARTCODES
    // MMAL_VIDEO_NALUNITFORMAT_NALUNITPERBUFFER
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_NALUNITFORMAT, MMAL_VIDEO_NALUNITFORMAT_STARTCODES);

    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_MINIMISE_FRAGMENTATION, MMAL_TRUE);

    // Setting the value to zero resets to the default (one slice per frame).
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_MB_ROWS_PER_SLICE, 0);

    // FIXME
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_EEDE_ENABLE, 0);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_EEDE_LOSSRATE, 0);

    // Do not force I-frame
    //mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_REQUEST_I_FRAME, MMAL_FALSE);

    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_INTRA_REFRESH, MMAL_PARAMETER_VIDEO_INTRA_REFRESH_T);
    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, MMAL_FALSE);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_BIT_RATE, 4000000); // 4 Mbps
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_FRAME_RATE, 30);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_MIN_QUANT, 1);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_MAX_QUANT, 1);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_RC_MODEL, MMAL_PARAMETER_VIDEO_ENCODE_RC_MODEL_T);
    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_DROPPABLE_PFRAMES, MMAL_FALSE);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_INITIAL_QUANT, 1);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_QP_P, 1);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_RC_SLICE_DQUANT, 1);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_FRAME_LIMIT_BITS, 1);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_PEAK_RATE, 1);
    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_DISABLE_CABAC, MMAL_FALSE);
    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_LOW_LATENCY, MMAL_FALSE);
    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_AU_DELIMITERS, MMAL_FALSE);
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_DEBLOCK_IDC, 0);

    // MMAL_VIDEO_ENCODER_H264_MB_4x4_INTRA
    // MMAL_VIDEO_ENCODER_H264_MB_8x8_INTRA
    // MMAL_VIDEO_ENCODER_H264_MB_16x16_INTRA
    mmal_port_parameter_set_uint32(portOut, MMAL_PARAMETER_VIDEO_ENCODE_H264_MB_INTRA_MODE, MMAL_PARAMETER_VIDEO_ENCODER_H264_MB_INTRA_MODES_T);
    mmal_port_parameter_set_boolean(portOut, MMAL_PARAMETER_VIDEO_ENCODE_HEADERS_WITH_FRAME, MMAL_FALSE);

    return true;
}

void MmalEncoder::Shutdown()
{
    if (Encoder) {
        mmal_wrapper_destroy(Encoder);
        Encoder = nullptr;
    }
}

uint8_t* MmalEncoder::Encode(int& bytes)
{
    // 
}


} // namespace kvm
