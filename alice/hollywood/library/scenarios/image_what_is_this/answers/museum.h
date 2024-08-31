#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

    class TMuseum : public IAnswer {
    public:
        TMuseum();

        static TMuseum* GetPtr();

    protected:
        static constexpr TStringBuf PROTMO_PATH = "/Promo/museum";
        static constexpr std::pair<int, int> IMAGE_SIZE = std::make_pair(120, 120);

        bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
        void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    };

}
