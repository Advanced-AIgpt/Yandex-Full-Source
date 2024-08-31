#pragma once

#include <alice/hollywood/library/scenarios/metronome/proto/metronome_scenario_state.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>

namespace NAlice::NHollywoodFw::NMetronome {

constexpr i64 DEFAULT_SHIFT = 5;
constexpr i64 SLIGHT_SHIFT = 2;
constexpr i64 SIGNIFICANT_SHIFT = 20;
constexpr i64 DEFAULT_BPM = 120;

struct TMetronome {
    i64 Bpm;
    TString TrackId;

    TString Print() const {
        return TStringBuilder{} << "{bpm=" << Bpm <<", track_id=" << TrackId <<"}";
    }
};

struct TUpdateMetronomeData {
    TMetronomeScenarioUpdateArguments::EMethod Method;
    TMaybe<i64> Value;
    i64 CurrentBpm;
};

TMetronome GetMetronome(i64 bpm, TString& responseType);
TMetronome GetUpdatedMetronome(const TUpdateMetronomeData& data, TString& responseType);

} // namespace NAlice::NHollywoodFw::NMetronome
