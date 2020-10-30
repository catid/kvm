// Copyright 2020 Christopher A. Taylor

#include "janus_payloader.hpp"

namespace janus {


//------------------------------------------------------------------------------
// RTP Payloader

bool WrapH264Rtp(
    uint64_t frame_number,
    uint64_t shutter_usec,
    const uint8_t* video,
    int bytes,
    bool keyframes,
    RtpCallback callback)
{
    
}


} // namespace janus
