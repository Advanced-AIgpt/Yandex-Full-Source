#include "yt_helpers.h"

namespace NMatrix::NNotificator::NAnalytics {

TYtTimestamp EventTimestampToYtTimestamp(TEventTimestamp timestamp) {
    return TInstant::MicroSeconds(timestamp).MicroSeconds();
}

} // namespace NMatrix::NNotificator::NAnalytics
