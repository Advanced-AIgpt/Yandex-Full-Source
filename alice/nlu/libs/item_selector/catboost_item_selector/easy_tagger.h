#pragma once

#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/rnn_tagger/rnn_tagger.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {
namespace NItemSelector{

struct TToken {
    TString Text;
    TString Tag;
};

struct TTaggingAlternative {
    TVector<TToken> Tokens;
    double Probability = 0;
};

using TTaggerResult = TVector<TTaggingAlternative>;

class TEasyTagger {
public:
    TEasyTagger(const NVins::TRnnTagger& rnnTagger,
                const NAlice::TTokenEmbedder& tokenEmbedder,
                const size_t topSize = TOP_SIZE,
                const size_t beamWidth = BEAM_WIDTH)
        : RnnTagger(rnnTagger)
        , TokenEmbedder(tokenEmbedder)
        , TopSize(topSize)
        , BeamWidth(beamWidth)
    {
        RnnTagger.EstablishSession();
    }

    TTaggerResult Tag(const TString& text, ELanguage language = LANG_RUS) const;

private:
    static constexpr size_t TOP_SIZE = 10;
    static constexpr size_t BEAM_WIDTH = 20;
    NVins::TRnnTagger RnnTagger;
    NAlice::TTokenEmbedder TokenEmbedder;
    const size_t TopSize;
    const size_t BeamWidth;
};

} // NItemSelector
} // NAlice
