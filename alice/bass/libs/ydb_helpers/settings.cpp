#include "settings.h"

using namespace NYdb::NTable;

namespace NYdbHelpers {

namespace {

TRetryOperationSettings MakeDefaultYdbRetrySettings() {
    TRetryOperationSettings settings;
    settings.MaxRetries(3)
            .SlowBackoffSettings(TRetryOperationSettings::DefaultSlowBackoffSettings()
                                    .SlotDuration(TDuration::MilliSeconds(100)));
    return settings;
}

} // namespace

const TRetryOperationSettings& DefaultYdbRetrySettings() {
    static const auto settings = MakeDefaultYdbRetrySettings();
    return settings;
}

} // namespace NYdbHelpers
