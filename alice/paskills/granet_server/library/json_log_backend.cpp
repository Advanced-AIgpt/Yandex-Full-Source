#include "json_log_backend.h"

#include <library/cpp/logger/record.h>
#include <library/cpp/json/json_writer.h>

#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

namespace {

static TStringBuf ELogPriorityToString(ELogPriority priority) {
    switch (priority) {
        case TLOG_EMERG:
            return TStringBuf("EMERG");
        case TLOG_ALERT:
            return TStringBuf("ALERT");
        case TLOG_CRIT:
            return TStringBuf("CRIT");
        case TLOG_ERR:
            return TStringBuf("ERR");
        case TLOG_WARNING:
            return TStringBuf("WARNING");
        case TLOG_NOTICE:
            return TStringBuf("NOTICE");
        case TLOG_INFO:
            return TStringBuf("INFO");
        case TLOG_DEBUG:
            return TStringBuf("DEBUG");
        case TLOG_RESOURCES:
            return TStringBuf("RESOURCES");
    }
    return TStringBuf();
}

}

TJsonLogBackend::TJsonLogBackend() {
}

void TJsonLogBackend::WriteData(const TLogRecord& rec) {
    NJson::TJsonValue value;
    TInstant now = Now();
    value["timestamp"] = now.ToIsoStringLocal();
    value["level"] = ELogPriorityToString(rec.Priority);
    value["message"] = TStringBuf(rec.Data, rec.Len);

    auto data = NJson::WriteJson(value, false);
    Cout << data << Endl;
}

void TJsonLogBackend::ReopenLog() {
}