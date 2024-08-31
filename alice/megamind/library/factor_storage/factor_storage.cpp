#include "factor_storage.h"

namespace NAlice::NMegamind {

TFactorDomain CreateFactorDomain() {
    return TFactorDomain{
        NFactorSlices::EFactorSlice::ALICE_BEGEMOT_QUERY_FACTORS,
        NFactorSlices::EFactorSlice::BEGEMOT_QUERY_FACTORS,
        NFactorSlices::EFactorSlice::BLENDER_PRODUCTION,
        NFactorSlices::EFactorSlice::ALICE_SEARCH_SCENARIO,
        NFactorSlices::EFactorSlice::ALICE_VIDEO_SCENARIO,
        NFactorSlices::EFactorSlice::ALICE_MUSIC_SCENARIO,
        NFactorSlices::EFactorSlice::ALICE_DIRECT_SCENARIO,
        NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE,
        NFactorSlices::EFactorSlice::ALICE_QUERY_TOKENS_FACTORS,
        NFactorSlices::EFactorSlice::ALICE_ASR_FACTORS,
        NFactorSlices::EFactorSlice::ALICE_GC_SCENARIO,
        NFactorSlices::EFactorSlice::ALICE_BEGEMOT_NLU_FACTORS,
    };
}

TFactorStorage CreateFactorStorage(const NFactorSlices::TFactorDomain& factorDomain) {
    TFactorStorage factorStorage{factorDomain};
    factorStorage.Clear();
    return factorStorage;
}

} // namespace NAlice::NMegamind
