#include "entity_recognition.h"

namespace NAlice {

namespace {

NItemSelector::TSelectionRequest MakeRequest(
    const TString& text,
    const ELanguage& lang,
    const NItemSelector::TEntityType& entityType,
    const NItemSelector::TSelectorName& selectorName
) {
    NItemSelector::TNonsenseTagging nonsenseTagging;
    return {
        .Phrase = {
            .Text = text,
            .Language = lang
        },
        .NonsenseTagging = {},
        .EntityType = entityType,
        .SelectorName = selectorName,
    };
}

#define LOG(msg, log)               \
    do {                            \
        if (log) {                  \
            *log << msg << '\n';    \
        }                           \
    } while (false)

} // namespace

TSemanticFrame RecognizeEntities(
    const TFrameCandidate& frameCandidate,
    const TVector<TString>& tokens,
    const TFrameDescription& frameDescription,
    const THashMap<TString, TEntityContents>& galleries,
    const NItemSelector::IItemSelector& itemSelector,
    const NItemSelector::TSelectorName& selectorName,
    const ELanguage& lang,
    IOutputStream* log
) {
    TSemanticFrame frame;
    frame.SetName(frameCandidate.Name);
    for (const TRecognizedSlot& recognizedSlot : frameCandidate.SlotValues) {
        const auto* slotDescription = frameDescription.Slots.FindPtr(recognizedSlot.Name);
        const bool shouldKeepVariants = slotDescription ? slotDescription->KeepVariants : false;

        TSemanticFrame::TSlot& slot = *frame.AddSlots();
        slot.SetName(recognizedSlot.Name);
        if (shouldKeepVariants) {
            slot.SetType(TString(NAlice::SLOT_VARIANTS_TYPE));
            slot.SetValue(NAlice::PackVariantsValue(recognizedSlot.Variants));
        } else if (!recognizedSlot.Variants.empty()) {
            slot.SetType(recognizedSlot.Variants[0].Type);
            slot.SetValue(recognizedSlot.Variants[0].Value);
        }

        if (!slotDescription) {
            for (const auto& variant : recognizedSlot.Variants) {
                if (variant.Type.empty()) {
                    slot.AddAcceptedTypes(variant.Type);
                }
            }

            LOG("No config entry for slot '" << slot.GetName() << "' of frame '" << frame.GetName() << "'", log);
            continue;
        }

        for (const TString& acceptedType : slotDescription->Types) {
            slot.AddAcceptedTypes(acceptedType);
        }

        if (slot.GetType() == NAlice::SLOT_VARIANTS_TYPE) {
            continue;
        }

        for (const TString& acceptedType : slotDescription->Types) {
            if (slot.GetType() == acceptedType) {
                break;
            }
            if (const TEntityContents* contents = galleries.FindPtr(acceptedType)) {
                if (contents->Items.empty()) {
                    continue;
                }

                const NItemSelector::TSelectionRequest request = MakeRequest(
                    GetSlotValue(tokens, recognizedSlot.Begin, recognizedSlot.End), lang, acceptedType, selectorName);

                const TVector<NItemSelector::TSelectionResult> selectionResult = itemSelector.Select(request, contents->Items);

                TMaybe<size_t> bestIndex;
                for (size_t index = 0; index < selectionResult.size(); ++index) {
                    if (selectionResult[index].IsSelected &&
                        (!bestIndex.Defined() || selectionResult[*bestIndex].Score < selectionResult[index].Score)
                    ) {
                        bestIndex = index;
                    }
                }

                if (bestIndex.Defined()) {
                    slot.SetType(acceptedType);
                    slot.SetValue(contents->Aliases[*bestIndex]);
                    break;
                }
            }
        }
    }
    return frame;
}

} // namespace NAlice
