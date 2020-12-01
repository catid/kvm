// Copyright 2020 Christopher A. Taylor

/*
    FIXME
*/

#pragma once

#include "kvm_core.hpp"

namespace kvm {


//------------------------------------------------------------------------------
// MmalEncoder

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
