#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

    class TSimilarArtwork : public IAnswer {
    public:
        TSimilarArtwork();

        static TSimilarArtwork* GetPtr();

        void MakeRequests(TImageWhatIsThisApplyContext& context) const override;
    protected:
        bool IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const override;
        bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
        void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
        bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    };

}
