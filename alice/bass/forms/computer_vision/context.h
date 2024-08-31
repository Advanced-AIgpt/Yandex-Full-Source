#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/common/personal_data.h>
#include <web/src_setup/lib/setup/images_cbir_postprocess/intents_classifier/intents/intents.h>

namespace NBASS {

namespace NComputerVisionForms {

extern const TStringBuf IMAGE_WHAT_IS_THIS;
extern const TStringBuf IMAGE_WHAT_IS_THIS_CLOTHES_BOXES;
extern const TStringBuf IMAGE_WHAT_IS_THIS_NEGATIVE_FEEDBACK;
extern const TStringBuf IMAGE_WHAT_IS_THIS_OCR_VOICE_SUGGEST;
extern const TStringBuf ONBOARDING_IMAGE_SEARCH;

} // NComputerVisionForms

namespace NComputerVisionFlags {

extern const TStringBuf ENABLE_COMMON_TAGS;
extern const TStringBuf DISABLE_AUTO_OCR;
extern const TStringBuf ENABLE_OFFICE_LENS;
extern const TStringBuf ENABLE_PEOPLE;
extern const TStringBuf FORCE_SIMILAR_AS_SECOND_ANSWER;

extern const TStringBuf CV_EXP_MARKET_CARD_V3_ALICE;
extern const TStringBuf CV_EXP_MARKET_CARD_V3_MARKET;
extern const TStringBuf CV_EXP_MARKET_CARD_V3_EMPTY;
extern const TStringBuf CV_EXP_MARKET_CARD_DOUBLE_BEST_CARDS;

extern const TStringBuf CV_EXP_OFFICE_LENS_CROP;
extern const TStringBuf CV_EXP_OFFICE_LENS_BUTTONS;
} // NComputerVisionFlags

namespace NComputerVisionFeedbackOptions {

extern const TStringBuf BARCODE_WRONG;
extern const TStringBuf BARCODE_UNWANTED;
extern const TStringBuf CLOTHES_UNWANTED;
extern const TStringBuf MARKET_LINK;
extern const TStringBuf MARKET_POOR;
extern const TStringBuf MARKET_UNWANTED;
extern const TStringBuf OCR_POOR;
extern const TStringBuf OCR_TRANSLATE;
extern const TStringBuf OCR_UNWANTED;
extern const TStringBuf TAG_WRONG;
extern const TStringBuf UNSIMILAR;
extern const TStringBuf USELESS;
extern const TStringBuf OFFENSIVE_ANSWER;
extern const TStringBuf OTHER;

} // NComputerVisionFeedbackOptions

enum EOcrResultCategory {
    ORC_NONE    = 0 /* "None" */,
    ORC_ANY     = 1 /* "Any" */,
    ORC_VAGUE   = 2 /* "Vague" */,
    ORC_CERTAIN = 3 /* "Certain" */,
};

enum class ECaptureMode {
    OCR_VOICE   = 0             /* "voice_text" */,
    OCR         = 1             /* "text" */,
    PHOTO       = 2             /* "photo" */,
    MARKET      = 3             /* "market" */,
    DOCUMENT    = 4             /* "document" */,
    CLOTHES     = 5             /* "clothes" */,
    DETAILS     = 6             /* "details" */,
    SIMILAR_LIKE = 7            /* "similar_like" */,
    SIMILAR_PEOPLE = 8          /* "similar_people" */,
    SIMILAR_PEOPLE_FRONTAL = 9  /* "similar_people_frontal" */,
    BARCODE                = 10 /* "barcode" */,
    TRANSLATE              = 11 /* "translate" */,
    SIMILAR_ARTWORK        = 12 /* "similar_artwork" */,
    UNDEFINED,
};

enum class EContactType {
    CT_MAIL     = 0 /* "email" */,
    CT_PHONE    = 1 /* "phone" */,
    CT_URL      = 2 /* "url" */,
};

class IComputerVisionBaseHandler;
class IComputerVisionAnswer;

using TAnswersList = const IComputerVisionAnswer*[];
using TIntentToAnswerMap = TMap<NImages::NCbir::ECbirIntents, const IComputerVisionAnswer&>;
using TFlagAnswerPairs = TVector<std::pair<TStringBuf, const IComputerVisionAnswer&>>;
using TCaptureModeToAnswerMap = TMap<ECaptureMode, const IComputerVisionAnswer&>;

class TComputerVisionContext: private NNonCopyable::TNonCopyable {
public:
    TComputerVisionContext(const IComputerVisionBaseHandler& handler, TContext& ctx, const TInstant& deadline);
    ~TComputerVisionContext() = default;

