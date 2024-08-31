#pragma once

#include <util/generic/hash.h>

namespace NAlice::NHollywood::NWeather {

struct TAvatar {
    TAvatar(TStringBuf http, TStringBuf https);

    const TString Http;
    const TString Https;
};

class TAvatarsMap {
public:
    TAvatarsMap(const double screenScaleFactor);

    const TAvatar* Find(const TStringBuf name, const TStringBuf suffix = TStringBuf(".png")) const;

private:
    const double MatchScreenScaleFactor_;
};

} // namespace NAlice::NHollywood::NWeather
