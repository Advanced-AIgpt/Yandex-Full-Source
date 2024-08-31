#include "logger.h"

#include "logger_utils.h"

#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/logger/log.h>

#include <util/datetime/base.h>
#include <util/generic/singleton.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/system/env.h>

namespace NAlice {
namespace {

const TString NANNY_SERVICE_ID = GetEnv("NANNY_SERVICE_ID");

ELogPriority ConstructLogPriority(ui32 level) {
    ELogPriority priority = LOG_MAX_PRIORITY;
    if (level <= static_cast<ui32>(LOG_MAX_PRIORITY)) {
        priority = static_cast<ELogPriority>(level);
    }
    return priority;
}

NRTLog::TClient ConstructRTLogClient(const TRTLog& config) {
    NRTLog::TClientOptions options{
        config.GetAsync(),
        TDuration::Seconds(config.GetFlushPeriodSecs()),
        TDuration::Seconds(config.GetFileStatCheckPeriodSecs())
    };
    if (config.GetUseNewCompression()) {
        return NRTLog::TClient{config.GetFilename(), config.GetServiceName(), options, COMPRESSED_LOG_FORMAT_V5, config.GetUnifiedAgentUri(), config.GetUnifiedAgentLogFile()};
    }
    return NRTLog::TClient{config.GetFilename(), config.GetServiceName(), options, COMPRESSED_LOG_FORMAT_V4, config.GetUnifiedAgentUri(), config.GetUnifiedAgentLogFile()};
}

std::unique_ptr<TLog> ConstructDuplicateLogger(const TRTLog& config, ELogPriority priority) {
    if (!config.HasCopyOutputTo()) {
        return {};
    }

    const TString type{config.GetCopyOutputTo()};
    for (const TStringBuf predefined : {TStringBuf("cout"), TStringBuf("cerr")}) {
        if (type == predefined) {
            return std::make_unique<TLog>(CreateLogBackend(type, priority, true/* threaded */));
        }
    }

    return {};
}

struct TLogEventInfo {
    const TSourceLocation& SourceLocation;
    TStringBuf Message;
    const TVector<TString>& Tags;
};

void LogString(TRTLogger& logger, ELogPriority priority, const TLogEventInfo& logEventInfo) {
    // Logic of enabling/disabling logging should be not stricter than in TRTLogElement::IsLogEnabled
    try {
        const bool isError = IsIn({TLOG_EMERG, TLOG_ALERT, TLOG_CRIT, TLOG_ERR}, priority);

        if (logger.IsSuitable(priority)) {
            NRTLogEvents::TLogEvent ev;

            ev.SetMessage(TString{logEventInfo.Message});
            ev.SetSeverity(isError ? NRTLogEvents::ESeverity::RTLOG_SEVERITY_ERROR
                                   : NRTLogEvents::ESeverity::RTLOG_SEVERITY_INFO);
            (*ev.MutableFields())["pos"] = TStringBuilder() << logEventInfo.SourceLocation.File << ':'
                                                            << logEventInfo.SourceLocation.Line;
            (*ev.MutableFields())["request_id"] = logger.PureRequestId(); // alice client request id (x-alice-client-reqid)
            (*ev.MutableFields())["hypothesis_number"] = ToString(logger.HypothesisNumber());
            (*ev.MutableFields())["nanny_service_id"] = NANNY_SERVICE_ID;

            if (!logEventInfo.Tags.empty()) {
                (*ev.MutableFields())["tags"] = JoinSeq(',', logEventInfo.Tags);
            }
            logger.LogEvent(ev);
        }

        if (auto* out = logger.GetDuplicate()) {
            NLogging::LogLine(*out, priority, logger.RequestId(), logEventInfo.SourceLocation, logEventInfo.Message);
        }
    } catch (...) {
        Cerr << "Exception during log output: " << CurrentExceptionMessage() << Endl;
    }
}

class TAsyncLogAction final : public IObjectInQueue {
public:
    TAsyncLogAction(TTempBufOutput&& buffer, TLogFutures logFutures, TRTLogger&& logger, ELogPriority priority,
                    TSourceLocation sourceLocation, TVector<TString>&& tags)
        : Buffer{std::move(buffer)}
        , Priority{priority}
        , SourceLocation{sourceLocation}
        , Logger{std::move(logger)}
        , Futures{std::move(logFutures)}
        , Tags{std::move(tags)}
    {
    }

