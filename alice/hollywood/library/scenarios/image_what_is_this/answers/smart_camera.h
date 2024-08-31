#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

    class TSmartCamera : public IAnswer {
    public:
        TSmartCamera();

        static TSmartCamera* GetPtr();

    protected:
        bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
        void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    };

}
