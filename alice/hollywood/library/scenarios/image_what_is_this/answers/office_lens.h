#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TOfficeLens : public IAnswer {
public:
    TOfficeLens();

    void MakeRequests(TImageWhatIsThisApplyContext& ctx) const override;
    void CleanUp(TImageWhatIsThisApplyContext& ctx) const override;

    static TOfficeLens* GetPtr();

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;
};

}
