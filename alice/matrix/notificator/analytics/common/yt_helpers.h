#pragma once

#include <library/cpp/eventlog/eventlog.h>

#include <util/datetime/base.h>

namespace NMatrix::NNotificator::NAnalytics {

using TYtTimestamp = ui64;

TYtTimestamp EventTimestampToYtTimestamp(TEventTimestamp timestamp);

} // namespace NMatrix::NNotificator::NAnalytics
