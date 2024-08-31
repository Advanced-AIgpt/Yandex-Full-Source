#include "common.h"
#include "radio_parser.h"
#include "track_parser.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>

#include <util/generic/hash_set.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf ACCEPTED_SEEDS = "acceptedSeeds";
constexpr TStringBuf BATCH_ID = "batchId";
constexpr TStringBuf DESCRIPTION_SEED = "descriptionSeed";
constexpr TStringBuf RADIO_SESSION_ID = "radioSessionId";
constexpr TStringBuf SEQUENCE = "sequence";
constexpr TStringBuf PUMPKIN = "pumpkin";

void LogSeeds(TRTLogger& logger, const NJson::TJsonValue::TArray& acceptedSeedsJson, const TVector<TStringBuf>& sentSeeds) {
    THashSet<TString> acceptedSeeds;
    for (const auto& item : acceptedSeedsJson) {
        acceptedSeeds.insert(TString::Join(item["type"].GetString(), ":", item["value"].GetString()));
    }
    LOG_INFO(logger) << "Accepted radio seeds: " << JoinSeq(", ", acceptedSeeds);

    for (const auto& sentSeed : sentSeeds) {
        if (!acceptedSeeds.contains(sentSeed)) {
            LOG_WARN(logger) << "Ignored radio seed: " << sentSeed;
        }
    }
}

void AddRepeatedTrackRate(NMetrics::ISensors& sensors) {
    static const NMonitoring::TLabels labels{
        {"scenario_name", "music"},
        {"name", "thin_client_radio_repeated_tracks"},
    };
    sensors.IncRate(labels);
}

void LogAndSensorIfRepeatedTrack(TRTLogger& logger, NMetrics::ISensors& sensors, TMusicQueueWrapper& mq, const TString& trackId) {
    if (!mq.HasTrackInHistory(trackId, TContentId_EContentType_Radio)) {
        // the track has not been seen in recent history
        return;
    }
    LOG_ERROR(logger) << "Has track in history: trackId=" << trackId;
    AddRepeatedTrackRate(sensors);
}

void LogAndSensorIfPumpkin(TRTLogger& logger, NMetrics::ISensors& sensors, const bool isPumpkin) {
    static const NMonitoring::TLabels labels{
        {"scenario_name", "music"},
        {"name", "thin_client_radio_pumpkins"},
    };

    if (!isPumpkin) {
        return;
    }
    LOG_WARNING(logger) << "Has pumpkin radio response";
    sensors.IncRate(labels);
}

}

void ParseRadio(TRTLogger& logger, NMetrics::ISensors& sensors, const NJson::TJsonValue& radioJson,
                TMusicQueueWrapper& mq, bool hasMusicSubscription) {
    mq.SetRadioBatchId(radioJson[BATCH_ID].GetStringSafe());

    // only "/session/new" handler returns new session id
    // "/session/{radioSessionId}/tracks" doesn't have (we use a known sessionId)
    if (const auto* sessionIdJson = radioJson.GetValueByPath(RADIO_SESSION_ID); sessionIdJson && sessionIdJson->IsString()) {
        mq.SetRadioSessionId(sessionIdJson->GetString());
    } else {
        mq.SetRadioSessionId(mq.GetRadioSessionId()); // copy "next" radioSessionId from "previous" radioSessionId if possible
    }

    mq.SetIsRadioPumpkin(radioJson[PUMPKIN].GetBooleanRobust());
    if (radioJson.Has(DESCRIPTION_SEED)) {
        mq.UpdateContentId(TString::Join(radioJson[DESCRIPTION_SEED]["type"].GetString(), ":",
                                         radioJson[DESCRIPTION_SEED]["value"].GetString()));
    } else if (mq.IsRadioPumpkin()) {
        mq.UpdateContentId(NMusic::USER_RADIO_STATION_ID);
    }

    if (radioJson.Has(ACCEPTED_SEEDS)) {
        // Only "/session/new" handler has this field
        LogSeeds(logger, radioJson[ACCEPTED_SEEDS].GetArray(), mq.ContentIdsValues());
    }

    LogAndSensorIfPumpkin(logger, sensors, mq.IsRadioPumpkin());

    const auto& sequence = radioJson[SEQUENCE].GetArraySafe();
    for (const auto& item : sequence) {
        const auto& trackJson = item[TRACK];
        auto track = ParseTrack(trackJson);
        LogAndSensorIfRepeatedTrack(logger, sensors, mq, track.GetTrackId());
        mq.TryAddItem(std::move(track), hasMusicSubscription);
    }
}

} // NAlice::NHollywoood::NMusic
