// Copyright 2020 Christopher A. Taylor

/*
    MMAL Encoder for Raspberry Pi

    References:
    [1] https://www.itu.int/rec/T-REC-H.264
    [2] https://github.com/raspberrypi/userland/blob/master/interface/mmal/
*/

#pragma once

#include "kvm_core.hpp"
#include "kvm_frame.hpp"

#include <memory>
#include <vector>
#include <mutex>

#include <bcm_host.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_format.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_component_wrapper.h>
#include <interface/mmal/util/mmal_util_params.h>

namespace kvm {


//------------------------------------------------------------------------------
// MmalEncoder

/*
    Kbps: The overall bitrate used by the stream over 1 second.
    Framerate: Input framerate in frames per second.
        We expect the input framerate to be constant.
    GopSize: Interval between keyframes.

    The GopSize for this encoder also controls the size of the keyframes.
    GopSize below 60 tends to produce I-frames that are too large to send
    over Janus WebRTC, as they get to be above 100 KB per frame, and Janus
    will fail to send some frames if they are larger.

    Larger GopSize causes slightly lower quality and longer load times,
    as clients need to wait for the next keyframe to join the video stream.
*/
struct MmalEncoderSettings
{
    int Kbps = 5000; // 4 Mbps
    int Framerate = 30; // From frame source settings
    int GopSize = 60; // Interval between keyframes
};

class MmalEncoder
{
public:
    ~MmalEncoder()
    {
        Shutdown();
    }

    void SetSettings(const MmalEncoderSettings& settings)
    {
        Settings = settings;
    }

    void Shutdown();

    // Pointer is valid until the next Encode() call
    uint8_t* Encode(const std::shared_ptr<Frame>& frame, bool force_keyframe, int& bytes);

protected:
    MmalEncoderSettings Settings;

    MMAL_WRAPPER_T* Encoder = nullptr;
    int Width = 0, Height = 0;

    MMAL_PORT_T* PortIn = nullptr;
    MMAL_PORT_T* PortOut = nullptr;

    std::vector<uint8_t> Data;

    bool Initialize(int width, int height, int input_encoding);
};


} // namespace kvm
