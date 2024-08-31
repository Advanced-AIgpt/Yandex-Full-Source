#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TEntity : public IAnswer {
public:
    TEntity();

    void AppendFeedbackOptions(TImageWhatIsThisApplyContext& ctx) const override;

    static TEntity* GetPtr();

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
private:
    bool IsValidEntitySearchData(const NSc::TValue& cvData) const;
    void PutEntitySearchResults(TImageWhatIsThisApplyContext& ctx, bool fillOnlySlot, bool &multipleObjectsGenerated) const;
    NSc::TValue CreateEntitySearchObject(const TImageWhatIsThisApplyContext& ctx, const NSc::TValue& entityResults) const;
};

}
