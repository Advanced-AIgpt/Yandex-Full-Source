#include "metronome_helpers.h"

#include <util/generic/ymath.h>

namespace NAlice::NHollywoodFw::NMetronome {

namespace {

// Sorted by bpm!
const TVector<TMetronome> METRONOMES = {
    {/* bpm= */ 40, /* track_id= */ "30657961"},
    {/* bpm= */ 41, /* track_id= */ "30657962"},
    {/* bpm= */ 42, /* track_id= */ "30657963"},
    {/* bpm= */ 43, /* track_id= */ "30657964"},
    {/* bpm= */ 44, /* track_id= */ "30657965"},
    {/* bpm= */ 45, /* track_id= */ "30657966"},
    {/* bpm= */ 46, /* track_id= */ "30657967"},
    {/* bpm= */ 47, /* track_id= */ "30657968"},
    {/* bpm= */ 48, /* track_id= */ "30657969"},
    {/* bpm= */ 49, /* track_id= */ "30657970"},
    {/* bpm= */ 50, /* track_id= */ "30657971"},
    {/* bpm= */ 51, /* track_id= */ "30657972"},
    {/* bpm= */ 52, /* track_id= */ "30657973"},
    {/* bpm= */ 53, /* track_id= */ "30657974"},
    {/* bpm= */ 54, /* track_id= */ "30657975"},
    {/* bpm= */ 55, /* track_id= */ "30657976"},
    {/* bpm= */ 56, /* track_id= */ "30657977"},
    {/* bpm= */ 57, /* track_id= */ "30657978"},
    {/* bpm= */ 58, /* track_id= */ "30657979"},
    {/* bpm= */ 59, /* track_id= */ "30657980"},
    {/* bpm= */ 60, /* track_id= */ "30657981"},
    {/* bpm= */ 61, /* track_id= */ "30657982"},
    {/* bpm= */ 62, /* track_id= */ "30657983"},
    {/* bpm= */ 63, /* track_id= */ "30657984"},
    {/* bpm= */ 64, /* track_id= */ "30657985"},
    {/* bpm= */ 65, /* track_id= */ "30657986"},
    {/* bpm= */ 66, /* track_id= */ "30657987"},
    {/* bpm= */ 67, /* track_id= */ "30657988"},
    {/* bpm= */ 68, /* track_id= */ "30657989"},
    {/* bpm= */ 69, /* track_id= */ "30657990"},
    {/* bpm= */ 70, /* track_id= */ "30657991"},
    {/* bpm= */ 71, /* track_id= */ "30657992"},
    {/* bpm= */ 72, /* track_id= */ "30657993"},
    {/* bpm= */ 73, /* track_id= */ "30657994"},
    {/* bpm= */ 74, /* track_id= */ "30657995"},
    {/* bpm= */ 75, /* track_id= */ "30657996"},
    {/* bpm= */ 76, /* track_id= */ "30657997"},
    {/* bpm= */ 77, /* track_id= */ "30657998"},
    {/* bpm= */ 78, /* track_id= */ "30657999"},
    {/* bpm= */ 79, /* track_id= */ "30658000"},
    {/* bpm= */ 80, /* track_id= */ "30658001"},

    {/* bpm= */ 81, /* track_id= */ "30688688"},
    {/* bpm= */ 82, /* track_id= */ "30688689"},
    {/* bpm= */ 83, /* track_id= */ "30688690"},
    {/* bpm= */ 84, /* track_id= */ "30688691"},
    {/* bpm= */ 85, /* track_id= */ "30688692"},
    {/* bpm= */ 86, /* track_id= */ "30688693"},
    {/* bpm= */ 87, /* track_id= */ "30688694"},
    {/* bpm= */ 88, /* track_id= */ "30688695"},
    {/* bpm= */ 89, /* track_id= */ "30688696"},
    {/* bpm= */ 90, /* track_id= */ "30688697"},
    {/* bpm= */ 91, /* track_id= */ "30688698"},
    {/* bpm= */ 92, /* track_id= */ "30688699"},
    {/* bpm= */ 93, /* track_id= */ "30688700"},
    {/* bpm= */ 94, /* track_id= */ "30688701"},
    {/* bpm= */ 95, /* track_id= */ "30688702"},
    {/* bpm= */ 96, /* track_id= */ "30688703"},
    {/* bpm= */ 97, /* track_id= */ "30688704"},
    {/* bpm= */ 98, /* track_id= */ "30688705"},
    {/* bpm= */ 99, /* track_id= */ "30688706"},
    {/* bpm= */ 100, /* track_id= */ "30688707"},
    {/* bpm= */ 101, /* track_id= */ "30688708"},
    {/* bpm= */ 102, /* track_id= */ "30688709"},
    {/* bpm= */ 103, /* track_id= */ "30688710"},
    {/* bpm= */ 104, /* track_id= */ "30688711"},
    {/* bpm= */ 105, /* track_id= */ "30688712"},
    {/* bpm= */ 106, /* track_id= */ "30688713"},
    {/* bpm= */ 107, /* track_id= */ "30688714"},
    {/* bpm= */ 108, /* track_id= */ "30688715"},
    {/* bpm= */ 109, /* track_id= */ "30688716"},
    {/* bpm= */ 110, /* track_id= */ "30688717"},
    {/* bpm= */ 111, /* track_id= */ "30688718"},
    {/* bpm= */ 112, /* track_id= */ "30688719"},
    {/* bpm= */ 113, /* track_id= */ "30688720"},
    {/* bpm= */ 114, /* track_id= */ "30688721"},
    {/* bpm= */ 115, /* track_id= */ "30688722"},
    {/* bpm= */ 116, /* track_id= */ "30688723"},
    {/* bpm= */ 117, /* track_id= */ "30688724"},
    {/* bpm= */ 118, /* track_id= */ "30688725"},
    {/* bpm= */ 119, /* track_id= */ "30688726"},
    {/* bpm= */ 120, /* track_id= */ "30688727"},
    {/* bpm= */ 121, /* track_id= */ "30688728"},
    {/* bpm= */ 122, /* track_id= */ "30688729"},
    {/* bpm= */ 123, /* track_id= */ "30688730"},
    {/* bpm= */ 124, /* track_id= */ "30688731"},
    {/* bpm= */ 125, /* track_id= */ "30688732"},
    {/* bpm= */ 126, /* track_id= */ "30688733"},
    {/* bpm= */ 127, /* track_id= */ "30688734"},
    {/* bpm= */ 128, /* track_id= */ "30688735"},
    {/* bpm= */ 129, /* track_id= */ "30688736"},
    {/* bpm= */ 130, /* track_id= */ "30688737"},
    {/* bpm= */ 131, /* track_id= */ "30688738"},
    {/* bpm= */ 132, /* track_id= */ "30688739"},
    {/* bpm= */ 133, /* track_id= */ "30688740"},
    {/* bpm= */ 134, /* track_id= */ "30688741"},
    {/* bpm= */ 135, /* track_id= */ "30688742"},
    {/* bpm= */ 136, /* track_id= */ "30688743"},
    {/* bpm= */ 137, /* track_id= */ "30688744"},
    {/* bpm= */ 138, /* track_id= */ "30688745"},
    {/* bpm= */ 139, /* track_id= */ "30688746"},
    {/* bpm= */ 140, /* track_id= */ "30688747"},
    {/* bpm= */ 141, /* track_id= */ "30688748"},
    {/* bpm= */ 142, /* track_id= */ "30688749"},
    {/* bpm= */ 143, /* track_id= */ "30688750"},
    {/* bpm= */ 144, /* track_id= */ "30688751"},
};

} // namespace

TMetronome GetMetronome(i64 bpm, TString& responseType) {
    responseType = "exact";
    auto targetMetronome = METRONOMES[0];
    i64 bestBmpDiff = Abs(targetMetronome.Bpm - bpm);
    for (const auto& metronome : METRONOMES) {
        i64 bpmDiff = Abs(metronome.Bpm - bpm);
        if (bpmDiff < bestBmpDiff) {
            bestBmpDiff = bpmDiff;
            targetMetronome = metronome;
        }
    }

    if (bpm < targetMetronome.Bpm) {
        responseType = "min";
    } else if (bpm > targetMetronome.Bpm) {
        responseType = "max";
    }
    return targetMetronome;
}

TMetronome GetUpdatedMetronome(const TUpdateMetronomeData& data, TString& responseType) {
    i64 newBpm = data.CurrentBpm;
    switch (data.Method) {
        case TMetronomeScenarioUpdateArguments_EMethod_Faster:
            newBpm += DEFAULT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_Slower:
            newBpm -= DEFAULT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_SignificantlyFaster:
            newBpm += SIGNIFICANT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_SignificantlySlower:
            newBpm -= SIGNIFICANT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_SlightlyFaster:
            newBpm += SLIGHT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_SlightlySlower:
            newBpm -= SLIGHT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_FasterBy:
            newBpm += data.Value.Defined() ? *data.Value : DEFAULT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_SlowerBy:
            newBpm -= data.Value.Defined() ? *data.Value : DEFAULT_SHIFT;
            break;
        case TMetronomeScenarioUpdateArguments_EMethod_SetExactValue:
            newBpm = data.Value.Defined() ? *data.Value : DEFAULT_BPM;
            break;
        default:
            break;
    }
    auto targetMetronome = GetMetronome(newBpm, responseType);
    return targetMetronome;
}

} // namespace NAlice::NHollywoodFw::NMetronome
