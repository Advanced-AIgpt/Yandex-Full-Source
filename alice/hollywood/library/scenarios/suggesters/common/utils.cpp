#include "utils.h"

#include <alice/hollywood/library/frame/callback.h>

#include <alice/library/video_common/defs.h>

namespace NAlice::NHollywood {

namespace {

const THashSet<TString> CARTOON_GENRES = {
    ToString(NVideoCommon::EVideoGenre::Anime),
    ToString(NVideoCommon::EVideoGenre::Childrens)
};

} // namespace

TMaybe<TStringBuf> TryGetFlagValue(const TStringBuf flag, const TStringBuf key) {
    TStringBuf flagValue;
    if (flag.AfterPrefix(key, flagValue)) {
        return flagValue;
    }
    return Nothing();
}

TSemanticFrame InitCallbackFrameEffect(const TSemanticFrame& semanticFrame, const TString& frameName,
                                       const THashSet<TString>& copySlots)
{
    TSemanticFrame frame;
    frame.SetName(frameName);

    for (const auto& slot : semanticFrame.GetSlots()) {
        if (copySlots.contains(slot.GetName())) {
            *frame.AddSlots() = slot;
        }
    }

    return frame;
}

TMaybe<TString> GetContentTypeByGenre(const TMaybe<TString>& genre) {
    if (!genre) {
        return Nothing();
    }
    if (CARTOON_GENRES.contains(*genre)) {
        return ToString(NVideoCommon::EContentType::Cartoon);
    }
    return ToString(NVideoCommon::EContentType::Movie);
}

void AddTypeTextSuggest(const TString& text, const TString& actionId, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::TFrameAction action;
    action.MutableNluHint()->SetFrameName(actionId);
    action.MutableDirectives()->AddList()->MutableTypeTextDirective()->SetText(text);

    bodyBuilder.AddAction(actionId, std::move(action));
    bodyBuilder.AddActionSuggest(actionId).Title(text);
}

} // namespace NAlice::NHollywood
