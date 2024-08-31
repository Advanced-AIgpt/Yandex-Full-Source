#pragma once

#include <alice/nlu/libs/item_selector/interface/item_selector.h>
#include <alice/nlu/libs/binary_classifier/boltalka_dssm_embedder.h>

#include <alice/library/parsed_user_phrase/stopwords.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/maybe.h>
#include <util/stream/input.h>

namespace NAlice {
namespace NItemSelector {

NParsedUserPhrase::TStopWordsHolder LoadIDFs(IInputStream& stream);

NParsedUserPhrase::TStopWordsHolder LoadIDFs();

class TDefaultItemSelector : public IItemSelector {
public:
    TDefaultItemSelector(const NAlice::TBoltalkaDssmEmbedder* embedder, const ELanguage language,
                         TMaybe<NParsedUserPhrase::TStopWordsHolder> idfs)
        : Embedder(embedder)
        , Language(language)
        , IDFs(idfs)
    {
    }

    TVector<TSelectionResult> Select(const TSelectionRequest& request, const TVector<TSelectionItem>& items) const;

private:
    const NAlice::TBoltalkaDssmEmbedder* Embedder;
    const ELanguage Language;
    const TMaybe<NParsedUserPhrase::TStopWordsHolder> IDFs;
};

} // namespace NItemSelector
} // namespace NAlice
