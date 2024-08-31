#include "bass_cache.h"

#include <library/cpp/threading/future/future.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>

using namespace NBASS;
using namespace NThreading;

namespace {
Y_UNIT_TEST_SUITE(BassCache) {
    Y_UNIT_TEST(Smoke) {
        const auto delay = TDuration::MilliSeconds(500);

        auto promise = NewPromise<void>();
        auto future = promise.GetFuture();

        const auto now = TInstant::Now();

        TCacheManager manager;

        manager.Schedule(
            [&promise, &now, &delay]() {
                UNIT_ASSERT(TInstant::Now() >= now + delay);
                promise.SetValue();
                return TDuration{};
            },
            delay);

        future.Wait();
    }
}
} // namespace
