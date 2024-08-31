#pragma once

#include "constants.h"

namespace NAlice::NUniproxy {
    template <typename T, typename... TArgs>
    T DoWithTimingsRecord(TSensor& sensor, std::function<T(TArgs...)> doFunc,
                          TArgs... args) {
        auto begin = TInstant::Now();
        T result = doFunc(args...);
        auto end = TInstant::Now();
        sensor.Histogram->Record((end - begin).MilliSeconds());
        return result;
    }
}
