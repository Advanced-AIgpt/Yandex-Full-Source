#include "util.h"

namespace NAlice::NMegamind {

TErrorOr<ELogPriority> MapUniproxyLogLevelToMegamindLogLevel(NAlice::ELogLevel uniproxyLogLevel) {
    using ELogLevel = NAlice::ELogLevel;
    switch (uniproxyLogLevel) {
        case ELogLevel::ELL_DISABLED:
            return ELogPriority::TLOG_EMERG;
        case ELogLevel::ELL_ERROR:
            return ELogPriority::TLOG_INFO;
        case ELogLevel::ELL_WARN:
            return ELogPriority::TLOG_INFO;
        case ELogLevel::ELL_INFO:
            return ELogPriority::TLOG_DEBUG;
        case ELogLevel::ELL_DEBUG:
            return ELogPriority::TLOG_DEBUG;
        case ELogLevel::ELL_ALL:
            return ELogPriority::TLOG_DEBUG;
        default:
            return TError{TError::EType::Parse} << "Can't parse logger level from uniproxy";
    }
}
} // namespace NAlice::NMegamind