    void Switch(const TStringBuf formName);
    bool ShouldChangeImage(ECaptureMode captureMode);
    bool PrepareComputerVisionData(ECaptureMode captureMode, bool frontalCamera, bool needOcrData, bool needOfficeLensData,
                                   const TStringBuf additionalFlag);
    bool PrepareComputerVisionOcrData(ECaptureMode captureMode, const TStringBuf apphostGraphApiNumber);
    bool CheckComputerVisionResp(NHttpFetcher::TResponse::TRef resp, NSc::TValue& result);
    void OutputResult();

    bool HasTagAnswer() const;
    bool CheckForTagAnswer() const;
    NImages::NCbir::ETagConfidenceCategory GetTagConfidenceCategory() const;
    bool CheckForSimilarAnswer() const;
    bool CheckForMarketAnswer(bool allowAny) const;
    bool CheckMarketItem(const NSc::TValue& marketItem) const;
    EOcrResultCategory GetOcrResultCategory() const;

    TResultValue RequestClothesEllipsis();
    void AddClothesEllipsisAnswer();
    bool AddContactsCard(NSc::TValue card);
    void AppendContact(const NSc::TValue& value, const EContactType type, NSc::TArray& contacts) const;
    NSc::TValue GetContactsOCR() const;
    bool AddOcrVoiceAnswer(const THashSet<TUtf16String>& swearWords);
    void AddOfficeLensAnswer();
    bool AddTagAnswer();
    void PutTag(const NSc::TValue& tag, NImages::NCbir::ETagConfidenceCategory);
    bool AddOcrAnswer(const EOcrResultCategory minToSuggest, bool force = false);
    bool AddClothesTabsGallery(NSc::TValue foundCrops);
    bool AddSimilarsGallery();
    void AddMarketGallery(bool ignoreFirst = false);
    void FillMarketStars(NSc::TValue& galleryDivCard) const;
    void FillMarketItem(const NSc::TValue& item, NSc::TValue& marketItem, bool isOrigImage, bool imageThumb, const TString& cardType) const;
    void FillMarketStars();
    void AppendFeedbackOption(const TStringBuf optionName);
    void AttachAlternativeIntentsSuggest(const TSet<NImages::NCbir::ECbirIntents>& allowedIntents,
                                         const TVector<NImages::NCbir::ECbirIntents>& forcedIntents,
                                         const TVector<NImages::NCbir::ECbirIntents>& firstForcedIntens = TVector<NImages::NCbir::ECbirIntents>());
    void AttachSimilarSearchSuggest(bool showButton = true);
    void AttachSearchSuggest();
    void AttachFeedbackSuggest(const TStringBuf feedbackType);
    void AttachUtteranceSuggest(const TStringBuf text);
    void AttachCustomUriSuggest(const TStringBuf suggestType, const TStringBuf uri, const TStringBuf label = TStringBuf());
    void AttachUpdateFormSuggest(const TStringBuf suggestType, const TStringBuf formName, const TStringBuf label = TStringBuf());
    TString AttachOcrSuggest(const TStringBuf label = TStringBuf());
    void RedirectTo(const TStringBuf aliceSource, const TStringBuf redirect, bool disablePtr = false);

