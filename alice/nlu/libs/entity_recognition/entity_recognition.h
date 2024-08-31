#pragma once

#include <alice/library/frame/description.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/nlu/libs/frame/slot.h>
#include <alice/nlu/libs/item_selector/interface/item_selector.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NAlice {

struct TEntityContents {
    TVector<NAlice::NItemSelector::TSelectionItem> Items;
    TVector<TString> Aliases;
};

struct TFrameCandidate {
    TString Name;
    TVector<TRecognizedSlot> SlotValues;
};

TSemanticFrame RecognizeEntities(
    const TFrameCandidate& frameCandidate,
    const TVector<TString>& tokens,
    const TFrameDescription& frameDescription,
    const THashMap<TString, TEntityContents>& galleries,
    const NItemSelector::IItemSelector& itemSelector,
    const NItemSelector::TSelectorName& selectorName,
    const ELanguage& lang,
    IOutputStream* log = nullptr
);

} // namespace NAlice
