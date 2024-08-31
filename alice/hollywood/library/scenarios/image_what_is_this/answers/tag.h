#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents/intents.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TTag : public IAnswer {
public:
    TTag();

    bool HasTagAnswer(TImageWhatIsThisApplyContext& ctx) const;

    bool CheckForTagAnswer(TImageWhatIsThisApplyContext& ctx) const;
    NImages::NCbir::ETagConfidenceCategory GetTagConfidenceCategory(TImageWhatIsThisApplyContext& ctx) const;

    bool AddTagAnswer(TImageWhatIsThisApplyContext& ctx, const TString& cardName = "render_tag") const;
    void PutTag(const NSc::TValue& tag, NImages::NCbir::ETagConfidenceCategory confidence, TImageWhatIsThisApplyContext& ctx, const TString& cardName) const;

    static TTag* GetPtr();

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    NSc::TValue GetSwitchSuggestData(TImageWhatIsThisApplyContext& ctx) const override;
};

}
