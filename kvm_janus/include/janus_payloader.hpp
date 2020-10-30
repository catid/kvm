// Copyright 2020 Christopher A. Taylor

/*
    Wrap H.264 video frames in RTP frames for WebRTC transport with Janus-gateway
*/

#pragma once

#include <stdint.h>
#include <functional>

namespace janus {


//------------------------------------------------------------------------------
// RTP Payloader

using RtpCallback = std::function<void(const uint8_t* rtp_data, int rtp_bytes)>;

bool WrapH264Rtp(
    uint64_t frame_number,
    uint64_t shutter_usec,
    const uint8_t* video,
    int bytes,
    bool keyframes,
    RtpCallback callback);


} // namespace janus