    NJson::TJsonValue RequestDiskInfo(const TString& path) const;
    bool RequestDiskCreateDir(const TString& path) const;
    bool RequestDiskPutFile(const TString& path, const TStringBuf imageUrl) const;

    inline void AttachTextCard() {
        HandlerContext().AddTextCardBlock(TStringBuf("render_text_card_result"));
    }
    TString GenerateImagesSearchUrl(const TStringBuf aliceSource,
                                    const TStringBuf report = "imageview", bool disablePtr = false, const TStringBuf cbirPage = "") const;
    TString GenerateMarketDealsLink(int cropId) const;

    bool HasExpFlag(const TStringBuf name) const {
        return Context.HasExpFlag(name);
    }

    TContext& HandlerContext() {
        return (SwitchedContext ? *SwitchedContext : Context);
    }

    const TContext& HandlerContext() const {
        return (SwitchedContext ? *SwitchedContext : Context);
    }

    NSc::TValue& Output(const TStringBuf field) {
        return Result[field];
    }

    const NSc::TValue& Output(const TStringBuf field) const {
        return Result[field];
    }

    bool HasCbirId() const {
        return !CbirId.empty();
    }

    const NSc::TValue& GetAnswerField(const TStringBuf name) const {
        const TContext::TSlot* slotAnswer = HandlerContext().GetSlot("answer");
        return (nullptr != slotAnswer && !slotAnswer->Value.IsNull())
               ? slotAnswer->Value[name]
               : NSc::Null();
    }

    bool AnswerHas(const TStringBuf field) const {
        return !GetAnswerField(field).IsNull();
    }

    const TString& GetImageUrl() const {
        return ImageUrl;
    }

    ECaptureMode GetCaptureMode();
    void FillCaptureMode(ECaptureMode captureMode, NSc::TValue& payload);

    const NSc::TValue& GetData() const {
        return VisionData;
    }

    const NSc::TValue& GetOcrData() const {
        return OcrData;
    }

    const TVector<const IComputerVisionAnswer*> GetPrioritizedAnswers() const {
        TVector<const IComputerVisionAnswer*> result;
        for (const auto& intent : CbirIntents) {
            if (AnswerAlternatives.contains(intent)) {
                result.emplace_back(&AnswerAlternatives.at(intent));
            }
        }
        return result;
    }

    bool IsEntityAnswerSuitable() const;

    TMaybe<TStringBuf> GetForceAnswerSlot() const;
    TMaybe<TStringBuf> GetSubCaptureModeSlot() const;
    void TryFillLastForceAnswer(ECaptureMode captureMode);

    const TString GetCbirId() const {
        return CbirId;
    }

    NAlice::IRng& GetRng() {
        return Rng;
    }

    void ExtractPersonalData() {
        PersonalDataHelper = MakeHolder<TPersonalDataHelper>(HandlerContext());
    }

    bool ExtractUserTicket() {
        return PersonalDataHelper->GetTVM2UserTicket(UserTicket);
    }

private:
    void SetCbirId() {
        const NSc::TValue& value = VisionData["CbirId"];
        CbirId = value.IsNull() ? TString() : value.ForceString();
    }

    void SetCbirIntents() {
        CbirIntents.clear();
        for (const auto& intent : VisionData["Intents"].GetArray()) {
            const TStringBuf intentName = intent["name"].GetString();
            CbirIntents.emplace_back(FromString<NImages::NCbir::ECbirIntents>(intentName));
            if (CbirIntents.back() == NImages::NCbir::ECbirIntents::CI_OCR) {
                CbirIntents.emplace_back(NImages::NCbir::ECbirIntents::CI_OCR_VOICE);
            }
        }
    }

