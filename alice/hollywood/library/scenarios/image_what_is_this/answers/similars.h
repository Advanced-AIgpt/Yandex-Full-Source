#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>

namespace NAlice::NHollywood::NImage {

namespace NAnswers {

class TSimilars : public IAnswer {
public:
    TSimilars();
    //void AppendFeedbackOptions(TImageWhatIsThisApplyContext& ctx) const override;

    static TSimilars* GetPtr();

    bool AddSimilarsGallery(TImageWhatIsThisApplyContext& ctx) const;
    bool CheckForSimilarAnswer(TImageWhatIsThisApplyContext& ctx) const;

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    NSc::TValue GetSwitchSuggestData(TImageWhatIsThisApplyContext& ctx) const override;
};

}

}
