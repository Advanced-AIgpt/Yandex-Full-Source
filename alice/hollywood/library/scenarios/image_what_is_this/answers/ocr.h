#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>

namespace NAlice::NHollywood::NImage::NAnswers {

enum EOcrResultCategory {
    ORC_NONE    = 0 /* "None" */,
    ORC_ANY     = 1 /* "Any" */,
    ORC_VAGUE   = 2 /* "Vague" */,
    ORC_CERTAIN = 3 /* "Certain" */,
};


class TOcr : public IAnswer {
public:
    TOcr();

    static TOcr* GetPtr();

    bool AddOcrAnswer(TImageWhatIsThisApplyContext& ctx, const EOcrResultCategory minToSuggest, bool force = false) const;
    EOcrResultCategory GetOcrResultCategory(const TImageWhatIsThisApplyContext& ctx) const;

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;

private:
    NSc::TValue GetContactsOCR(TImageWhatIsThisApplyContext& ctx) const;
    bool AddContactsCard(NSc::TValue card, TImageWhatIsThisApplyContext& ctx) const;
};

}
