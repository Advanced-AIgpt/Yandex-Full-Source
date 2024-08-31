#pragma once

#include "logadapter.h"
#include "logger.h"

namespace NAlice {

class TRTLogAdapter final : public TLogAdapter {
public:
    explicit TRTLogAdapter(TRTLogger& logger)
        : Logger{logger}
    {
    }

private:
    void LogImpl(TStringBuf msg, const TSourceLocation& location, ELogAdapterType type) const override {
        switch (type) {
            case ELogAdapterType::CRIT:
                LOG_CRIT_WITH_LOC(Logger, location) << msg;
                break;
            case ELogAdapterType::DEBUG:
                LOG_DEBUG_WITH_LOC(Logger, location) << msg;
                break;
            case ELogAdapterType::EMERGE:
                LOG_EMERG_WITH_LOC(Logger, location) << msg;
                break;
            case ELogAdapterType::ERROR:
                LOG_ERR_WITH_LOC(Logger, location) << msg;
                break;
            case ELogAdapterType::INFO:
                LOG_INFO_WITH_LOC(Logger, location) << msg;
                break;
            case ELogAdapterType::WARNING:
                LOG_WARN_WITH_LOC(Logger, location) << msg;
                break;
        }
    }

    TRTLogger& Logger;
};

} // namespace NAlice
