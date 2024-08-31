#pragma once
#include <web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents/intents.h>

#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>

#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage {

namespace NAnswers {

class IAnswer {
public:
    IAnswer(TStringBuf answerName, TStringBuf shortAnswerName, TStringBuf disableFlag);

    TStringBuf GetAnswerName() const;
    TMaybe<ECaptureMode> GetCaptureMode() const;
    bool GetIsFrontalCaptureMode() const;
    bool GetIsRepeatable() const;
    TStringBuf GetDisableFlag() const;
    TMaybe<NImages::NCbir::ECbirIntents> GetIntent() const;

    bool IsSuitable(TImageWhatIsThisApplyContext& ctx, bool force) const;
    bool IsSuggestible(TImageWhatIsThisApplyContext& ctx) const;
    bool GetIsSwitchableTo() const;
    bool GetIsSupportSmartCamera() const;
    bool GetIsForceable() const;
    virtual void Compose(TImageWhatIsThisApplyContext& ctx) const;
    virtual void CleanUp(TImageWhatIsThisApplyContext& ctx) const;

    virtual void MakeRequests(TImageWhatIsThisApplyContext& context) const;
    virtual void AppendFeedbackOptions(TImageWhatIsThisApplyContext& ctx) const;
    void RenderError(TImageWhatIsThisApplyContext& ctx) const;
    virtual TStringBuf GetTrueAnswerName(TImageWhatIsThisApplyContext& ctx) const;
    virtual TStringBuf GetShortAnswerName(TImageWhatIsThisApplyContext& ctx) const;
    virtual TMaybe<ECaptureMode> GetOpenCaptureMode() const;

    TStringBuf GetAliceMode() const;
    virtual TMaybe<TStringBuf> GetAliceSmartMode(const TImageWhatIsThisRunContext& ctx) const;
    TMaybe<TStringBuf> GetAliceSmartMode(const TImageWhatIsThisApplyContext& ctx) const;


protected:
    TStringBuf AnswerName;
    TStringBuf ShortAnswerName;
    TStringBuf DisableFlag;
    TMaybe<NImages::NCbir::ECbirIntents> Intent = Nothing();
    bool IsFrontalCaptureMode = false;
    TMaybe<TStringBuf> IntentButtonIcon = Nothing();
    bool IsRepeatable = false;
    bool IsSwitchableTo = true;
    bool IsSupportSmartCamera = false;
    bool IsForceable = true;

    TMaybe<ECaptureMode> CaptureMode = Nothing();
    TStringBuf AliceMode = "photo";
    TMaybe<TStringBuf> AliceSmartMode = Nothing();

    TVector<NImages::NCbir::ECbirIntents> FirstForceAlternativeSuggest;
    TSet<NImages::NCbir::ECbirIntents> AllowedIntents;
    TVector<NImages::NCbir::ECbirIntents> LastForceAlternativeSuggest;

protected:
    virtual bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const = 0;
    virtual bool IsSuggestibleAnswer(TImageWhatIsThisApplyContext& ctx) const;
    virtual void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const = 0;
    virtual NSc::TValue GetAnswerSwitchingDescriptor(TImageWhatIsThisApplyContext& ctx) const;
    virtual NSc::TValue GetSwitchSuggestData(TImageWhatIsThisApplyContext& ctx) const;
    virtual bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const;

private:
    bool DisabledByFlag(TImageWhatIsThisApplyContext& ctx) const;
    void AttachAlternativeIntentsSuggest(TImageWhatIsThisApplyContext& ctx) const;
    void AddIntentButtonsAndSuggests(TImageWhatIsThisApplyContext& ctx) const;
};

}

}
