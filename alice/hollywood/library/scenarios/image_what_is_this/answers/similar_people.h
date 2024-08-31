#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

    class TSimilarPeopleCommon : public IAnswer {
    public:
        TSimilarPeopleCommon(const TStringBuf answerName, const TStringBuf shortAnswerName,
                             const TStringBuf disableFlag);

        void MakeRequests(TImageWhatIsThisApplyContext& context) const override;
    protected:
        bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
        void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
        bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    };

    class TSimilarPeople : public TSimilarPeopleCommon {
    public:
        TSimilarPeople();

        static TSimilarPeople* GetPtr();
    };

    class TSimilarPeopleFrontal : public TSimilarPeopleCommon {
    public:
        TSimilarPeopleFrontal();

        static TSimilarPeopleFrontal* GetPtr();
    };

}