    NHttpFetcher::THandle::TRef MakeMarketRequest(const NSc::TValue& data, NHttpFetcher::IMultiRequest::TRef& requests) const;
    NHttpFetcher::THandle::TRef MakeSimilarsRequest(const NSc::TValue& data, NHttpFetcher::IMultiRequest::TRef& requests) const;
    bool FetchComputerVisionData(bool needOcrData, bool needOfficeLensData, const TStringBuf additionalFlag);
    NHttpFetcher::THandle::TRef MakeComputerVisionCommonRequest(NHttpFetcher::IMultiRequest::TRef requests,
                                                                const TStringBuf additionalFlag) const;
    NHttpFetcher::THandle::TRef MakeComputerVisionCbirFeaturesRequest(const TStringBuf apphostGraphApiNumber,
                                                                      NHttpFetcher::IMultiRequest::TRef requests,
                                                                      TCgiParameters& cgi) const;
    NSc::TValue ExtractEllipsisInfo() const;
    NSc::TValue MakeMarketGallery(const NSc::TValue& visionData, bool ignoreFirst, bool appendTailLink, int cropId = -1) const;
    void AppendSpecialButtons();
    bool CheckAndSetImageUrl(ECaptureMode captureMode, bool frontalCamera);
    TString CutText(const TString& text, size_t len) const;
    TString CutTexts(const NSc::TArray& texts, size_t len, bool appendEllipsis = false) const;
    TStringBuf CleanUrl(const TStringBuf& url) const;
    bool IsContainsEnoughLongWords(const NSc::TArray& texts, size_t wordLength, int enoughWords, const TString& delimeters);
    bool IsObscene(const TUtf16String& word, const THashSet<TUtf16String>& swearWords);
    bool FixSwear(const NSc::TArray& texts, const THashSet<TUtf16String>& swearWords,
                  NSc::TValue& result);
    bool ReplaceAsterisks(NSc::TArray& texts);
    void DecreaseSentencesLength(TString& text);

private:
    static const TIntentToAnswerMap AnswerAlternatives;

    const IComputerVisionBaseHandler& Handler;
    TContext& Context;
    const TInstant& Deadline;
    TContext::TPtr SwitchedContext;
    NSc::TValue Result;
    NSc::TValue VisionData;
    NSc::TValue OcrData;
    TString ImageUrl;
    TString SourceImageUrl;
    TVector<std::pair<float, float>> CropCoordinates;
    TString CbirId;
    TVector<NImages::NCbir::ECbirIntents> CbirIntents;
    NSc::TValue NegativeFeedbackOptions;
    size_t UsedTagNo = 0;
    TString UsedTagPath;
    NAlice::IRng& Rng;
    THolder<TPersonalDataHelper> PersonalDataHelper;
    TString UserTicket;
};

class IComputerVisionAnswer: private NNonCopyable::TNonCopyable {
protected:
    const TStringBuf SuggestName;
    const TStringBuf IntentButtonIcon;

public:
    IComputerVisionAnswer(const TStringBuf suggestName = TStringBuf(),
                          const TStringBuf buttonIcon = TStringBuf())
        : SuggestName(suggestName)
        , IntentButtonIcon(buttonIcon)
    {
    }

    virtual ~IComputerVisionAnswer() = default;

    virtual void LoadData() {}

