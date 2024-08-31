#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

    class THomeWork : public IAnswer {
    public:
        THomeWork();

        static THomeWork* GetPtr();

        TMaybe<TStringBuf> GetAliceSmartMode(const TImageWhatIsThisRunContext& ctx) const override;

    protected:
        bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
        void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    };

}
