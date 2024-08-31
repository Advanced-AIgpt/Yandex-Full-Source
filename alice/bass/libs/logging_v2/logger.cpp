#include "logger.h"

#include <alice/rtlog/client/client.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/logger/file.h>
#include <library/cpp/logger/filter.h>
#include <library/cpp/logger/stream.h>
#include <library/cpp/logger/thread.h>

#include <library/cpp/http/io/stream.h>

#include <util/string/subst.h>
#include <util/stream/mem.h>
#include <util/system/env.h>
#include <util/system/execpath.h>
#include <util/system/mutex.h>
#include <util/system/shellcommand.h>

Y_THREAD(TLogging::TReqInfo) TLogging::ReqInfo;

Y_THREAD(TString) TLogging::BassReqId;
Y_THREAD(TString) TLogging::SubReqId;
Y_THREAD(NRTLog::TRequestLogger*) TLogging::RequestLogger;

void TLogging::SetSubReqId(const TString& subReqId) {
    if (subReqId) {
        SubReqId.Get() = TStringBuilder() << '<' << subReqId << TStringBuf("> ");
    } else {
        SubReqId.Get().clear();
    }
}

namespace {

struct TLoggingContext {
    TString Path = "console";
    TString BinaryName = "test";
    ui16 Port = 0;

    struct TStorage {
        TMutex Lock;
        TVector<std::unique_ptr<TLog>> Loggers;
    };
    std::unique_ptr<TStorage> Storage = std::make_unique<TStorage>();

    void Add(TLog* logger) {
        auto g = Guard(Storage->Lock);
        Storage->Loggers.emplace_back(logger);
    }

    void RotateLogs() {
        auto g = Guard(Storage->Lock);
        for (size_t i = 0; i < Storage->Loggers.size(); ++i) {
            Storage->Loggers[i]->ReopenLog();
        }
    }
};

THolder<TThreadedLogBackend> CreateThreadedLogBackendFilename(const TString& name) {
    TLoggingContext* ctx = Singleton<TLoggingContext>();
    TFsPath logFile = TFsPath(ctx->Path) / name;
    if (!logFile.Exists()) {
        logFile = logFile.Parent().RealPath() / logFile.Basename();
    } else {
        logFile = logFile.RealPath();
    }
    return MakeHolder<TOwningThreadedLogBackend>(new TFileLogBackend(logFile.GetPath()));
}

THolder<TThreadedLogBackend> CreateThreadedLogBackend(const char* name) {
    TLoggingContext* ctx = Singleton<TLoggingContext>();
    TString filename = Sprintf("current-%s_%s-%s-%d", (ctx->BinaryName).data(), name, (ctx->BinaryName).data(), ctx->Port);
    return CreateThreadedLogBackendFilename(filename);
}


THolder<TLogBackend> CreateTSUMTraceLogBckend(bool enable) {
    if (enable) {
        TLoggingContext* ctx = Singleton<TLoggingContext>();
        TString tsum_trace_filename = Sprintf("current-%s-%d-tsum-trace.log", (ctx->BinaryName).data(), ctx->Port);
        return CreateThreadedLogBackendFilename(tsum_trace_filename);
    } else {
        return MakeHolder<TNullLogBackend>();
    }
}

}

