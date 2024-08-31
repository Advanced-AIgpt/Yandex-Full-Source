#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>

namespace NAlice::NHollywood::NImage {

namespace NAnswers {

class TSimilarLike : public IAnswer {
public:
    TSimilarLike();

    static TSimilarLike* GetPtr();

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    bool IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
};

}

}
