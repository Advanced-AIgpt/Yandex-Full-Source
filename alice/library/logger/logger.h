#pragma once

#include "fwd.h"
#include "log_types.h"

#include <alice/library/logger/proto/config.pb.h>
#include <alice/rtlog/client/client.h>

#include <library/cpp/threading/future/async.h>

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/stream/tempbuf.h>
#include <util/string/builder.h>
#include <util/system/src_location.h>
#include <util/thread/pool.h>

#include <memory>

#if defined(LOG)
#error The LOG must not be defined.
#endif

/*
    We create TRTLogElement if, and only if `severity` is acceptable, so that we do not evaluate extra functions, for example:
    LOG_DEBUG(logger) << TimeConsumingDump();
*/
#define LOG(logger, severity) ((logger).IsSuitable(severity)) && TRTLogElement(logger, severity, __LOCATION__)
#define LOG_INFO(logger) LOG((logger), (TLOG_INFO))
#define LOG_CRIT(logger) LOG((logger), (TLOG_CRIT))
#define LOG_DEBUG(logger) LOG((logger), (TLOG_DEBUG))
#define LOG_ERR(logger) LOG((logger), (TLOG_ERR))
#define LOG_WARN(logger) LOG((logger), (TLOG_WARNING))
#define LOG_EMERG(logger) LOG((logger), (TLOG_EMERG))

/*
    We do LOG if, and only if `type` is acceptable, so that we do not evaluate extra functions, for example:
    LOG_WITH_TYPE(logger, TLOG_INFO, ELogMessageType::MY_TYPE) << TimeConsumingDump();
*/
#define LOG_WITH_TYPE(logger, severity, type) ((logger).IsLogMessageTypeAllowed(type)) && LOG(logger, severity)

#define LOG_ERROR LOG_ERR
#define LOG_WARNING LOG_WARN

#define LOG_WITH_LOC(logger, severity, location) TRTLogElement(logger, severity, location)
#define LOG_CRIT_WITH_LOC(logger, location) LOG_WITH_LOC((logger), (TLOG_CRIT), (location))
#define LOG_DEBUG_WITH_LOC(logger, location) LOG_WITH_LOC((logger), (TLOG_DEBUG), (location))
#define LOG_EMERG_WITH_LOC(logger, location) LOG_WITH_LOC((logger), (TLOG_EMERG), (location))
#define LOG_ERR_WITH_LOC(logger, location) LOG_WITH_LOC((logger), (TLOG_ERR), (location))
#define LOG_INFO_WITH_LOC(logger, location) LOG_WITH_LOC((logger), (TLOG_INFO), (location))
#define LOG_WARN_WITH_LOC(logger, location) LOG_WITH_LOC((logger), (TLOG_WARNING), (location))

namespace NAlice {

struct TLogMessageTag {
    explicit TLogMessageTag(const TString& name)
        : Name_{name}
    {
    }

    const TString Name_;
};

class TRTLogger : private TMoveOnly {
public:
    TRTLogger(IThreadPool& loggingThread, IThreadPool& serializers, NRTLog::TRequestLoggerPtr requestLogger,
              ELogPriority priority, TLog* duplicate);

    TRTLogger(TRTLogger&&) noexcept = default;
    TRTLogger& operator=(TRTLogger&&) noexcept = default;

    virtual ~TRTLogger() {
        if (RequestLogger_) {
            RequestLogger_->Finish();
        }
    }

    template <class T>
    void LogEvent(const T& ev) {
        RequestLogger_->LogEvent(ev);
    }

    void LogEventAsync(THolder<IObjectInQueue> logAction) {
        if (!LoggingThread_.AddAndOwn(std::move(logAction))) {
            Cerr << "Failed to log event asynchronously!" << Endl;
        }
    }

    IThreadPool& GetSerializers() {
        return Serializers_;
    }

    TLog* GetDuplicate() {
        return Duplicate_;
    }

    NRTLog::TRequestLogger* RequestLogger() {
        return RequestLogger_.get();
    }

    const TString& RequestId() const {
        return RequestId_;
    }

    const TString& PureRequestId() const {
        return PureRequestId_;
    }

    i64 HypothesisNumber() const {
        return HypothesisNumber_;
    }

    void UpdateRequestId(TStringBuf reqId, ui32 hypothesisNumber, bool endOfUtterance, TStringBuf appHostNode = {});
    void UpdateRequestId(TStringBuf reqId, TStringBuf appHostNode = {});

    bool IsSuitable(ELogPriority priority) const;

