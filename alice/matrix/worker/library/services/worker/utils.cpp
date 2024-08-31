#include "utils.h"

#include <library/cpp/protobuf/interop/cast.h>

namespace NMatrix::NWorker {

namespace {

// Just some disclamers
//
// TInstant/TDuration sum are safe to overflow
// https://a.yandex-team.ru/arc/trunk/arcadia/util/datetime/base.h?rev=r9348586#L636-640
//
// However multiplication is not
// https://a.yandex-team.ru/arc/trunk/arcadia/util/datetime/base.h?rev=r9348586#L774-779

TDuration SafeMul(TDuration a, ui64 b) {
    if (b == 0) {
        return TDuration::Zero();
    }

    {
        static_assert(TDuration::Max() == TDuration::FromValue(Max<TDuration::TValue>()));
        if (a.GetValue() > Max<TDuration::TValue>() / b) {
            return TDuration::Max();
        }
    }

    return a * b;
}

ui64 SafeMul(ui64 a, ui64 b) {
    ui64 res;
    if (__builtin_umulll_overflow(a, b, (unsigned long long*)&res)) {
        return Max<ui64>();
    }

    return res;
}

ui64 SafePow(ui64 x, ui64 n) {
    ui64 res = 1;
    while (n > 0) {
        if (n & 1) {
            res = SafeMul(res, x);
        }
        x = SafeMul(x, x);
        n >>= 1;
    }

    return res;

}

} // namespace

TInstant GetNextTimeByRetryPolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TRetryPolicy& retryPolicy,
    ui64 consecutiveFailuresCounter
) {
    TDuration restartPeriodScale = NProtoInterop::CastFromProto(retryPolicy.GetRestartPeriodScale());
    ui64 restartPeriodBackOff = retryPolicy.GetRestartPeriodBackOff();
    TDuration minRestartPeriod = NProtoInterop::CastFromProto(retryPolicy.GetMinRestartPeriod());
    TDuration maxRestartPeriod = NProtoInterop::CastFromProto(retryPolicy.GetMaxRestartPeriod());

    return now + Min(
        maxRestartPeriod,
        minRestartPeriod + SafeMul(restartPeriodScale, SafePow(restartPeriodBackOff, consecutiveFailuresCounter))
    );
}

// SendOncePolicy
TInstant GetNextRetryTimeBySendOncePolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendOncePolicy& sendOncePolicy,
    ui64 consecutiveFailuresCounter
) {
    return GetNextTimeByRetryPolicy(now, sendOncePolicy.GetRetryPolicy(), consecutiveFailuresCounter);
}

// SendPeriodicallyPolicy
TInstant GetNextRetryTimeBySendPeriodicallyPolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendPeriodicallyPolicy& sendPeriodicallyPolicy,
    ui64 consecutiveFailuresCounter
) {
    // TODO(chegoryu) Add retry policy to SendPeriodicallyPolicy
    // return GetNextTimeByRetryPolicy(now, sendPeriodicallyPolicy.GetRetryPolicy(), consecutiveFailuresCounter);
    Y_UNUSED(consecutiveFailuresCounter);
    return GetNextTimeBySendPeriodicallyPolicy(now, sendPeriodicallyPolicy);
}

TInstant GetNextTimeBySendPeriodicallyPolicy(
    TInstant now,
    const NMatrix::NApi::TScheduledActionSpec::TSendPolicy::TSendPeriodicallyPolicy& sendPeriodicallyPolicy
) {
    return now + NProtoInterop::CastFromProto(sendPeriodicallyPolicy.GetPeriod());
}

} // namespace NMatrix::NWorker