    virtual bool TryApplyTo(TComputerVisionContext& ctx, bool force = false, bool shouldAttachAlternativeIntents = true) const {
        if (IsSuitable(ctx, force) && !DisabledByFlag(ctx)) {
            Compose(ctx);
            AttachAlternativeIntentsSuggest(ctx);
            ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::USELESS);
            ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OTHER);
            ctx.TryFillLastForceAnswer(AnswerType());
            return true;
        } else if (force) {
            ctx.TryFillLastForceAnswer(AnswerType());
            if (shouldAttachAlternativeIntents) {
                AttachCannotApplyMessage(ctx);
            }
        }
        return false;
    }

    virtual NSc::TValue GetAnswerSwitchingDescriptor(const TComputerVisionContext& ctx) const {
        NSc::TValue descr;
        if (SuggestName && IntentButtonIcon && IsSuggestible(ctx)) {
            descr["id"] = SuggestName;
            descr["icon"] = IntentButtonIcon;
        }
        return descr;
    }

    virtual bool IsSuitable(const TComputerVisionContext& ctx, bool force) const = 0;

    virtual bool IsSimilarSubstituteAllowed(const TComputerVisionContext& ctx) const {
        return ctx.HasExpFlag(NComputerVisionFlags::FORCE_SIMILAR_AS_SECOND_ANSWER);
    }

    bool DisabledByFlag(TComputerVisionContext& ctx) const {
        const TStringBuf flag = GetDisableFlag();
        return !flag.empty() && ctx.HasExpFlag(flag);
    }

    virtual TStringBuf GetDisableFlag() const {
        return TStringBuf();
    }

    virtual bool IsNeedOcrData() const {
        return false;
    }

    virtual bool IsNeedOfficeLensData() const {
        return false;
    }

    virtual const TStringBuf AdditionalFlag() const {
        return TStringBuf();
    }

    virtual ECaptureMode AnswerType() const {
        return ECaptureMode::PHOTO;
    }

    virtual TStringBuf GetAnswerId() const = 0;

    virtual TStringBuf GetCannotApplyMessage() const {
        return TStringBuf("cannot_apply_common");
    }

    virtual void AttachAlternativeIntentsSuggest(TComputerVisionContext&) const {
    }

    void AttachCannotApplyMessage(TComputerVisionContext& ctx) const {
        ctx.Output(GetCannotApplyMessage()) = true;
        ctx.AttachTextCard();
        AttachAlternativeIntentsSuggest(ctx);
    }

protected:

    virtual bool IsSuggestible(const TComputerVisionContext& ctx) const {
        return IsSuitable(ctx, /* force */ false);
    }

    virtual void Compose(TComputerVisionContext& ctx) const = 0;
};

class TCVAnswerPorn: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("porn"); }

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerGruesome: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("gruesome"); }

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerDark: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("dark"); }

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerEntitySearch: public IComputerVisionAnswer {
public:
    void LoadData() override;
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("entity"); }
    TStringBuf GetDisableFlag() const override;
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;
    ECaptureMode AnswerType() const override {
        return ECaptureMode::MARKET;
    }

private:
    bool IsValidEntitySearchData(const NSc::TValue& cvData) const;
    void PutEntitySearchResults(TComputerVisionContext& ctx, bool fillOnlySlot, bool &multipleObjectsGenerated) const;
    NSc::TValue CreateEntitySearchObject(const TComputerVisionContext& ctx,
                                         const NSc::TValue& entityResults) const;
private:
    NSc::TValue EntitySearchJokes;
};

class TCVAnswerPeople: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("people"); }

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerClothes: public IComputerVisionAnswer {
public:
    TCVAnswerClothes()
        : IComputerVisionAnswer(
            TStringBuf("image_what_is_this__clothes_forced"),
            TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/38061/icon-clothes/orig"))
    {
    }
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("clothes"); }
    TStringBuf GetDisableFlag() const override;
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;
    virtual TStringBuf GetCannotApplyMessage() const override {
        return TStringBuf("cannot_apply_clothes");
    }


protected:
    void Compose(TComputerVisionContext& ctx) const override;
    NSc::TValue MakeClothesCrops(const TComputerVisionContext& ctx) const;
    ECaptureMode AnswerType() const override {
        return ECaptureMode::CLOTHES;
    }
};

class TCVAnswerMarket: public IComputerVisionAnswer {
public:
    TCVAnswerMarket()
        : IComputerVisionAnswer(
            TStringBuf("image_what_is_this__market"),
            TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/15681/icon-market/orig"))
    {
    }
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("market"); }
    TStringBuf GetDisableFlag() const override;
    TStringBuf GetCannotApplyMessage() const override;
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;
    const TStringBuf AdditionalFlag() const override {
        return TStringBuf("force_market");
    }

protected:
    void Compose(TComputerVisionContext& ctx) const override;

