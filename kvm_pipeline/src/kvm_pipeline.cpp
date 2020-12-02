// Copyright 2020 Christopher A. Taylor

#include "kvm_pipeline.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Pipeline");


//------------------------------------------------------------------------------
// PipelineNode

void PipelineNode::Initialize(const std::string& name, int max_queue_depth)
{
    Name = name;
    MaxQueueDepth = max_queue_depth;

    Count = 0;
    TotalUsec = 0;
    LastReportUsec = 0;

    Terminated = false;
    Thread = std::make_shared<std::thread>(&PipelineNode::Loop, this);
}

void PipelineNode::Shutdown()
{
    Terminated = true;
    {
        std::unique_lock<std::mutex> locker(Lock);
        Condition.notify_all();
    }
    JoinThread(Thread);

    QueuePrivate.clear();
    QueuePublic.clear();
}

void PipelineNode::Queue(std::function<void()> func)
{
    std::unique_lock<std::mutex> locker(Lock);
    if ((int)QueuePublic.size() >= MaxQueueDepth) {
        Logger.Error(Name, ": Fell too far behind. Dropping incoming frame!");
        return;
    }
    QueuePublic.push_back(func);
    Condition.notify_all();
}

void PipelineNode::Loop()
{
    SetCurrentThreadName(Name.c_str());

    while (!Terminated)
    {
        {
            // unique_lock used since QueueCondition.wait requires it
            std::unique_lock<std::mutex> locker(Lock);

            if (QueuePublic.empty() && !Terminated) {
                Condition.wait(locker);
            }

            std::swap(QueuePublic, QueuePrivate);
        }

        for (auto& func : QueuePrivate)
        {
            if (Terminated) {
                break;
            }

            const uint64_t t0 = GetTimeUsec();

            func();

            const uint64_t t1 = GetTimeUsec();
            const int64_t dt = t1 - t0;

            if (Count == 0) {
                FastestUsec = SlowestUsec = dt;
                LastReportUsec = t1;
            } else if (FastestUsec > dt) {
                FastestUsec = dt;
            } else if (SlowestUsec < dt) {
                SlowestUsec = dt;
            }
            ++Count;
            TotalUsec += dt;

            const int64_t report_interval_usec = 20 * 1000 * 1000;
            if (t1 - LastReportUsec > report_interval_usec) {
                Logger.Info(Name, ": ", Count, " frames, avg=",
                    TotalUsec / (float)Count / 1000.f, ", min=",
                    FastestUsec / 1000.f, ", max=", SlowestUsec / 1000.f, " (msec)");

                Count = 0;
                TotalUsec = 0;
                LastReportUsec = t1;
            }
        }

        QueuePrivate.clear();
    }
}


//------------------------------------------------------------------------------
// VideoPipeline

void VideoPipeline::Initialize(PiplineCallback callback)
{
    Callback = callback;

    Terminated = false;
    Thread = std::make_shared<std::thread>(&VideoPipeline::Loop, this);
}

void VideoPipeline::Loop()
{
    Start();

    uint64_t t0 = GetTimeMsec();

    int consecutive_waits = 0;

    while (!Terminated)
    {
        if (ErrorState || Capture.IsError()) {
            Logger.Info("Capture failure: Stopping pipeline!");
            Stop();

            uint64_t t1 = GetTimeMsec();
            int64_t dt = t1 - t0;

            int64_t back_off_msec = (consecutive_waits + 1) * 1000;

            if (dt < back_off_msec) {
                Logger.Warn("Waiting before retrying... ", back_off_msec / 1000.f, " sec");

                int waits = (back_off_msec - dt + 100 - 1) / 100;
                for (int i = 0; i < waits; ++i) {
                    ThreadSleepForMsec(100);
                    if (Terminated) {
                        break;
                    }
                }

                ++consecutive_waits;
                if (consecutive_waits > 4) {
                    consecutive_waits = 4;
                }
            } else {
                consecutive_waits = 0;
            }

            if (Terminated) {
                break;
            }

            Logger.Info("Capture failure: Restarting pipeline!");
            Start();

            t0 = GetTimeMsec();
        }

        Stats.TryReport();

        ThreadSleepForMsec(100);
    }

    Stop();
}

