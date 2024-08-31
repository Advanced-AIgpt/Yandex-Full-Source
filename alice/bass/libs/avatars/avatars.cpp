#include "avatars.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>

#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace {

using TAvMap = THashMap<TString, TAvatar>;

TAvMap ConstructMap(TStringBuf fileName = TStringBuf()) {
    NSc::TValue data;
    bool loadedFromFile = false;

    if (fileName) {
        TFileInput in(ToString(fileName));
        data = NSc::TValue::FromJson(in.ReadAll());
    }

    if (data.IsNull()) {
        TString rawData;
        if (NResource::FindExact("avatars_map", &rawData)) {
            data = NSc::TValue::FromJson(rawData);
        }
    }
    else {
        loadedFromFile = true;
    }

    TAvMap map;

    if (data.IsNull()) {
        LOG(ERR) << "Unable to load icons map for weather" << Endl;
    }
    else {
        for (const auto& kv : data.GetDict()) {
            const TString key = TStringBuilder() << kv.first << ':';

            for (const auto& kv : kv.second.GetDict()) {
                map.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(TStringBuilder() << key << kv.first),
                    std::forward_as_tuple(kv.second["http"].GetString(), kv.second["https"].GetString())
                );
            }
        }

        LOG(DEBUG) << "Avatars map loaded from " << (loadedFromFile ? "file" : "resources") << ": " << map.size() << Endl;
    }

    return map;
}

} // anon namespace

TAvatarsMap::TAvatarsMap()
    : Map(ConstructMap())
{
}

TAvatarsMap::TAvatarsMap(TStringBuf filename)
    : Map(ConstructMap(filename))
{
}

const TAvatar* TAvatarsMap::Get(TStringBuf ns, TStringBuf key) const {
    return Map.FindPtr(TStringBuilder() << ns << ':' << key);
}

TAvatar::TAvatar(TStringBuf http, TStringBuf https)
    : Http(http)
    , Https(https)
{
}

