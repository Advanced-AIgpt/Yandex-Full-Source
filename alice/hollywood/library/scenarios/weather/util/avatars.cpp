#include "avatars.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/resource/resource.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NWeather {

namespace {

//BE CAREFUL! Make sure all scale factors are supported by avatars.mds!
const TVector<float> ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT = { 1, 1.5, 2, 3, 3.5, 4 };

float MatchScreenScaleFactor(double screenScaleFactor) {
    const float desiredScale = screenScaleFactor - std::numeric_limits<float>::epsilon();
    for (float s: ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT) {
        if (s >= desiredScale) {
            return s;
        }
    }
    return ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT.back();
}

THashMap<TString, TAvatar> ConstructMap() {
    TString rawAvatars = NResource::Find("avatars.json");
    NJson::TJsonValue map;
    NJson::ReadJsonFastTree(rawAvatars, &map);

    THashMap<TString, TAvatar> result;

    NJson::TJsonValue subMap = map["weather"];
    for (const auto& kv : subMap.GetMap()) {
        result.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(kv.first),
            std::forward_as_tuple(kv.second["http"].GetString(), kv.second["https"].GetString())
        );
    }

    return result;
}

const THashMap<TString, TAvatar> AVATARS_MAP = ConstructMap();

} // namespace

TAvatarsMap::TAvatarsMap(const double screenScaleFactor)
    : MatchScreenScaleFactor_{MatchScreenScaleFactor(screenScaleFactor)}
{
}

const TAvatar* TAvatarsMap::Find(const TStringBuf name, const TStringBuf suffix) const {
    return AVATARS_MAP.FindPtr(TStringBuilder{} << name << '.' << MatchScreenScaleFactor_ << suffix);
}

TAvatar::TAvatar(TStringBuf http, TStringBuf https)
    : Http(http)
    , Https(https)
{
}

} // namespace NAlice::NHollywood::NWeather
