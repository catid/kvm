// Copyright 2020 Christopher A. Taylor

/*
    Video Pipeline:
    Camera -> JPEG Decode -> H.264 Encode -> Video Parser
*/

#pragma once

#include "kvm_core.hpp"
#include "kvm_capture.hpp"
#include "kvm_turbojpeg.hpp"
#include "kvm_encode.hpp"
#include "kvm_video.hpp"

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace kvm {


//------------------------------------------------------------------------------
// ParsedVideo

struct ParsedVideo
{
    std::vector<uint8_t> Data;
};


//------------------------------------------------------------------------------
// PipelineNode

class PipelineNode
{
public:
    ~PipelineNode()
    {
        Shutdown();
    }
    void Initialize(const std::string& name);
    void Shutdown();

    bool IsTerminated() const
    {
        return Terminated;
    }

    void Queue(std::function<void()> func);

protected:
    std::string Name;

    int Count = 0;
    int64_t TotalUsec = 0;
    int64_t FastestUsec = 0;
    int64_t SlowestUsec = 0;

    uint64_t LastReportUsec = 0;

    mutable std::mutex Lock;
    std::condition_variable Condition;
    std::vector<std::function<void()>> QueuePublic;

    std::vector<std::function<void()>> QueuePrivate;

    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;

    void Loop();
};


//------------------------------------------------------------------------------
// VideoPipeline

using PiplineCallback = std::function<void(
    uint64_t frame_number,
    uint64_t shutter_usec,
    const uint8_t* data,
    int bytes,
    bool keyframes)>;

class VideoPipeline
{
public:
    ~VideoPipeline()
    {
        Shutdown();
    }
    void Initialize(PiplineCallback callback);
    void Shutdown();

    bool IsTerminated()
    {
        return Capture.IsError() ||
            Terminated ||
            EncoderNode.IsTerminated() ||
            DecoderNode.IsTerminated();
    }

protected:
    PiplineCallback Callback;

    V4L2Capture Capture;
    uint64_t LastFrameNumber = 0;

    PipelineNode DecoderNode;
    TurboJpegDecoder Decoder;

    PipelineNode EncoderNode;
    MmalEncoder Encoder;

    VideoParser Parser;

    std::shared_ptr<std::vector<uint8_t>> VideoParameters;

    PipelineNode AppNode;

    std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
};


} // namespace kvm
