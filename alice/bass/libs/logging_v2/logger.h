#pragma once

#include "log_types.h"

#include <library/cpp/logger/global/global.h>

#include <util/string/builder.h>
#include <util/system/tls.h>

namespace NRTLog {
class TRequestLogger;
}

struct TLogging {

    class TReqInfo {
    public:
        TReqInfo()
            : HypothesisNumber_(-1)
        {
        }

        void Update(TStringBuf reqId, i64 hypothesisNumber = -1) {
            ReqId_ = TString{reqId};
            HypothesisNumber_ = hypothesisNumber;
        }

        // For special ocasions, when we only wat
        void AppendToReqId(TStringBuf reqId) {
            ReqId_ = TStringBuilder() << ReqId_ << TString{reqId};
        }

        const TString& ReqId() const {
            return ReqId_;
        }

        i64 HypothesisNumber() const {
            return HypothesisNumber_;
        }

    private:
        TString ReqId_ = "";
        i64 HypothesisNumber_ = -1;

    };

    static void SetSubReqId(const TString& subReqId);

    static void Configure(const TString& logDir, ui16 port, ELogPriority logLevel, bool enableTSUMTrace = false);
    static void Rotate();
    static Y_THREAD(TReqInfo) ReqInfo;
    static Y_THREAD(TString) BassReqId;
    static Y_THREAD(TString) SubReqId;
    static Y_THREAD(NRTLog::TRequestLogger*) RequestLogger;
    static TLog* CreateLogger(const TString& name);
    static TString SingleLine(TStringBuf lines);
    static TString AsCurl(TStringBuf addr, TStringBuf body);

};

namespace NPrivate {
constexpr TSourceLocation FAKE_LOCATION(TStringBuf("fake"), -1);

class TRTLogElement: public IOutputStream {
public:
    TRTLogElement(ELogPriority priority, const TSourceLocation& sourceLocation, const TStringBuf type, const TLogging::TReqInfo& reqInfo);

    void DoWrite(const void* buf, size_t len) override;

    void DoFlush() override;

    template <class T>
    inline TRTLogElement&& operator<<(T&& t) && {
        static_cast<IOutputStream&>(*this) << std::forward<T>(t);

        return std::move(*this);
    }

private:
    friend inline TRTLogElement&& operator<<(TLogElement& logElement, TRTLogElement&& rtLogElement) {
        rtLogElement.LogElement = &logElement;
        rtLogElement.StartOffset = logElement.Filled();
        return std::move(rtLogElement);
    }

private:
    ELogPriority Priority;
    const TStringBuf LogType;
    const TSourceLocation& SourceLocation;
    TLogElement* LogElement;
    size_t StartOffset;
    const TString& ReqId;
    i64 HypothesisNumber;

    static TString NannyServiceId;
};
}

#define LOG_IMPL(level, level_msg, type, location) \
    (TRTYLogPreprocessor::CheckLoggingContext(TLoggerOperator<TGlobalLog>::Log(), TLogRecordContext(NPrivate::FAKE_LOCATION, {}, TLOG_##level))) && \
        NPrivateGlobalLogger::TEatStream() | \
            (*(TRTYLogPreprocessor::StartRecord(TLoggerOperator<TGlobalLog>::Log(), TLogRecordContext(location, level_msg, TLOG_##level), nullptr)) << '<' << TLogging::ReqInfo.Get().ReqId() << '+' << TLogging::ReqInfo.Get().HypothesisNumber() << TStringBuf("> ") << TLogging::SubReqId.Get() << NPrivate::TRTLogElement(TLOG_##level, location, type, TLogging::ReqInfo.Get()))

#define LOG_TYPE(level, type) LOG_IMPL(level, ToString(TLOG_##level).data(), NBASS::LOG_TYPE_##type, __LOCATION__)

#define LOG(level) LOG_IMPL(level, ToString(TLOG_##level).data(), NBASS::LOG_TYPE_DEFAULT, __LOCATION__)
#define LOG_WITH_LOC(level, custom_location) LOG_IMPL(level, ToString(TLOG_##level).data(), NBASS::LOG_TYPE_DEFAULT, custom_location)

#undef LOG_IF
#define LOG_IF(level, expr) \
    (expr) && LOG(level)

#undef CHECK_WITH_LOG

#define CHECK_WITH_LOG(expr) \
    Y_UNLIKELY(!(expr)) && NPrivateGlobalLogger::TEatStream() | NPrivateGlobalLogger::TVerifyEvent() << __LOCATION__ << TStringBuf(": <") << TLogging::ReqInfo.Get().ReqId() << '+' << TLogging::ReqInfo.Get().HypothesisNumber() << TStringBuf("> ") << #expr << TStringBuf("(verification failed!): ")

#undef FATAL_LOG
#undef ERROR_LOG
#undef WARNING_LOG
#undef NOTICE_LOG
#undef INFO_LOG
#undef DEBUG_LOG

#define VERIFY(expr, message, ret) \
    do { \
        if (Y_UNLIKELY(!(expr))) { \
            Cerr << __LOCATION__ << ": <" << TLogging::ReqInfo.Get().ReqId() << '+' << TLogging::ReqInfo.Get().HypothesisNumber() << TStringBuf("> ") << #expr << TStringBuf(" (verification failed!): ") << (message) << Endl; \
            LOG(ERR) << #expr << TStringBuf(" (verification failed!): ") << (message) << Endl; \
            ret; \
        } \
    } while (false)

#if defined(NDEBUG)
#define DBG LOG_IF(RESOURCES, false)
#else
#define DBG LOG_IMPL(DEBUG, "__DBG__", NBASS::LOG_TYPE_DEFAULT, __LOCATION__)
#endif

class TTSUMTraceLog : public TGlobalLog {
public:
    TTSUMTraceLog() : TGlobalLog("console", TLOG_INFO) {};
};;

template <>
TTSUMTraceLog* CreateDefaultLogger<TTSUMTraceLog>() ;

#define TSUM_TRACE() TLoggerOperator<TTSUMTraceLog>::Log()
