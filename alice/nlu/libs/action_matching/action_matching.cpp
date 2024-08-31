#include "action_matching.h"

#include <alice/nlu/libs/item_selector/default/default.h>
#include <alice/nlu/libs/item_selector/relevance_based/lstm_relevance_computer.h>
#include <alice/nlu/libs/item_selector/relevance_based/relevance_based.h>

#include <library/cpp/json/json_reader.h>

#include <util/charset/utf8.h>
#include <util/string/cast.h>
#include <util/generic/hash.h>

namespace NAlice {
namespace {

TSemanticFrame MakeFrame(const TString& name) {
    TSemanticFrame frame;
    frame.SetName(name);
    return frame;
}

TVector<NItemSelector::TSelectionItem> ConvertHintsToSelectionItems(const TVector<TActionHint>& hints) {
    TVector<NItemSelector::TSelectionItem> items;
    for (const TActionHint& hint : hints) {
        items.push_back({
            .Synonyms = hint.Phrases,
            .Negatives = hint.Negatives
        });
    }
    return items;
}

TMaybe<TActionHint> TryMatchByPhrases(const NItemSelector::TSelectionRequest& request, const TVector<TActionHint>& hints,
                                         const NItemSelector::IItemSelector* itemSelector) {
    const TVector<NItemSelector::TSelectionItem> items = ConvertHintsToSelectionItems(hints);

    const auto selection = itemSelector->Select(request, items);

    for (size_t i = 0; i < selection.size(); ++i) {
        if (selection[i].IsSelected) {
            return hints[i];
        }
    }
    return Nothing();
}

bool MatchesNegativePhrase(const NItemSelector::TSelectionRequest& request, const TActionHint& hint) {
    const TString utterance = ToLowerUTF8(request.Phrase.Text);
    for (const NItemSelector::TPhrase& negative : hint.Negatives) {
        if (utterance == ToLowerUTF8(negative.Text)) {
            return true;
        }
    }
    return false;
}

TVector<TString> ReadListOfStrings(IInputStream* input) {
    const NJson::TJsonValue actionsJson = NJson::ReadJsonTree(input, true);
    TVector<TString> actions;
    for (const auto& action : actionsJson.GetArray()) {
        actions.push_back(action.GetString());
    }
    return actions;
}

} // namespace anonymous

bool TPermitter::Check(const NItemSelector::TSelectionRequest& request, const TVector<TActionHint>& hints) const {
    if (Languages && !Languages->contains(request.Phrase.Language)) {
        return false;
    }
    if (!FrameNames) {
        return true;
    }
    for (const TActionHint& hint : hints) {
        if(!FrameNames->contains(hint.RecognizedFrameName)){
            return false;
        }
    }
    return true;
}

TVector<NAlice::TPermittedSelector> LoadSelectors(
        const TBoltalkaDssmEmbedder* dssm, IInputStream* protobufModel, IInputStream* modelDescription,
        const NAlice::TTokenEmbedder* embedder, IInputStream* specialEmbeddings, IInputStream* actionsStream) {
    TVector<NAlice::TPermittedSelector> selectors;
    selectors.push_back({
        .Selector = MakeHolder<NItemSelector::TDefaultItemSelector>(
            nullptr,
            ELanguage::LANG_UNK,
            Nothing()
        ),
        .Permitter = {
            /* frameNames = */ Nothing(),
            /* languages = */ Nothing()
        },
        .Priority = 1
    });
    selectors.push_back({
        .Selector = MakeHolder<NAlice::NItemSelector::TDefaultItemSelector>(
            dssm,
            ELanguage::LANG_RUS,
            NItemSelector::LoadIDFs()
        ),
        .Permitter = {
            /* frameNames = */ Nothing(),
            /* languages = */ MakeMaybe<THashSet<ELanguage>>({ELanguage::LANG_RUS})
        },
        .Priority = 10
    });
    TVector<TString> supportedActions = ReadListOfStrings(actionsStream);
    selectors.push_back({
        .Selector = MakeHolder<NItemSelector::TRelevanceBasedItemSelector>(
            /* computer = */ MakeHolder<NItemSelector::TLSTMRelevanceComputer>(
                MakeHolder<NItemSelector::TNnComputer>(protobufModel, modelDescription),
                embedder,
                NItemSelector::ReadSpecialEmbeddings(specialEmbeddings),
                /* maxTokens = */ 128
            ),
            /* threshold = */ 0.5,
            /* filterItemsByRequestLanguage = */ false
        ),
        .Permitter = {
            /* frameNames = */ MakeMaybe<THashSet<TString>>(supportedActions.begin(), supportedActions.end()),
            /* languages = */ MakeMaybe<THashSet<ELanguage>>({ELanguage::LANG_RUS})
        },
        .Priority = 20
    });

    return selectors;
}

TMaybe<TSemanticFrame> RecognizeAction(const NItemSelector::TSelectionRequest& request, const TVector<TSemanticFrame>& frames,
                                       const TVector<TActionHint>& hints, const NItemSelector::IItemSelector* itemSelector) {
    if (const auto actions = RecognizeActions(request, frames, hints, itemSelector); !actions.empty()) {
        return actions[0];
    }
    return Nothing();
}

TMaybe<TSemanticFrame> RecognizeAction(const NItemSelector::TSelectionRequest& request, const TVector<TSemanticFrame>& frames,
                                       const TVector<TActionHint>& hints, const TVector<TPermittedSelector>& selectors) {
    if (const auto actions = RecognizeActions(request, frames, hints, selectors); !actions.empty()) {
        return actions[0];
    }
    return Nothing();
}

TVector<TSemanticFrame> RecognizeActions(const NItemSelector::TSelectionRequest& request,
                                         const TVector<TSemanticFrame>& frames, const TVector<TActionHint>& hints,
                                         const NItemSelector::IItemSelector* itemSelector) {
    THashMap<TString, TSemanticFrame> nameToFrame;
    for (const auto& frame : frames) {
        nameToFrame[frame.GetName()] = frame;
    }
    TVector<TSemanticFrame> recognizedFrames;

    if (const auto hint = TryMatchByPhrases(request, hints, itemSelector); hint.Defined()) {
        const auto& frameName = hint->RecognizedFrameName;
        if (const auto* frame = nameToFrame.FindPtr(frameName)) {
            recognizedFrames.push_back(*frame);
            nameToFrame.erase(frameName);
        } else {
            recognizedFrames.push_back(MakeFrame(frameName));
        }
    }

    for (const auto& hint : hints) {
        const auto& frameName = hint.RecognizedFrameName;
        if (const TSemanticFrame* frame = nameToFrame.FindPtr(frameName);
            frame && !MatchesNegativePhrase(request, hint)
        ) {
            recognizedFrames.push_back(*frame);
            nameToFrame.erase(frameName);
        }
    }

    return recognizedFrames;
}

TVector<TSemanticFrame> RecognizeActions(const NItemSelector::TSelectionRequest& request,
                                         const TVector<TSemanticFrame>& frames, const TVector<TActionHint>& hints,
                                         const TVector<TPermittedSelector>& selectors) {
    int winningPriority = 0;
    const NItemSelector::IItemSelector* winningSelector = nullptr;
    for (const TPermittedSelector& selector : selectors) {
        if (selector.Priority > winningPriority && selector.Permitter.Check(request, hints)) {
            winningPriority = selector.Priority;
            winningSelector = selector.Selector.Get();
        }
    }
    return RecognizeActions(request, frames, hints, winningSelector);
}

} // namespace NAlice
