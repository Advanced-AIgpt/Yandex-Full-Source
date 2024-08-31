#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TOcrVoice : public IAnswer {
public:
    TOcrVoice();

    void MakeRequests(TImageWhatIsThisApplyContext& ctx) const override;

    static TOcrVoice* GetPtr();

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    bool IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
private:
    bool AddOcrVoiceAnswer(TImageWhatIsThisApplyContext& ctx, bool silentMode) const;
    void DecreaseSentencesLength(TString& text) const;
    TString CutTexts(const NSc::TArray& texts, size_t len, bool appendEllipsis) const;
    bool ReplaceAsterisks(NSc::TArray &texts) const;
    bool FixSwear(const NSc::TArray& texts, NSc::TValue& result, const TImageWhatIsThisApplyContext& ctx) const;
    bool IsContainsEnoughLongWords(const NSc::TArray& texts, size_t wordLength,
                                   int enoughWords, const TString& delimeters) const;
    bool IsObscene(const TString& word, const TImageWhatIsThisApplyContext& ctx) const;
    void AddButtons(TImageWhatIsThisApplyContext& ctx) const;
};

}