    void Process(void*) override {
        TStringBuilder out;
        out << TStringBuf{Buffer.Data(), Buffer.Filled()};
        std::for_each(Futures.begin(), Futures.end(), [&](const auto& future) { out << future.GetValueSync(); });
        LogString(Logger, Priority, {SourceLocation, out, Tags});
    }

private:
    TTempBufOutput Buffer;
    ELogPriority Priority;
    TSourceLocation SourceLocation;
    TRTLogger Logger;
    TLogFutures Futures;
    TVector<TString> Tags;
};

TStringBuf AppHostPrefix(TStringBuf appHostNode) {
    return appHostNode.Empty() ? TStringBuf{} : TStringBuf("@");
}

class TNullLogger : public TRTLogger {
public:
    TNullLogger()
        : TRTLogger{LoggingThread_, Serializer_, Client_.CreateRequestLogger(), ELogPriority::TLOG_EMERG,
                    &OutputLog_}
    {
    }

private:
    static NRTLog::TClient Client_;
    static TFakeThreadPool LoggingThread_;
    static TFakeThreadPool Serializer_;
    static TLog OutputLog_;
};

NRTLog::TClient TNullLogger::Client_ = NRTLog::TClient{"/dev/null", "null logger"};
TFakeThreadPool TNullLogger::LoggingThread_ = TFakeThreadPool{};
TFakeThreadPool TNullLogger::Serializer_ = TFakeThreadPool{};
TLog TNullLogger::OutputLog_ = TLog{MakeHolder<TStreamLogBackend>(&Cerr)};

} // namespace

// TRTLogger ------------------------------------------------------------------
TRTLogger::TRTLogger(IThreadPool& loggingThread, IThreadPool& serializers, NRTLog::TRequestLoggerPtr requestLogger,
                     ELogPriority priority, TLog* duplicate)
    : LoggingThread_{loggingThread}
    , Serializers_{serializers}
    , RequestLogger_{requestLogger}
    , Priority_{priority}
    , Duplicate_{duplicate}
    , HypothesisNumber_{-1}
{
}

void TRTLogger::UpdateRequestId(TStringBuf reqId, ui32 hypothesisNumber, bool endOfUtterance, TStringBuf appHostNode) {
    RequestId_ = TStringBuilder{} << " <" << reqId << '+' <<  hypothesisNumber << '+' << endOfUtterance
                                  << AppHostPrefix(appHostNode) << appHostNode << '>';
    PureRequestId_ = TString{reqId};
    HypothesisNumber_ = hypothesisNumber;
}

void TRTLogger::UpdateRequestId(TStringBuf reqId, TStringBuf appHostNode) {
    RequestId_ = TStringBuilder{} << " <" << reqId << AppHostPrefix(appHostNode) << appHostNode << '>';;
    PureRequestId_ = TString{reqId};
}

bool TRTLogger::IsSuitable(ELogPriority priority) const {
    return priority <= Priority_;
}

TRTLogger& TRTLogger::NullLogger() {
    class TContainer {
    public:
        TContainer()
            : Client_{"/dev/null", "null logger"}
            , Logger_{LoggingThread_, Serializer_, Client_.CreateRequestLogger(), ELogPriority::TLOG_EMERG,
                      nullptr /* duplicate */}
        {
        }

        TRTLogger& Logger() {
            return Logger_;
        }

    private:
        NRTLog::TClient Client_;
        TFakeThreadPool LoggingThread_;
        TFakeThreadPool Serializer_;
        TRTLogger Logger_;
    };

    return Singleton<TContainer>()->Logger();
}

TRTLogger& TRTLogger::StderrLogger() {
    class TContainer {
    public:
        TContainer()
            : Client_{"/dev/null", "null logger"}
            , OutputLog_{MakeHolder<TStreamLogBackend>(&Cerr)}
            , Logger_{LoggingThread_, Serializer_, Client_.CreateRequestLogger(), ELogPriority::TLOG_EMERG,
                      &OutputLog_}
        {
        }

        TRTLogger& Logger() {
            return Logger_;
        }

    private:
        NRTLog::TClient Client_;
        TLog OutputLog_;
        TFakeThreadPool LoggingThread_;
        TFakeThreadPool Serializer_;
        TRTLogger Logger_;
    };

    return Singleton<TContainer>()->Logger();
}

TRTLogger TRTLogger::Copy() const {
    TRTLogger logger{LoggingThread_, Serializers_, RequestLogger_, Priority_, Duplicate_};
    logger.SetLogMessagesTypes(LogMessageTypes_);
    logger.RequestId_ = RequestId_;
    logger.PureRequestId_ = PureRequestId_;
    logger.HypothesisNumber_ = HypothesisNumber_;
    return logger;
}

TRTLogger CreateNullLogger() {
    return TNullLogger{};
}

// TRTLogClient ---------------------------------------------------------------
TRTLogClient::TRTLogClient(const TRTLog& config)
    : Client_{ConstructRTLogClient(config)}
    , Priority_{ConstructLogPriority(config.GetLevel())}
    , Duplicate_{ConstructDuplicateLogger(config, Priority_)}
{
    if (config.GetShouldUseAsyncSerialization()) {
        LoggingThread_ = std::make_unique<TThreadPool>(TThreadPoolParams{"LoggingThread"});
        LoggingThread_->Start(/* threadCount= */ 1);
        Serializers_ = std::make_unique<TThreadPool>(TThreadPoolParams{"SerializeThread"});
        Serializers_->Start(config.GetSerializerThreadsCount());
    } else {
        LoggingThread_ = std::make_unique<TFakeThreadPool>();
        Serializers_ = std::make_unique<TFakeThreadPool>();
    }
}

TRTLogger TRTLogClient::CreateLogger(TStringBuf token, bool session, TMaybe<ELogPriority> logPriorityFromRequest) {
    auto chosenPriority = logPriorityFromRequest ? *logPriorityFromRequest : Priority_;
    return TRTLogger{*LoggingThread_, *Serializers_, Client_.CreateRequestLogger(token, session), chosenPriority,
                     Duplicate_.get()};
}

void TRTLogClient::Rotate() {
    Client_.Reopen();
}

// TRTLogElement --------------------------------------------------------------
TRTLogElement::TRTLogElement(TRTLogger& logger,
                             ELogPriority priority,
                             const TSourceLocation& sourceLocation)
    : Logger_{logger}
    , Priority_{priority}
    , SourceLocation_{sourceLocation}
{
}

TRTLogElement::~TRTLogElement() {
    if (IsBufferEmpty_) {
        return;
    }
    if (LogFutures_.empty()) {
        LogString(Logger_, Priority_, {SourceLocation_, TStringBuf{Buffer_.Data(), Buffer_.Filled()}, Tags_});
    } else {
        Logger_.LogEventAsync(MakeHolder<TAsyncLogAction>(std::move(Buffer_), std::move(LogFutures_), std::move(Logger_.Copy()),
                                                          Priority_, SourceLocation_, std::move(Tags_)));
    }
}

bool TRTLogElement::IsLogEnabled() const {
    if (!Logger_.IsLogMessageTypeAllowed(LogMessageTypes_)) {
        return false;
    }
    if (Logger_.GetDuplicate()) {
        return true;
    }
    if (!Logger_.IsSuitable(Priority_)) {
        return false;
    }
    return true;
}

} // namespace NAlice