    void SetLogMessagesTypes(TLogMessageTypes types) {
        LogMessageTypes_ = types;
    }

    void AddLogMessagesTypes(TLogMessageTypes types) {
        LogMessageTypes_ |= types;
    }

    bool IsLogMessageTypeAllowed(TLogMessageTypes types) {
        return types & ELogMessageType::All || LogMessageTypes_ & ELogMessageType::All || LogMessageTypes_ & types;
    }

    static TRTLogger& NullLogger();
    // For testing purposes
    static TRTLogger& StderrLogger();

    TRTLogger Copy() const;

    ELogPriority GetLogPriority() const {
        return Priority_;
    }

private:
    IThreadPool& LoggingThread_;
    IThreadPool& Serializers_;

    TString RequestId_;
    TString PureRequestId_;
    NRTLog::TRequestLoggerPtr RequestLogger_;
    ELogPriority Priority_;
    TLog* Duplicate_;
    i64 HypothesisNumber_;
    TLogMessageTypes LogMessageTypes_;
};

TRTLogger CreateNullLogger();

class TRTLogClient : public NNonCopyable::TMoveOnly {
public:
    explicit TRTLogClient(const TRTLog& config);

    void Rotate();

    TRTLogger CreateLogger(TStringBuf token, bool session, TMaybe<ELogPriority> logPriorityFromRequest = Nothing());

private:
    NRTLog::TClient Client_;
    ELogPriority Priority_;
    // This is only for duplicating eventlog.
    // Do not use it for real logging.
    std::unique_ptr<TLog> Duplicate_;

    std::unique_ptr<IThreadPool> LoggingThread_;
    std::unique_ptr<IThreadPool> Serializers_;
};

using TAsyncLogFn = std::function<TString()>;

using TLogFuture = NThreading::TFuture<TString>;
using TLogFutures = TVector<TLogFuture>;

template <typename TFn, typename... TArgs>
TAsyncLogFn LogAsync(TFn fn, TArgs&&... args) {
    static_assert(std::is_same_v<TFunctionResult<TFn>, TString>, "The function should return TString!");
    return [fn, args = std::make_tuple(std::forward<TArgs>(args)...)]() { return std::apply(fn, args); };
}

template <typename T>
TString PrintToString(const T& msg) {
    TStringStream out;
    static_cast<IOutputStream&>(out) << msg;
    return out.Str();
}

template <typename T>
TAsyncLogFn PrintAsync(T&& msg) {
    return LogAsync(PrintToString<T>, std::forward<T>(msg));
}

template <typename T>
TLogFuture MakeTrivialLogFuture(const T& arg) {
    NThreading::TPromise<TString> promise;
    promise.SetValue(std::move(PrintToString(arg)));
    return promise.GetFuture();
}

class TRTLogElement {
public:
    TRTLogElement(TRTLogger& logger, ELogPriority priority, const TSourceLocation& sourceLocation);

    ~TRTLogElement();

    // We need this cast to use logical operators in macros.
    // Read comments at LOG and LOG_WITH_TYPE definitions
    operator bool() const {
        return true;
    }

    bool IsLogEnabled() const;

    TRTLogElement& operator<<(const ELogMessageType type) = delete;

    TRTLogElement& operator<<(const TLogMessageTag& tag) {
        if (tag.Name_.size() < 40 && tag.Name_.find(",") == TString::npos) {
            Tags_.push_back(tag.Name_);
        }
        return *this;
    }

    TRTLogElement& operator<<(const TVector<TLogMessageTag>& tags) {
        for (const auto& tag : tags) {
            *this << tag;
        }
        return *this;
    }

    template <typename T>
    TRTLogElement& operator<<(const T& t) {
        if (IsLogEnabled()) {
            IsBufferEmpty_ = false;
            if (LogFutures_.empty()) {
                static_cast<IOutputStream&>(Buffer_) << t;
            } else {
                LogFutures_.push_back(MakeTrivialLogFuture(t));
            }
        }
        return *this;
    }

    TRTLogElement& operator<<(TAsyncLogFn fn) {
        auto future = NThreading::Async(fn, Logger_.GetSerializers());
        LogFutures_.push_back(std::move(future));
        return *this;
    }

private:
    TTempBufOutput Buffer_;
    TLogFutures LogFutures_;

    TRTLogger& Logger_;
    ELogPriority Priority_;
    const TSourceLocation& SourceLocation_;
    TVector<TString> Tags_;
    TLogMessageTypes LogMessageTypes_ = ELogMessageType::All;
    bool IsBufferEmpty_ = true;
};

} // namespace NAlice
