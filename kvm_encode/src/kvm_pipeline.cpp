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
// Video Pipeline

void Pipeline::Initialize(PiplineCallback callback)
{
    Callback = callback;

    DecoderNode.Initialize("Decoder");
    EncoderNode.Initialize("Encoder");

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

            auto frame = Decoder.Decompress(buffer->Image, buffer->ImageBytes);
            if (!frame) {
                Logger.Error("Failed to decode JPEG");
                return;
            }

            EncoderNode.Queue([this, frame]()
            {
                int bytes = 0;
                uint8_t* data = Encoder.Encode(frame, false, bytes);

                Decoder.Release(frame);

                Logger.Info("FIXME: Parse encoder output here");
            });
        });
    });
}

void Pipeline::Shutdown()
{
    Terminated = true;

    Capture.Stop();

    Encoder.Shutdown();
    Capture.Shutdown();

    DecoderNode.Shutdown();
    EncoderNode.Shutdown();
}


} // namespace kvm
