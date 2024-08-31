#pragma once

#include <alice/protos/api/matrix/scheduled_action.pb.h>

#include <util/datetime/base.h>


namespace NMatrix::NWorker {

TInstant GetNextTimeByRetryPolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy& retryPolicy,
    ui64 consecutiveFailuresCounter
);

// SendOncePolicy
TInstant GetNextRetryTimeBySendOncePolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendOncePolicy& sendOncePolicy,
    ui64 consecutiveFailuresCounter
);

// SendPeriodicallyPolicy
TInstant GetNextRetryTimeBySendPeriodicallyPolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendPeriodicallyPolicy& sendPeriodicallyPolicy,
    ui64 consecutiveFailuresCounter
);

TInstant GetNextTimeBySendPeriodicallyPolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendPeriodicallyPolicy& sendPeriodicallyPolicy
);

} // namespace NMatrix::NWorker
