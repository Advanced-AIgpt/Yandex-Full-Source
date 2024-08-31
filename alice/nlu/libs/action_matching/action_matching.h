#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/nlu/libs/binary_classifier/boltalka_dssm_embedder.h>
#include <alice/nlu/libs/embedder/embedder.h>
#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <search/begemot/rules/granet/proto/granet.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

struct TActionHint {
    TVector<NItemSelector::TPhrase> Phrases;
    TVector<NItemSelector::TPhrase> Negatives;
    TString RecognizedFrameName;
};

class TPermitter {
 public:
    TPermitter(const TMaybe<THashSet<TString>>& frameNames, const TMaybe<THashSet<ELanguage>>& languages)
        : FrameNames(frameNames)
        , Languages(languages)
    {
    }

    bool Check(const NItemSelector::TSelectionRequest& request, const TVector<TActionHint>& hints) const;

 private:
    const TMaybe<THashSet<TString>> FrameNames;
    const TMaybe<THashSet<ELanguage>> Languages;
};

struct TPermittedSelector {
    THolder<NItemSelector::IItemSelector> Selector;
    TPermitter Permitter;
    const int Priority = 0;
};

TVector<NAlice::TPermittedSelector> LoadSelectors(
    const TBoltalkaDssmEmbedder* dssm, IInputStream* protobufModel, IInputStream* modelDescription,
    const NAlice::TTokenEmbedder* embedder, IInputStream* specialEmbeddings, IInputStream* actionsStream
);

TMaybe<TSemanticFrame> RecognizeAction(const NItemSelector::TSelectionRequest& request, const TVector<TSemanticFrame>& frames,
                                       const TVector<TActionHint>& hints, const NItemSelector::IItemSelector* itemSelector);

TMaybe<TSemanticFrame> RecognizeAction(const NItemSelector::TSelectionRequest& request, const TVector<TSemanticFrame>& frames,
                                       const TVector<TActionHint>& hints, const TVector<TPermittedSelector>& selectors);

TVector<TSemanticFrame> RecognizeActions(const NItemSelector::TSelectionRequest& request,
                                         const TVector<TSemanticFrame>& frames, const TVector<TActionHint>& hints,
                                         const NItemSelector::IItemSelector* itemSelector);

TVector<TSemanticFrame> RecognizeActions(const NItemSelector::TSelectionRequest& request,
                                         const TVector<TSemanticFrame>& frames, const TVector<TActionHint>& hints,
                                         const TVector<TPermittedSelector>& selectors);

} // namespace NAlice
