#include "splitter.h"

#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>

namespace NAlice::NSplitter {

namespace {

const TString SYNTHETIC_GUID_MARKER = "deadbeef";

template<class T>
typename std::enable_if_t<std::is_base_of_v<google::protobuf::Message, T>, TMaybe<const T*>>
TryGet(const TEvent* event) {
    if (event->GetProto()->GetDescriptor()->name() == T::descriptor()->name()) {
        return event->Get<T>();
    }
    return Nothing();
}

struct TContext {
    TFsPath SavePath;
    TVector<TFsPath> LogPaths;
};

class TEventProcessor final : public IEventProcessor {
public:
    explicit TEventProcessor(const TFsPath& savePath, TString alias)
        : SavePath_{savePath}
        , Alias_{std::move(alias)} // megamind/bass/vins
    {
    }

    void ProcessEvent(const TEvent* event) override {
        if (const auto activationStarted = TryGet<NRTLogEvents::ActivationStarted>(event)) {
            ProcessActivationStarted(event->FrameId, *activationStarted.GetRef());
        } else if (const auto logEvent = TryGet<NRTLogEvents::LogEvent>(event)) {
            ProcessLogEvent(event->FrameId, *logEvent.GetRef());
        }
    }

    void PostProcess() {
        for (auto&& framePair : FrameMap_) {
            DroppedFrames_.push_back(std::move(framePair.second));
        }

        // collect logs for all reqIds from all frames
        THashMap<TString, TVector<TString>> reqidLogs;
        for (const auto& frame : DroppedFrames_) {
            auto& logs = reqidLogs[frame->ReqId];
            for (auto&& log : frame->LogMessages) {
                logs.push_back(std::move(log));
            }
        }

        // flush logs reqId by reqId
        for (auto& reqidLogsPair : reqidLogs) {
            const auto& reqid = reqidLogsPair.first;
            if (reqid.StartsWith(SYNTHETIC_GUID_MARKER)) {
                // Example: "save/deadbeef-01234567-01234567-01234567/megamind.txt"
                TFsPath path = SavePath_ / reqid / (Alias_ + ".txt");
                FlushLogs(path, std::move(reqidLogsPair.second));
            }
        }
    }

private:
    // prevent copying large data
    struct TFrame : NNonCopyable::TNonCopyable {
        TVector<TString> LogMessages;
        TString ReqId;
    };

    void ProcessActivationStarted(ui32 frameId, NRTLogEvents::ActivationStarted activationStarted) {
        THolder<TFrame> newFrame = MakeHolder<TFrame>();
        newFrame->ReqId = activationStarted.reqid();

        auto* framePtr = FrameMap_.FindPtr(frameId);
        if (framePtr) {
            DroppedFrames_.push_back(std::move(*framePtr));
            *framePtr = std::move(newFrame);
        } else {
            FrameMap_.emplace(frameId, std::move(newFrame));
        }
    }

    void ProcessLogEvent(ui32 frameId, NRTLogEvents::LogEvent logEvent) {
        auto& frame = FrameMap_[frameId];
        frame->LogMessages.push_back(logEvent.message());
    }

    void FlushLogs(TFsPath path, TVector<TString> logs) {
        if (!path.Parent().Exists()) {
            path.Parent().MkDirs();
        }

        try {
            TFile file(path, CreateAlways | WrOnly);
            for (const auto& log : logs) {
                file.Write(log.data(), log.Size());
                file.Write("\n", 1 /* len */);
            }
            file.Close();
            INFO_LOG << "Saved logs to " << path.GetPath() << Endl;
        } catch (...) {
            ERROR_LOG << "Unsuccessful flushing logs to " << path.GetPath() << Endl;
        }
    }

private:
    const TFsPath& SavePath_;
    const TString Alias_;
    THashMap<ui32, THolder<TFrame>> FrameMap_; // <frame id, frame>
    TVector<THolder<TFrame>> DroppedFrames_; // Фреймы могут перезатираться (в Vins), поэтому храним историю выкинутых из FrameMap_ объектов
};

} // namespace

TSplitter::TSplitter(TFsPath savePath, TVector<TFsPath> logPaths)
    : SavePath_{std::move(savePath)}
    , LogPaths_{std::move(logPaths)}
{
}

void TSplitter::Split() {
    for (const auto& logPath : LogPaths_) {
        TEventProcessor proc(SavePath_, logPath.GetName());

        const char* argv[] = {
            "",
            logPath.GetPath().data()
        };
        IterateEventLog(NEvClass::Factory(), &proc, Y_ARRAY_SIZE(argv), argv);

        proc.PostProcess();
    }
}

} // namespace NAlice::NSplitter