    bool IsSuggestible(const TComputerVisionContext& ctx) const override {
        return ctx.CheckForMarketAnswer(/* allowAny */ true);
    }
    ECaptureMode AnswerType() const override {
        return ECaptureMode::MARKET;
    }

private:
    void AddMarketCard(TComputerVisionContext& ctx) const;
};

class TCVAnswerTag: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("tag"); }
    TStringBuf GetDisableFlag() const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerOcr: public IComputerVisionAnswer {
public:
    TCVAnswerOcr()
        : IComputerVisionAnswer(
            TStringBuf("image_what_is_this__ocr"),
            TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/40186/icon-ocr/orig"))
    {
    }
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("ocr"); }
    TStringBuf GetDisableFlag() const override;

    NSc::TValue GetAnswerSwitchingDescriptor(const TComputerVisionContext& ctx) const override {
        auto descr = IComputerVisionAnswer::GetAnswerSwitchingDescriptor(ctx);
        if (!descr.IsNull()) {
            descr["context"]["fast_ocr"].SetString(ToString(EOcrResultCategory::ORC_ANY));
        }
        return descr;
    }

    TStringBuf GetCannotApplyMessage() const override;
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;
    ECaptureMode AnswerType() const override {
        return ECaptureMode::OCR;
    }

private:
    void FillContacts(const NSc::TValue& fastOcr, NSc::TArray& cardContacts, const TClientInfo& clientInfo) const;
};

class TCVAnswerFace: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("face"); }
    TStringBuf GetDisableFlag() const override;
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerSimilarGallery: public IComputerVisionAnswer {
public:
    TCVAnswerSimilarGallery()
        : IComputerVisionAnswer(
            TStringBuf("image_what_is_this__details"),
            TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-what/orig"))
    {
    }
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("similar"); }
    TStringBuf GetDisableFlag() const override;

    NSc::TValue GetAnswerSwitchingDescriptor(const TComputerVisionContext& ctx) const override {
        if (IsSuggestible(ctx) && ctx.GetData()["Faces"].ArraySize() > 0) {
            NSc::TValue descr;
            descr["id"] = SuggestName;
            descr["icon"].SetString("https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-who/orig");
            descr["person"].SetBool(true);
            return descr;
        }
        return IComputerVisionAnswer::GetAnswerSwitchingDescriptor(ctx);
    }

    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;

    bool IsSuggestible(const TComputerVisionContext& ctx) const override {
        return ctx.CheckForTagAnswer();
    }
    ECaptureMode AnswerType() const override {
        return ECaptureMode::DETAILS;
    }
};

class TCVAnswerIziPushkinMuseum: public IComputerVisionAnswer {
public:
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("pushkin"); }
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

class TCVAnswerSimilarPeople: public IComputerVisionAnswer {
public:
    TCVAnswerSimilarPeople()
        : IComputerVisionAnswer(
            TStringBuf("image_what_is_this__similar_people"),
            TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/13615/icon-similar_people/orig"))
    {
    }
    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
    bool IsSuggestible(const TComputerVisionContext& ctx) const override {
        return ctx.GetData()["Faces"].ArraySize() > 0;
    }
    bool IsSimilarSubstituteAllowed(const TComputerVisionContext& ctx) const override;
    TStringBuf GetAnswerId() const override { return TStringBuf("similar_people"); }
    TStringBuf GetDisableFlag() const override;

    const TStringBuf AdditionalFlag() const override {
        return TStringBuf("selebrity_filter");
    }

    TStringBuf GetCannotApplyMessage() const override;
    void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

protected:
    void Compose(TComputerVisionContext& ctx) const override;
};

} // NBASS