void VideoPipeline::Shutdown()
{
    Terminated = true;
    JoinThread(Thread);
}

void VideoPipeline::Start()
{
    const int max_queue_depth = 4;
    DecoderNode.Initialize("Decoder", max_queue_depth);
    EncoderNode.Initialize("Encoder", max_queue_depth);
    AppNode.Initialize("App", max_queue_depth);

    // Please see kvm_encode.hpp for comments on these settings
    MmalEncoderSettings settings;
    settings.Kbps = 4000;
    settings.Framerate = 30;
    settings.GopSize = 60; // This affects the keyframe size
    Encoder.SetSettings(settings);

    bool capture_okay = Capture.Initialize([this](const std::shared_ptr<CameraFrame>& buffer)
    {
        DecoderNode.Queue([this, buffer]()
        {
            //Logger.Info("Got frame #", buffer->FrameNumber, " bytes = ", buffer->ImageBytes);

            uint64_t frame_number = buffer->FrameNumber;
            uint64_t shutter_usec = buffer->ShutterUsec;

            if (LastFrameNumber > 0 && frame_number - LastFrameNumber != 1) {
                Logger.Warn("Camera skipped ", frame_number - LastFrameNumber, " frames");
            }

            auto frame = Decoder.Decompress(buffer->Image, buffer->ImageBytes);
            if (!frame) {
                // Note that JPEG decode failures happen a lot when plugged into USB2 ports,
                // so this is not a critical error.
                Logger.Error("Failed to decode JPEG");
                return;
            }

            Stats.AddInput(buffer->ImageBytes);

            EncoderNode.Queue([this, frame, frame_number, shutter_usec]()
            {
                int bytes = 0;
                uint8_t* data = Encoder.Encode(frame, false, bytes);
                if (!data) {
                    Logger.Error("Encoder.Encode failed");
                    ErrorState = true;
                    return;
                }
                if (bytes == 0) {
                    // No image in frame
                    return;
                }
                Decoder.Release(frame);

                Stats.AddVideo(bytes);

                // Parse the video into pictures and parameters
                Parser.Reset();
                Parser.ParseVideo(false, data, bytes);

                if (Parser.Pictures.empty()) {
                    Logger.Error("Video output does not include a picture");
                    return;
                }

                if (Parser.Pictures.size() > 1) {
                    Logger.Warn("Video output includes ", Parser.Pictures.size(), " pictures");
                }

                if (!Parser.Parameters.empty()) {
                    VideoParameters = std::make_shared<std::vector<uint8_t>>(Parser.TotalParameterBytes);

                    uint8_t* dest = VideoParameters->data();
                    for (auto& range : Parser.Parameters) {
                        memcpy(dest, range.Ptr, range.Bytes);
                        dest += range.Bytes;
                    }
                }

                for (auto& picture : Parser.Pictures)
                {
                    //Logger.Info("Picture: slices=", picture.Ranges.size(), " bytes=", picture.TotalBytes);

                    const bool is_keyframe = picture.Keyframe && VideoParameters;

                    int video_frame_bytes = picture.TotalBytes;
                    if (is_keyframe) {
                        video_frame_bytes += VideoParameters->size();
                    }

                    auto video_frame = std::make_shared<std::vector<uint8_t>>(video_frame_bytes);
                    uint8_t* dest = video_frame->data();

                    if (is_keyframe) {
                        memcpy(dest, VideoParameters->data(), VideoParameters->size());
                        dest += VideoParameters->size();
                    }

                    for (auto& range : picture.Ranges) {
                        memcpy(dest, range.Ptr, range.Bytes);
                        dest += range.Bytes;
                    }

                    AppNode.Queue([this, frame_number, shutter_usec, video_frame]() {
                        Callback(frame_number, shutter_usec, video_frame->data(), video_frame->size());
                    });

                    Stats.OnOutputFrame();
                }
            });
        });
    });

    ErrorState = !capture_okay;
}

