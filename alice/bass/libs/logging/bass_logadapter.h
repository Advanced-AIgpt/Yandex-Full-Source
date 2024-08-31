#pragma once

#include "logadapter.h"
#include "logger.h"

using namespace NAlice;

namespace NBASS {

class TBassLogAdapter final : public TLogAdapter {
private:
    void LogImpl(TStringBuf msg, const TSourceLocation& location, ELogAdapterType type) const override {
        switch (type) {
            case ELogAdapterType::CRIT:
                LOG_WITH_LOC(CRIT, location) << msg << Endl;
                break;
            case ELogAdapterType::DEBUG:
                LOG_WITH_LOC(DEBUG, location) << msg << Endl;
                break;
            case ELogAdapterType::EMERGE:
                LOG_WITH_LOC(EMERG, location) << msg << Endl;
                break;
            case ELogAdapterType::ERROR:
                LOG_WITH_LOC(ERR, location) << msg << Endl;
                break;
            case ELogAdapterType::INFO:
                LOG_WITH_LOC(INFO, location) << msg << Endl;
                break;
            case ELogAdapterType::WARNING:
                LOG_WITH_LOC(WARNING, location) << msg << Endl;
                break;
        }
    }
};

} // namespace NBASS
