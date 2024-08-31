#include "query_tokens_features.h"

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/scenarios/defs/names.h>

#include <kernel/alice/query_tokens_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/iterator/filtering.h>

#include <util/string/split.h>

using namespace NAliceQueryTokensFactors;

namespace NAlice {

using namespace NKvSaaS;

namespace {

const TSet<TStringBuf> MUSIC_INTENTS = {
    HOLLYWOOD_MUSIC_SCENARIO,
};

const TSet<TStringBuf> VIDEO_INTENTS = {
    MM_VIDEO_PROTOCOL_SCENARIO,
};

const TSet<TStringBuf> VIDEO_SELECT_FROM_GALLERY_INTENTS = {
    MM_MORDOVIA_VIDEO_SELECTION_PROTOCOL_SCENARIO,
};

const TSet<TStringBuf> GC_INTENTS = {
    PROTOCOL_GENERAL_CONVERSATION_SCENARIO,
};

const TSet<TStringBuf> SEARCH_INTENTS = {
    MM_SEARCH_PROTOCOL_SCENARIO,
};

const TMap<EClientType, TVector<TStringBuf>> APP_MAPPING = {
    {
        ECT_SMART_SPEAKER,
        {"quasar", "small_smart_speakers", "elariwatch"}
    },
    {
        ECT_SMART_TV,
        {"tv"}
    },
    {
        ECT_TOUCH,
        {"launcher", "music_app_prod", "search_app_prod", "yandex_phone", "browser_prod"}
    }
};

struct TTokenFrequencies {
    float Music = 0.0;
    float Video = 0.0;
    float VideoSelectFromGallery = 0.0;
    float GC = 0.0;
    float Search = 0.0;
    float Other = 0.0;
};

TTokenFrequencies GetTokenFrequencies(const TIntentsStatRecord::TIntentsStat& stats) {
    if (stats.GetTotalCount() == 0) {
        return {};
    }

    ui64 totalMusicCount = 0;
    ui64 totalVideoCount = 0;
    ui64 videoSelectFromGalleryCount = 0;
    ui64 totalGCCount = 0;
    ui64 totalSearchCount = 0;

    for (const auto& intent : stats.GetIntents()) {
        if (MUSIC_INTENTS.contains(intent.GetIntent())) {
            totalMusicCount += intent.GetCount();
        }
        else if (VIDEO_INTENTS.contains(intent.GetIntent())) {
            totalVideoCount += intent.GetCount();
        }
        else if (GC_INTENTS.contains(intent.GetIntent())) {
            totalGCCount += intent.GetCount();
        }
        else if (SEARCH_INTENTS.contains(intent.GetIntent())) {
            totalSearchCount += intent.GetCount();
        }

        if (VIDEO_SELECT_FROM_GALLERY_INTENTS.contains(intent.GetIntent())) {
            videoSelectFromGalleryCount += intent.GetCount();
        }
    }

    ui64 totalOtherCount = stats.GetTotalCount() - totalMusicCount - totalVideoCount -
        totalGCCount - totalSearchCount;

    return {
        static_cast<float>(totalMusicCount) / stats.GetTotalCount(),
        static_cast<float>(totalVideoCount) / stats.GetTotalCount(),
        static_cast<float>(videoSelectFromGalleryCount) / stats.GetTotalCount(),
        static_cast<float>(totalGCCount) / stats.GetTotalCount(),
        static_cast<float>(totalSearchCount) / stats.GetTotalCount(),
        static_cast<float>(totalOtherCount) / stats.GetTotalCount(),
    };
}

struct TTokenFeatures {
    float MaxMusicFreq = 0.0;
    float MaxVideoFreq = 0.0;
    float AvgMusicFreq = 0.0;
    float AvgVideoFreq = 0.0;
    float MaxMusicFreqWHF = 0.0;
    float MaxVideoFreqWHF = 0.0;
    float AvgMusicFreqWHF = 0.0;
    float AvgVideoFreqWHF = 0.0;
    float MaxVideoSelectFromGalleryFreq = 0.0;
    float AvgVideoSelectFromGalleryFreq = 0.0;
    float MaxGCFreq = 0.0;
    float AvgGCFreq = 0.0;
    float MaxSearchFreq = 0.0;
    float AvgSearchFreq = 0.0;
    float MaxOtherFreq = 0.0;
    float AvgOtherFreq = 0.0;
};

class TNgramsFilter {
public:
    explicit TNgramsFilter(ui32 ngramSize)
        : NgramSize(ngramSize) {
    }

