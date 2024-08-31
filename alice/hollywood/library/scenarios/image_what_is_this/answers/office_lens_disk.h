#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TOfficeLensDisk: public IAnswer {
public:
    TOfficeLensDisk();

    void MakeRequests(TImageWhatIsThisApplyContext& ctx) const override;
    void CleanUp(TImageWhatIsThisApplyContext& ctx) const override;

    static TOfficeLensDisk* GetPtr();

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;

private:
    bool CanSave(TImageWhatIsThisApplyContext& ctx) const;
};

}
