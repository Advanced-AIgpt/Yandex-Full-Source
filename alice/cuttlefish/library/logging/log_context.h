#pragma once

#include "event_log.h"

#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <util/string/builder.h>

namespace NAlice::NCuttlefish {

class TLogContext {
public:
    struct TOptions {
        // Manually defining default constructor as workaround for clang bug (?)
        TOptions() {};

        bool WriteInfoToRtLog = true;
        bool WriteInfoToEventLog = true;
    };

public:
    TLogContext(
        TLogFramePtr logFrame,
        NRTLog::TRequestLoggerPtr rtLog,
        TOptions options = TOptions()
    )
        : LogFrame_(std::move(logFrame))
        , RtLog_(std::move(rtLog))
        , Options_(std::move(options))
    {
        Y_ASSERT(LogFrame_);
        LogDisablingStatusIfHas();
    }

    // Can be copied for guaranty existing (for delayed usage)
    TLogContext(const TLogContext& lc) = default;
    TLogContext& operator=(const TLogContext& lc) = default;

    void LogDisablingStatusIfHas() {
        if (!Options_.WriteInfoToRtLog && RtLog_) {
            SaveRtLogInfoEvent(RtLog_, "InfoLogDisabled");
        }
        if (!Options_.WriteInfoToEventLog && LogFrame_) {
            LogFrame_->LogEvent(NEvClass::InfoLogDisabled());
        }
    }

    TEventLogFrame& Frame() {
        return *LogFrame_;
    }
    const TLogFramePtr& FramePtr() const {
        return LogFrame_;
    }
    const TOptions& Options() const noexcept {
        return Options_;
    }

    template<class T>
    void LogEvent(const T& ev) const {
        LogFrame_->LogEvent(ev);
    }

    template<class T>
    void LogEventInfo(const T& ev) const {
        if (Options_.WriteInfoToEventLog) {
            LogFrame_->LogEvent(ev);
        }
    }

    // WARNING: can be nullptr!
    NRTLog::TRequestLogger* RtLog() const {
        return RtLog_.get();
    }
    NRTLog::TRequestLoggerPtr RtLogPtr() const {
        return RtLog_;
    }
    void ResetRtLogger(const NRTLog::TRequestLoggerPtr& rtLog) {
        RtLog_ = rtLog;
    }

    void TryRtLogInfo(const TString& message) const {
        if (Options_.WriteInfoToRtLog) {
            SaveRtLogInfoEvent(RtLog_, message);
        }
    }

    void TryRtLogError(const TString& message) const {
        SaveRtLogErrorEvent(RtLog_, message);
    }

    template <bool IsInfoLog>
    void TryRtLog(const TString& message) const {
        if constexpr (IsInfoLog) {
            TryRtLogInfo(message);
        } else {
            TryRtLogError(message);
        }
    }

    template <typename T, bool IsInfoLog>
    void LogEventToRtLog(const T& event) const {
        static const auto* descriptor = T::descriptor();

        if (descriptor->field_count() == 0) {
            static const TString message = T::descriptor()->name();
            TryRtLog<IsInfoLog>(message);
        } else {
            static const TString messagePrefix = TStringBuilder() << descriptor->name() << ": ";
            // We have same if in TryRtLogInfo
            // but we need this ugly code to prevent ShortUtf8DebugString from being called when we don't need its result
            if (!IsInfoLog || Options_.WriteInfoToRtLog) {
                TString message = TStringBuilder() << messagePrefix << event.ShortUtf8DebugString();
                TryRtLog<IsInfoLog>(message);
            }
        }
    }

    template <typename T, typename ...TArgs>
    void LogEventInfoCombo(TArgs&& ...args) const {
        const T event(std::forward<TArgs>(args)...);
        LogEventToRtLog<T, true>(event);
        LogEventInfo(event);
    }

    template <class T, typename ...TArgs>
    void LogEventErrorCombo(TArgs&& ...args) const {
        const T event(std::forward<TArgs>(args)...);
        LogEventToRtLog<T, false>(event);
        LogEventInfo(event);
    }

private:
    TLogFramePtr LogFrame_;            // MUST be not nullptr
    NRTLog::TRequestLoggerPtr RtLog_;  // Can contain nullptr
    TOptions Options_;
};

}  // namespace NAlice::NCuttlefish
