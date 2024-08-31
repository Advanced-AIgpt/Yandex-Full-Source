#pragma once

#include <library/cpp/watchdog/timeout/watchdog.h>
#include <library/cpp/watchdog/watchdog.h>

#include <util/generic/vector.h>

#include <memory>

namespace NAlice::NMegamind {

using TWatchDogPtr = std::unique_ptr<IWatchDog>;

class TWatchDogHolder {
public:
    TWatchDogHolder& Add(TWatchDogPtr watchdog) {
        WatchDogs_.emplace_back(std::move(watchdog));
        return *this;
    }

private:
    TVector<std::unique_ptr<IWatchDog>> WatchDogs_;
};

} // namespace NAlice::NMegamind
