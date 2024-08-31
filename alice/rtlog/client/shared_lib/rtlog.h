#pragma once

#include <memory>
#include <string>
#include <vector>

extern "C" {
    struct RTLog_Client;
    struct RTLog_RequestLogger;

    struct RTLog_ClientOptions {
        bool Async = false;
        int FlushPeriodMillis = -1;
        int FileStatCheckPeriodMillis = -1;
    };

    enum RTLog_Severity {
        RTLOG_SEVERITY_INFO = 1,
        RTLOG_SEVERITY_ERROR = 2
    };

    struct RTLog_LogEvent_Field {
        const char* Key;
        const char* Value;
    };

    struct RTLog_LogEvent {
        RTLog_Severity Severity;
        const char* Backtrace;
        const char* Message;
        RTLog_LogEvent_Field* Fields;
        unsigned FieldsCount;
    };

    RTLog_Client* RTLog_Client_Create(const char* fileName, const char* serviceName, const RTLog_ClientOptions* options);

    void RTLog_Client_Destroy(RTLog_Client* client);

    RTLog_RequestLogger* RTLog_Client_CreateRequestLogger(RTLog_Client* client, const char* token, bool session);

    //caller owns returned bytes, must free it via RTLog_DestroyString
    char* RTLog_RequestLogger_LogChildActivationStarted(RTLog_RequestLogger* logger, bool newReqid, const char* description);

    void RTLog_RequestLogger_LogChildActivationFinished(RTLog_RequestLogger* logger, const char* token, bool ok);

    void RTLog_RequestLogger_LogEvent(RTLog_RequestLogger* logger, const RTLog_LogEvent* event);

    void RTLog_RequestLogger_Finish(RTLog_RequestLogger* logger);

    void RTLog_RequestLogger_Destroy(RTLog_RequestLogger* logger);

    void RTLog_DestroyString(char* str);
}

#if !defined(FROM_IMPL)
namespace NRTLogShared {
    struct TClientOptions {
        bool Async = false;
        int FlushPeriodMillis = -1;
        int FileStatCheckPeriodMillis = -1;
    };

    struct TLogEventField {
        std::string Key;
        std::string Value;
    };

    enum class ESeverity {
        RTLOG_SEVERITY_INFO = 1,
        RTLOG_SEVERITY_ERROR = 2
    };

    struct TLogEvent {
        ESeverity Severity;
        std::string Backtrace;
        std::string Message;
        std::vector<TLogEventField> Fields;
    };

    class TRequestLogger {
    public:
        TRequestLogger(RTLog_RequestLogger* logger)
            : Logger(logger)
        {
        }

        ~TRequestLogger() {
            RTLog_RequestLogger_Destroy(Logger);
        }

        void LogEvent(const TLogEvent& logEvent) {
            RTLog_LogEvent ev;
            ev.Severity = logEvent.Severity == ESeverity::RTLOG_SEVERITY_ERROR
                          ? RTLog_Severity::RTLOG_SEVERITY_ERROR
                          : RTLog_Severity::RTLOG_SEVERITY_INFO;
            ev.Backtrace = logEvent.Backtrace.empty() ? NULL : logEvent.Backtrace.c_str();
            ev.Message = logEvent.Message.c_str();
            std::vector<RTLog_LogEvent_Field> fields(logEvent.Fields.size());
            for (size_t i = 0; i < logEvent.Fields.size(); ++i) {
                fields[i].Key = logEvent.Fields[i].Key.c_str();
                fields[i].Value = logEvent.Fields[i].Value.c_str();
            }
            ev.Fields = fields.data();
            ev.FieldsCount = static_cast<unsigned>(logEvent.Fields.size());
            RTLog_RequestLogger_LogEvent(Logger, &ev);
        }

        std::string LogChildActivationStarted(bool newReqid, const std::string& description) {
            char* token = RTLog_RequestLogger_LogChildActivationStarted(Logger, newReqid, description.c_str());
            std::string result(token);
            RTLog_DestroyString(token);
            return result;
        }

        void LogChildActivationFinished(const std::string& token, bool ok) {
            RTLog_RequestLogger_LogChildActivationFinished(Logger, token.c_str(), ok);
        }

        void Finish() {
            RTLog_RequestLogger_Finish(Logger);
        }

    private:
        RTLog_RequestLogger* Logger;
    };

    class TClient {
    public:
        TClient(const std::string& fileName, const std::string& serviceName, const TClientOptions& options = {})
            : Client(CreateClient(fileName, serviceName, options))
        {
        }

        ~TClient() {
            RTLog_Client_Destroy(Client);
        }

        std::shared_ptr<TRequestLogger> CreateRequestLogger(const std::string& token = "", bool session = false) {
            RTLog_RequestLogger* logger = RTLog_Client_CreateRequestLogger(Client, token.c_str(), session);
            return std::make_shared<TRequestLogger>(logger);
        }

    private:
        inline RTLog_Client* CreateClient(const std::string& fileName, const std::string& serviceName, const TClientOptions& options) {
            RTLog_ClientOptions clientOptions;
            clientOptions.Async = options.Async;
            clientOptions.FlushPeriodMillis = options.FlushPeriodMillis;
            clientOptions.FileStatCheckPeriodMillis = options.FileStatCheckPeriodMillis;
            return RTLog_Client_Create(fileName.c_str(), serviceName.c_str(), &clientOptions);
        }

    private:
        RTLog_Client* Client;
    };
}
#endif