    bool operator()(const TTokensStatsResponse::TTokenStatsByClients& tokenStats) const {
        return (Count(tokenStats.Token, ' ') + 1) == NgramSize;
    }

private:
    ui32 NgramSize = 0;
};

template <typename TFilter>
TTokenFeatures GetTokensFeatures(const TVector<TTokensStatsResponse::TTokenStatsByClients>& tokensStats, TVector<TStringBuf> apps, TFilter filter) {
    TTokenFeatures tokenFeatures;

    float totalMusicFreq = 0.0;
    float totalVideoFreq = 0.0;
    float totalMusicFreqWHF = 0.0;
    float totalVideoFreqWHF = 0.0;
    float totalVideoSelectFromGalleryFreq = 0.0;
    float totalGCFreq = 0.0;
    float totalSearchFreq = 0.0;
    float totalOtherFreq = 0.0;

    ui64 nonHighFrequencyCount = 0;
    ui64 tokensCount = 0;

    for (const auto& tokenStatsByClient : MakeFilteringRange(tokensStats, filter)) {
        for (const auto& [client, stats] : tokenStatsByClient.ClientsStats.GetClientIntentsStat()) {
            if (!IsIn(apps, client)) {
                continue;
            }
            ++tokensCount;

            const auto frequencies = GetTokenFrequencies(stats);

            tokenFeatures.MaxMusicFreq = std::max(tokenFeatures.MaxMusicFreq, frequencies.Music);
            tokenFeatures.MaxVideoFreq = std::max(tokenFeatures.MaxVideoFreq, frequencies.Video);
            tokenFeatures.MaxVideoSelectFromGalleryFreq = std::max(tokenFeatures.MaxVideoSelectFromGalleryFreq,
                frequencies.VideoSelectFromGallery);

            tokenFeatures.MaxGCFreq = std::max(tokenFeatures.MaxGCFreq, frequencies.GC);
            tokenFeatures.MaxSearchFreq = std::max(tokenFeatures.MaxSearchFreq, frequencies.Search);
            tokenFeatures.MaxOtherFreq = std::max(tokenFeatures.MaxOtherFreq, frequencies.Other);

            totalMusicFreq += frequencies.Music;
            totalVideoFreq += frequencies.Video;
            totalVideoSelectFromGalleryFreq += frequencies.VideoSelectFromGallery;
            totalGCFreq += frequencies.GC;
            totalSearchFreq += frequencies.Search;
            totalOtherFreq += frequencies.Other;

            if (frequencies.Music < 0.1 || frequencies.Video < 0.1) {
                tokenFeatures.MaxMusicFreqWHF = std::max(tokenFeatures.MaxMusicFreqWHF, frequencies.Music);
                tokenFeatures.MaxVideoFreqWHF = std::max(tokenFeatures.MaxVideoFreqWHF, frequencies.Video);

                totalMusicFreqWHF += frequencies.Music;
                totalVideoFreqWHF += frequencies.Video;

                ++nonHighFrequencyCount;
            }
        }
    }

    tokenFeatures.AvgMusicFreq = tokensCount > 0 ? totalMusicFreq / tokensCount : 0.0;
    tokenFeatures.AvgVideoFreq = tokensCount > 0 ? totalVideoFreq / tokensCount : 0.0;
    tokenFeatures.AvgMusicFreqWHF = nonHighFrequencyCount > 0 ? totalMusicFreqWHF / nonHighFrequencyCount : 0.0;
    tokenFeatures.AvgVideoFreqWHF = nonHighFrequencyCount > 0 ? totalVideoFreqWHF / nonHighFrequencyCount : 0.0;
    tokenFeatures.AvgVideoSelectFromGalleryFreq =
        tokensCount > 0 ? totalVideoSelectFromGalleryFreq / tokensCount : 0.0;
    tokenFeatures.AvgGCFreq = tokensCount > 0 ? totalGCFreq / tokensCount : 0.0;
    tokenFeatures.AvgSearchFreq = tokensCount > 0 ? totalSearchFreq / tokensCount : 0.0;
    tokenFeatures.AvgOtherFreq = tokensCount > 0 ? totalOtherFreq / tokensCount : 0.0;

    return tokenFeatures;
}

} // namespace

void FillQueryTokensFactors(const TTokensStatsResponse& tokensStatsResponse, const TClientFeatures& clientFeatures, TFactorStorage& storage) {
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_QUERY_TOKENS_FACTORS);

