#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TCommon : public IAnswer {
public:
    TCommon();

    void MakeRequests(TImageWhatIsThisApplyContext& ctx) const override;
    void Compose(TImageWhatIsThisApplyContext& ctx) const override;

    static TCommon* GetPtr();

    TStringBuf GetTrueAnswerName(TImageWhatIsThisApplyContext& ctx) const override;
    TStringBuf GetShortAnswerName(TImageWhatIsThisApplyContext& ctx) const override;
protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;

private:
    IAnswer* FindBestIntent(TImageWhatIsThisApplyContext& ctx) const;
};

}
