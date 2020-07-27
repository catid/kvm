// Copyright 2020 Christopher A. Taylor

/*
    MMAL Encoder for Raspberry Pi
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

class MmalEncoder
{
public:
    ~MmalEncoder()
    {
        Shutdown();
    }

    bool Initialize(int width, int height, int kbps = 4000, int fps = 30, int gop = 6);
    void Shutdown();

    uint8_t* Encode(const std::shared_ptr<Frame>& frame, int& bytes);

protected:
    MMAL_WRAPPER_T* Encoder = nullptr;
    int Width = 0, Height = 0;

    MMAL_PORT_T* PortIn = nullptr;
    MMAL_PORT_T* PortOut = nullptr;

    std::vector<uint8_t> Data;
};


} // namespace kvm
