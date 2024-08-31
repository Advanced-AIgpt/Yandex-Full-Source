#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TClothes : public IAnswer {
public:
    TClothes();

    //void AppendFeedbackOptions(TImageWhatIsThisContext& ctx) const override;

    static TClothes* GetPtr();

    void MakeRequests(TImageWhatIsThisApplyContext& ctx) const override;

    TMaybe<ECaptureMode> GetOpenCaptureMode() const override;

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;

private:
    NSc::TValue MakeClothesCrops(TImageWhatIsThisApplyContext& ctx) const;
    bool AddClothesTabsGallery(TImageWhatIsThisApplyContext& ctx, NSc::TValue& foundCrops) const;
};

}
