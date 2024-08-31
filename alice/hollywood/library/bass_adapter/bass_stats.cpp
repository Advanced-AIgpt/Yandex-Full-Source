#include "bass_stats.h"

#include <alice/library/metrics/util.h>

#include <util/generic/hash_set.h>
#include <util/string/join.h>

namespace NAlice::NHollywood {

namespace NImpl {

namespace {

// taken from searchapp landing entities
const THashSet<TStringBuf> ENTITY_TYPES = {
    TStringBuf("album"),
    TStringBuf("track"),
    TStringBuf("artist"),
    TStringBuf("playlist"),
    TStringBuf("chart"),
};

NMonitoring::TLabels CreateLabels(const TString& scenarioName, const TString& requestType) {
    return {
        {"scenario_name", scenarioName},
        {"scenario_handle", requestType},
    };
}

struct TSensorParams {
    TString AnswerType;
    TString SpecialPlaylist;
};

TString CreateRequestTypeLabelValue(const TSensorParams& params) {
    if (params.SpecialPlaylist) {
        // There are tons of special playlists,
        // and the only very slow one so far is playlist_of_the_day.
        return (params.SpecialPlaylist == PLAYLIST_OF_THE_DAY) ? PLAYLIST_OF_THE_DAY : SPECIAL_PLAYLIST;
    }

    if (ENTITY_TYPES.contains(params.AnswerType)) {
        // entities are basically processed the same way so we smoosh them together
        return ENTITY;
    }

    // not too many answer types left
    return params.AnswerType ? params.AnswerType : EMPTY;
}

TSensorParams ExtractSensorParams(const NJson::TJsonValue::TArray& slots) {
    TSensorParams result;

    bool gotAnswer = false;
    bool gotSpecialPlaylist = false;
    for (const auto& slot : slots) {
        const auto& slotName = slot["name"].GetString();
        if (!gotAnswer && slotName == "answer") {
            result.AnswerType = slot["value"]["type"].GetString();
            gotAnswer = true;
        } else if (!gotSpecialPlaylist && slotName == SPECIAL_PLAYLIST) {
            result.SpecialPlaylist = slot["value"].GetString();
            gotSpecialPlaylist = true;
        }

        if (gotAnswer && gotSpecialPlaylist) {
            break;
        }
    }

    return result;
}

} // namespace

TString ExtractMusicType(const NJson::TJsonValue& form) {
    const auto params = ExtractSensorParams(form["slots"].GetArray());
    return CreateRequestTypeLabelValue(params);
}

} // namespace NImpl

void ProcessBassResponseUpdateSensors(TRTLogger& logger,
                                      NMetrics::ISensors& sensors,
                                      const NJson::TJsonValue& bassResponse,
                                      const TString& scenarioName,
                                      const TString& requestType) {
    const auto& blocks = bassResponse["blocks"].GetArray();

    // Search for stats block
    const auto blockPtr = FindIfPtr(blocks, [](const auto& block) {
        return block["type"].GetString() == "stats";
    });
    if (!blockPtr) {
        LOG_DEBUG(logger) << "Stats block not found";
        return;
    }
    const auto& statsBlockData = (*blockPtr)["data"];

    auto musicType = NImpl::ExtractMusicType(bassResponse["form"]);

    auto baseLabels = NImpl::CreateLabels(scenarioName, requestType);
    for (const auto& [key, val] : statsBlockData.GetMap()) {
        auto labels = baseLabels;
        labels.Add("bass_source", key);
        labels.Add("music_type", musicType);

        const ui64 value = static_cast<ui64>(Max(val.GetInteger(), 0LL));
        sensors.AddHistogram(labels, value, NMetrics::TIME_INTERVALS);
        LOG_DEBUG(logger) << "Added histogram value for sensor " << key;
    }
}

} // namespace NAlice::NHollywood