void VideoPipeline::Stop()
{
    Logger.Info("Stopping capture...");
    Capture.Stop();

    Logger.Info("Stopping encoder...");
    Encoder.Shutdown();

    Logger.Info("Capture shutdown...");
    Capture.Shutdown();

    Logger.Info("DecoderNode shutdown...");
    DecoderNode.Shutdown();

    Logger.Info("EncoderNode shutdown...");
    EncoderNode.Shutdown();

    Logger.Info("AppNode shutdown...");
    AppNode.Shutdown();

    Logger.Info("Video pipeline stopped");
}


//------------------------------------------------------------------------------
// PiplineStatistics

void PiplineStatistics::AddInput(int bytes)
{
    std::lock_guard<std::mutex> locker(Lock);
    InputBytes += bytes;
    InputCount++;
}

void PiplineStatistics::AddVideo(int bytes)
{
    std::lock_guard<std::mutex> locker(Lock);
    VideoBytes += bytes;
    VideoCount++;
    if (MaxVideoBytes == 0 || bytes > MaxVideoBytes) {
        MaxVideoBytes = bytes;
    }
    if (MinVideoBytes == 0 || bytes < MinVideoBytes) {
        MinVideoBytes = bytes;
    }
}

void PiplineStatistics::OnOutputFrame()
{
    std::lock_guard<std::mutex> locker(Lock);
    if (LastOutputFrameUsec == 0) {
        Logger.Info("Success: Got first encoded frame from video pipeline");
    }
    LastOutputFrameUsec = GetTimeUsec();
}

void PiplineStatistics::TryReport()
{
    std::lock_guard<std::mutex> locker(Lock);
    uint64_t now_usec = GetTimeUsec();

    {
        // Warn if more than a second passes with no output video frames
        const int64_t warn_usec = 1000 * 1000;

        int64_t dt = now_usec - LastOutputFrameUsec;
        if (dt > warn_usec) {
            int64_t report_dt = now_usec - LastOutputReportUsec;
            const int64_t report_interval_usec = 2 * 1000 * 1000;
            if (report_dt >= report_interval_usec) {
                if (LastOutputFrameUsec == 0) {
                    Logger.Warn("Warning: No frames produced yet");
                } else {
                    Logger.Warn("Warning: No frames produced in ", (dt / 1000.f), " msec");
                }
                LastOutputReportUsec = now_usec;
            }
        }
    }

    int64_t dt = now_usec - LastReportUsec;

    if (dt >= ReportIntervalUsec) {
        if (LastReportUsec != 0) {
            Report();
        }

        InputBytes = 0;
        InputCount = 0;

        VideoBytes = 0;
        VideoCount = 0;

        MaxVideoBytes = 0;
        MinVideoBytes = 0;

        LastReportUsec = now_usec;
    }
}

void PiplineStatistics::Report()
{
    if (InputCount <= 0 || VideoCount <= 0) {
        Logger.Info("Statistics: ", InputCount, " input frames, ", VideoCount, " video frames");
        return;
    }

    const float avg_input = InputBytes / 1000.f / InputCount;
    const float avg_video = VideoBytes / 1000.f / VideoCount;

    float ratio = 0.f;
    if (avg_video > 0.00001f) {
        ratio = avg_input / avg_video;
    }

    Logger.Info("Statistics: ", InputCount, " input frames [", avg_input, " KB (avg)], ",
        VideoCount, " video frames [avg=", avg_video, " min=", MinVideoBytes/1000.f, " max=", MaxVideoBytes/1000.f, " KB], compression ratio = ", ratio, ":1");
}


} // namespace kvm