    const auto& tokensStats = tokensStatsResponse.GetTokensStatsByClients();

    const auto clientType = GetClientType(clientFeatures);
    const auto* currentApps = APP_MAPPING.FindPtr(clientType);

    if (!currentApps) {
        return;
    }

    const auto unigramFeatures = GetTokensFeatures(tokensStats, *currentApps, TNgramsFilter(1u));
    const auto bigramFeatures = GetTokensFeatures(tokensStats, *currentApps, TNgramsFilter(2u));

    view[FI_QUERY_TOKEN_MAX_FREQ_MUSIC] = unigramFeatures.MaxMusicFreq;
    view[FI_QUERY_TOKEN_MAX_FREQ_VIDEO] = unigramFeatures.MaxVideoFreq;
    view[FI_QUERY_TOKEN_AVG_FREQ_MUSIC] = unigramFeatures.AvgMusicFreq;
    view[FI_QUERY_TOKEN_AVG_FREQ_VIDEO] = unigramFeatures.AvgVideoFreq;
    view[FI_QUERY_TOKEN_MAX_FREQ_MUSIC_WHF] = unigramFeatures.MaxMusicFreqWHF;
    view[FI_QUERY_TOKEN_MAX_FREQ_VIDEO_WHF] = unigramFeatures.MaxVideoFreqWHF;
    view[FI_QUERY_TOKEN_AVG_FREQ_MUSIC_WHF] = unigramFeatures.AvgMusicFreqWHF;
    view[FI_QUERY_TOKEN_AVG_FREQ_VIDEO_WHF] = unigramFeatures.AvgVideoFreqWHF;
    view[FI_QUERY_TOKEN_MAX_FREQ_VIDEO_SELECT_FROM_GALLERY] = unigramFeatures.MaxVideoSelectFromGalleryFreq;
    view[FI_QUERY_TOKEN_AVG_FREQ_VIDEO_SELECT_FROM_GALLERY] = unigramFeatures.AvgVideoSelectFromGalleryFreq;
    view[FI_QUERY_TOKEN_MAX_FREQ_GC] = unigramFeatures.MaxGCFreq;
    view[FI_QUERY_TOKEN_AVG_FREQ_GC] = unigramFeatures.AvgGCFreq;
    view[FI_QUERY_TOKEN_MAX_FREQ_SEARCH] = unigramFeatures.MaxSearchFreq;
    view[FI_QUERY_TOKEN_AVG_FREQ_SEARCH] = unigramFeatures.AvgSearchFreq;
    view[FI_QUERY_TOKEN_MAX_FREQ_OTHER] = unigramFeatures.MaxOtherFreq;
    view[FI_QUERY_TOKEN_AVG_FREQ_OTHER] = unigramFeatures.AvgOtherFreq;

    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_MUSIC] = bigramFeatures.MaxMusicFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_VIDEO] = bigramFeatures.MaxVideoFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_MUSIC] = bigramFeatures.AvgMusicFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_VIDEO] = bigramFeatures.AvgVideoFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_MUSIC_WHF] = bigramFeatures.MaxMusicFreqWHF;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_VIDEO_WHF] = bigramFeatures.MaxVideoFreqWHF;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_MUSIC_WHF] = bigramFeatures.AvgMusicFreqWHF;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_VIDEO_WHF] = bigramFeatures.AvgVideoFreqWHF;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_VIDEO_SELECT_FROM_GALLERY] = bigramFeatures.MaxVideoSelectFromGalleryFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_VIDEO_SELECT_FROM_GALLERY] = bigramFeatures.AvgVideoSelectFromGalleryFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_GC] = bigramFeatures.MaxGCFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_GC] = bigramFeatures.AvgGCFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_SEARCH] = bigramFeatures.MaxSearchFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_SEARCH] = bigramFeatures.AvgSearchFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_MAX_FREQ_OTHER] = bigramFeatures.MaxOtherFreq;
    view[FI_QUERY_TOKEN_BIGRAMS_AVG_FREQ_OTHER] = bigramFeatures.AvgOtherFreq;
}

} // namespace NAlice
