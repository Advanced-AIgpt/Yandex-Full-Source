#include "rtlog.h"

#include <alice/rtlog/client/client.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <util/generic/string.h>

namespace {
    TMaybe<TDuration> ConvertDuration(int durationMillis) {
        if (durationMillis <= 0) {
            return Nothing();
        }
        return TDuration::MilliSeconds(durationMillis);
    }
}

extern "C" {
    struct RTLog_RequestLogger {
        NRTLog::TRequestLoggerPtr LoggerPtr;
    };

    RTLog_Client* RTLog_Client_Create(const char* fileName, const char* serviceName, const RTLog_ClientOptions* options) {
        NRTLog::TClientOptions clientOptions;
        clientOptions.Async = options->Async;
        clientOptions.FlushPeriod = ConvertDuration(options->FlushPeriodMillis);
        clientOptions.FileStatCheckPeriod = ConvertDuration(options->FileStatCheckPeriodMillis);
        auto result = new NRTLog::TClient(TString(fileName), TString(serviceName), clientOptions);
        return (RTLog_Client*)result;
    }

    void RTLog_Client_Destroy(RTLog_Client* client) {
        delete (NRTLog::TClient*)client;
    }

    RTLog_RequestLogger* RTLog_Client_CreateRequestLogger(RTLog_Client* client, const char* token, bool session) {
        auto rtlogClient = (NRTLog::TClient*)client;
        auto loggerPtr = rtlogClient->CreateRequestLogger(TString(token), session);
        auto result = new RTLog_RequestLogger();
        result->LoggerPtr = loggerPtr;
        return result;
    }

    char* RTLog_RequestLogger_LogChildActivationStarted(RTLog_RequestLogger* logger, bool newReqid, const char* description) {
        auto result = logger->LoggerPtr->LogChildActivationStarted(newReqid, TString(description));
        char* chars = new char[result.size() + 1];
        result.strcpy(chars, result.size() + 1);
        return chars;
    }

    void RTLog_RequestLogger_LogChildActivationFinished(RTLog_RequestLogger* logger, const char* token, bool ok) {
        logger->LoggerPtr->LogChildActivationFinished(TString(token), ok);

    }

    void RTLog_RequestLogger_LogEvent(RTLog_RequestLogger* logger, const RTLog_LogEvent* event) {
        NRTLogEvents::LogEvent ev;
        ev.SetSeverity(event->Severity == RTLOG_SEVERITY_INFO
                       ? NRTLogEvents::ESeverity::RTLOG_SEVERITY_INFO
                       : NRTLogEvents::ESeverity::RTLOG_SEVERITY_ERROR);
        ev.SetMessage(TString(event->Message));
        if (event->Backtrace) {
            ev.SetBacktrace(TString(event->Backtrace));
        }
        for (unsigned i = 0; i < event->FieldsCount; i++) {
            RTLog_LogEvent_Field* f = event->Fields + i;
            (*ev.MutableFields())[TString(f->Key)] = TString(f->Value);
        }
        logger->LoggerPtr->LogEvent(ev);
    }

    void RTLog_RequestLogger_Finish(RTLog_RequestLogger* logger) {
        logger->LoggerPtr->Finish();
    }

    void RTLog_RequestLogger_Destroy(RTLog_RequestLogger* logger) {
        delete logger;
    }

    void RTLog_DestroyString(char* str) {
        delete[] str;
    }
}