namespace NPrivate {
TRTLogElement::TRTLogElement(ELogPriority priority, const TSourceLocation& sourceLocation, TStringBuf logType, const TLogging::TReqInfo& reqInfo)
    : Priority(priority)
    , LogType(logType)
    , SourceLocation(sourceLocation)
    , LogElement(nullptr)
    , StartOffset(0)
    , ReqId(reqInfo.ReqId())
    , HypothesisNumber(reqInfo.HypothesisNumber())
{
}

void TRTLogElement::DoWrite(const void* buf, size_t len) {
    LogElement->Write(buf, len);
}

void TRTLogElement::DoFlush() {
    if (TLogging::RequestLogger) {
        auto ev = NRTLogEvents::TLogEvent();
        auto filled = LogElement->Filled();
        if (filled > StartOffset && LogElement->Data()[filled - 1] == '\n') {
            --filled;
        }
        if (filled > StartOffset) {
            ev.SetMessage(LogElement->Data() + StartOffset, filled - StartOffset);
            const auto isError = Priority == TLOG_EMERG || Priority == TLOG_ALERT ||
                                 Priority == TLOG_CRIT || Priority == TLOG_ERR;
            ev.SetSeverity(isError ? NRTLogEvents::ESeverity::RTLOG_SEVERITY_ERROR : NRTLogEvents::ESeverity::RTLOG_SEVERITY_INFO);
            (*ev.MutableFields())["pos"] = Sprintf("%s:%d", SourceLocation.File.data(), SourceLocation.Line);
            (*ev.MutableFields())["bass_log_type"] = LogType;
            (*ev.MutableFields())["request_id"] = ReqId;
            (*ev.MutableFields())["hypothesis_number"] = ToString(HypothesisNumber);
            (*ev.MutableFields())["nanny_service_id"] = NannyServiceId;
            TLogging::RequestLogger->LogEvent(ev);
        }
    }
    LogElement->Flush();
    StartOffset = LogElement->Filled();
}

TString TRTLogElement::NannyServiceId = GetEnv("NANNY_SERVICE_ID");

}

// static
TString TLogging::SingleLine(TStringBuf lines) {
    TString result(lines);
    SubstGlobal(result, "\r", "\\r");
    SubstGlobal(result, "\n", "\\n");
    return result;
}

//static
TString TLogging::AsCurl(TStringBuf addr, TStringBuf data) {
    TStringBuilder result;

    TStringBuf scheme;
    addr.NextTok("://", scheme);

    TString type;
    size_t pos = data.find_first_of(' ');
    if (pos != data.npos) {
        type = data.substr(0, pos);
    }
    else {
        type = TStringBuf("GET");
    }

    result << TStringBuf("curl -X") << type;

    if (data) {
        /* TODO
        TMemoryInput src(data);
        THttpInput httpInput(&src);
        TString aaa;
        ShellQuoteArg(aaa, httpInput.ReadAll());
        */
    }

    result << TStringBuf(" '") << (scheme.EndsWith("s") ? TStringBuf("https") : (TStringBuf("http"))) << TStringBuf("://") << addr << '\'';

    return result;
}

// static
void TLogging::Configure(const TString& logDir, ui16 port, ELogPriority logLevel, bool enableTSUMTrace) {
    const TString& execPath = GetExecPath();
    TString basename = execPath.substr(execPath.find_last_of("/\\") + 1);

    TLoggingContext* ctx = Singleton<TLoggingContext>();
    ctx->Path = logDir;
    ctx->BinaryName = basename;
    ctx->Port = port;

    THolder<TLogBackend> backend;
    THolder<TLogBackend> tsumBackend;
    if (logDir != TStringBuf("console")) {
        backend = MakeHolder<TFilteredLogBackend>(CreateThreadedLogBackend("main"), logLevel);
        tsumBackend = CreateTSUMTraceLogBckend(enableTSUMTrace);
    } else {
        backend = MakeHolder<TFilteredLogBackend>(MakeHolder<TStreamLogBackend>(&Cerr), logLevel); // not threaded
        tsumBackend = CreateTSUMTraceLogBckend(false);
    }

    TLoggerOperator<TGlobalLog>::Log().ResetBackend(std::move(backend));
    TLoggerOperator<TTSUMTraceLog>::Log().ResetBackend(std::move(tsumBackend));
}

// static
void TLogging::Rotate() {
    TLoggerOperator<TGlobalLog>::Log().ReopenLog();
    TLoggerOperator<TTSUMTraceLog>::Log().ReopenLog();
    Singleton<TLoggingContext>()->RotateLogs();
}

// static
TLog* TLogging::CreateLogger(const TString& name) {
    TLoggingContext* ctx = Singleton<TLoggingContext>();
    TLog* log;
    if (ctx->Path != TStringBuf("console")) {
        log = new TLog(THolder<TLogBackend>(CreateThreadedLogBackend(name.data())));
    } else {
        log = new TLog(MakeHolder<TStreamLogBackend>(&Cerr));
    }
    ctx->Add(log);
    return log;
}

template <>
TTSUMTraceLog* CreateDefaultLogger<TTSUMTraceLog>() {
    return new TTSUMTraceLog();
}
