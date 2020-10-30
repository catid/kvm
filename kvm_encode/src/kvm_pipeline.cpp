// Copyright 2020 Christopher A. Taylor

#include "kvm_pipeline.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Pipeline");


//------------------------------------------------------------------------------
// PipelineNode

void PipelineNode::Initialize(const std::string& name)
{
    Name = name;

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
    QueuePublic.push_back(func);
    Condition.notify_all();
}

void PipelineNode::Loop()
{
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
                LastReportUsec = t1;
                Logger.Info(Name, " stats: ", Count, " frames, avg=",
                    TotalUsec / (float)Count / 1000.f, ", min=",
                    FastestUsec / 1000.f, ", max=", SlowestUsec / 1000.f, " (msec)");
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

    DecoderNode.Initialize("Decoder");
    EncoderNode.Initialize("Encoder");
    AppNode.Initialize("App");

    MmalEncoderSettings settings;
    settings.Kbps = 4000;
    settings.Framerate = 30;
    settings.GopSize = 12;
    Encoder.SetSettings(settings);

    Capture.Initialize([this](const std::shared_ptr<CameraFrame>& buffer)
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
                Logger.Error("Failed to decode JPEG");
                return;
            }

            EncoderNode.Queue([this, frame, frame_number, shutter_usec]()
            {
                int bytes = 0;
                uint8_t* data = Encoder.Encode(frame, false, bytes);
                Decoder.Release(frame);

                // Parse the video into pictures and parameters
                Parser.Reset();
                Parser.ParseVideo(false, data, bytes);

                if (Parser.Pictures.empty()) {
                    Logger.Error("Video output does not include a picture");
                } else if (Parser.Pictures.size() > 1) {
                    Logger.Warn("Video output includes more than one picture");
                }

                if (!Parser.Parameters.empty())
                {
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

                    int video_frame_bytes = picture.TotalBytes;
                    if (picture.Keyframe && VideoParameters) {
                        video_frame_bytes += VideoParameters->size();
                    }

                    auto video_frame = std::make_shared<std::vector<uint8_t>>(video_frame_bytes);
                    uint8_t* dest = video_frame->data();

                    if (picture.Keyframe && VideoParameters) {
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
                }
            });
        });
    });
}

void VideoPipeline::Shutdown()
{
    Terminated = true;

    Capture.Stop();

    Encoder.Shutdown();
    Capture.Shutdown();

    DecoderNode.Shutdown();
    EncoderNode.Shutdown();
    AppNode.Shutdown();
}


} // namespace kvm
