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
    void Initialize(const std::string& name, int max_queue_depth);
    void Shutdown();

    bool IsTerminated() const
    {
        return Terminated;
    }

    void Queue(std::function<void()> func);

protected:
    std::string Name;
    int MaxQueueDepth = 0;

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
// PiplineStatistics

/*
    Report statistics on how well we are compressing the input data
*/
class PiplineStatistics
{
public:
    void AddInput(int bytes);
    void AddVideo(int bytes);
    void OnOutputFrame();

    void TryReport();

private:
    std::mutex Lock;

    static const int64_t ReportIntervalUsec = 20 * 1000 * 1000; // 20 seconds

    uint64_t LastReportUsec = 0;

    int InputBytes = 0;
    int InputCount = 0;

    int VideoBytes = 0;
    int VideoCount = 0;

    int MaxVideoBytes = 0;
    int MinVideoBytes = 0;

    uint64_t LastOutputFrameUsec = 0;
    uint64_t LastOutputReportUsec = 0;

    void Report();
};


//------------------------------------------------------------------------------
// VideoPipeline

using PiplineCallback = std::function<void(
    uint64_t frame_number,
    uint64_t shutter_usec,
    const uint8_t* data,
    int bytes)>;

class VideoPipeline
{
public:
    ~VideoPipeline()
    {
        Shutdown();
    }
    void Initialize(PiplineCallback callback);
    void Shutdown();

    bool IsTerminated() const
    {
        return Terminated;
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
    std::atomic<bool> ErrorState = ATOMIC_VAR_INIT(false);
    std::shared_ptr<std::thread> Thread;

    PiplineStatistics Stats;

    void Start();
    void Stop();
    void Loop();
};


} // namespace kvm
