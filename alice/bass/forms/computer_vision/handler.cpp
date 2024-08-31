#include "handler.h"

#include "barcode_handler.h"
#include "context.h"
#include "info_cv_handler.h"
#include "ocr_voice_handler.h"
#include "office_lens_handler.h"
#include "similar_artwork_handler.h"
#include "similarlike_cv_handler.h"
#include "translate_handler.h"

#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/proto/protobuf.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/digest/city.h>
#include <util/generic/guid.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>

namespace NBASS {

namespace NComputerVisionForms {
const TStringBuf IMAGE_WHAT_IS_THIS = "personal_assistant.scenarios.image_what_is_this";

const TStringBuf IMAGE_WHAT_IS_THIS_CLOTHES_BOXES = "personal_assistant.scenarios.image_what_is_this__clothes";

const TStringBuf IMAGE_WHAT_IS_THIS_CLOTHES = "personal_assistant.scenarios.image_what_is_this__clothes_forced";

const TStringBuf IMAGE_WHAT_IS_THIS_DETAILS = "personal_assistant.scenarios.image_what_is_this__details";

const TStringBuf IMAGE_WHAT_IS_THIS_FRONTAL = "personal_assistant.scenarios.image_what_is_this_frontal";

const TStringBuf IMAGE_WHAT_IS_THIS_FRONTAL_SIMILAR_PEOPLE = "personal_assistant.scenarios.image_what_is_this_frontal_similar_people";

const TStringBuf IMAGE_WHAT_IS_THIS_MARKET = "personal_assistant.scenarios.image_what_is_this__market";

const TStringBuf IMAGE_WHAT_IS_THIS_NEGATIVE_FEEDBACK = "personal_assistant.feedback.feedback_negative_images";

const TStringBuf IMAGE_WHAT_IS_THIS_OCR = "personal_assistant.scenarios.image_what_is_this__ocr";

const TStringBuf IMAGE_WHAT_IS_THIS_SIMILAR_PEOPLE = "personal_assistant.scenarios.image_what_is_this_similar_people";

const TStringBuf IMAGE_WHAT_IS_THIS_ELLIPSIS_SIMILAR_PEOPLE = "personal_assistant.scenarios.image_what_is_this__similar_people";

const TStringBuf ONBOARDING_IMAGE_SEARCH = "personal_assistant.scenarios.onboarding_image_search";

const TStringBuf ONBOARDING_IMAGE_CARD_ID = "onboarding_image";

} // NComputerVisionForms

namespace NComputerVisionFlags {
    const TStringBuf FORCE_SIMILAR_AS_SECOND_ANSWER = "computer_vision_force_similar_as_second_answer";

    const TStringBuf RANDOM_SCENARIO = "computer_vision_random_scenario";

    extern const TStringBuf DISABLE_TAG = "computer_vision_disable_tag";
}

using namespace NImages::NCbir;
using TAnswerHandlerHolder = const THolder<IComputerVisionAnswer>;

namespace {

constexpr TStringBuf ONBOARDING_IMAGES[] = {
    TStringBuf("object"), TStringBuf("goods"), TStringBuf("Drakula_V"),
    TStringBuf("text"), TStringBuf("celebrity"), TStringBuf("similar"),
    TStringBuf("animal"), TStringBuf("plant"), TStringBuf("qr_code"),
    TStringBuf("auto")
};

constexpr std::pair<TStringBuf, TStringBuf> ONBOARDING_IMAGES_DATA[] = {
    {"Распознать объект", "object"},
    {"Найти товар", "goods"},
    {"Распознать и перевести текст", "text"},
    {"Узнать знаменитость", "celebrity"},
    {"Найти похожие изображения", "similar"},
    {"Узнать породу животного", "animal"},
    {"Определить вид растения", "plant"},
    {"Распознать QR-код", "qr_code"},
    {"Определить модель авто", "auto"},
};

static TAnswerHandlerHolder answerBarcode = MakeHolder<TCVAnswerBarcode>();
static TAnswerHandlerHolder answerClothes = MakeHolder<TCVAnswerClothes>();
static TAnswerHandlerHolder answerDark = MakeHolder<TCVAnswerDark>();
static TAnswerHandlerHolder answerEntitySearch = MakeHolder<TCVAnswerEntitySearch>();
static TAnswerHandlerHolder answerFace = MakeHolder<TCVAnswerFace>();
static TAnswerHandlerHolder answerGruesome = MakeHolder<TCVAnswerGruesome>();
static TAnswerHandlerHolder answerMarket = MakeHolder<TCVAnswerMarket>();
static TAnswerHandlerHolder answerIziPushkinMuseum = MakeHolder<TCVAnswerIziPushkinMuseum>();
static TAnswerHandlerHolder answerOcr = MakeHolder<TCVAnswerOcr>();
static TAnswerHandlerHolder answerPeople = MakeHolder<TCVAnswerPeople>();
static TAnswerHandlerHolder answerPorn = MakeHolder<TCVAnswerPorn>();
static TAnswerHandlerHolder answerSimilar = MakeHolder<TCVAnswerSimilarGallery>();
static TAnswerHandlerHolder answerTag = MakeHolder<TCVAnswerTag>();
static TAnswerHandlerHolder answerSimilarPeople = MakeHolder<TCVAnswerSimilarPeople>();
static TAnswerHandlerHolder answerOcrVoice = MakeHolder<TCVAnswerOcrVoice>();
static TAnswerHandlerHolder answerOcrVoiceSuggest = MakeHolder<TCVAnswerOcrVoiceSuggest>();
static TAnswerHandlerHolder answerOfficeLens = MakeHolder<TCVAnswerOfficeLens>();
static TAnswerHandlerHolder answerInfo = MakeHolder<TCVAnswerInfo>();
static TAnswerHandlerHolder answerSimilarLike = MakeHolder<TCVAnswerSimilarLike>();
static TAnswerHandlerHolder answerTranslate = MakeHolder<TCVAnswerTranslate>();
static TAnswerHandlerHolder answerSimilarArtwork = MakeHolder<TCVAnswerSimilarArtwork>();

constexpr TStringBuf SHOW_BUTTONS_DIRECTIVE = "show_buttons";
constexpr TStringBuf FILL_CLOUD_UI_DIRECTIVE = "fill_cloud_ui";
constexpr TStringBuf CLOUD_UI_HARDCODED_TEXT = "Чем могу помочь?";

void FillFrameActionOnCardCallback(NAlice::NScenarios::TCallbackDirective& onCardActionDirective, const size_t itemNumber) {
    onCardActionDirective.SetName("on_card_action");
    onCardActionDirective.SetIgnoreAnswer(true);
    *onCardActionDirective.MutablePayload() = NAlice::TProtoStructBuilder()
            .Set("card_id", NComputerVisionForms::ONBOARDING_IMAGE_CARD_ID.data())
            .Set("intent_name", NComputerVisionForms::ONBOARDING_IMAGE_SEARCH.data())
            .Set("item_number", ToString(itemNumber + 1))
            .Build();
}

NAlice::NScenarios::TFrameAction ConstructFrameAction(const size_t itemNumber) {
    NAlice::NScenarios::TFrameAction frameAction;
    auto& directives = *frameAction.MutableDirectives()->MutableList();

    // add "start_image_recognizer" directive
    directives.Add()->MutableStartImageRecognizerDirective();

    // add "on_card_action" directive
    FillFrameActionOnCardCallback(*directives.Add()->MutableCallbackDirective(), itemNumber);

    return frameAction;
}

NSc::TValue ConstructShowButtonData(const TString& actionId, const TStringBuf text, const TStringBuf imageurl) {
    NSc::TValue data;
    data["action_id"].SetString(actionId);
    data["title"].SetString(TString{text});
    data["text"].SetString(TString{text});

    NSc::TValue theme;
    theme["image_url"].SetString(imageurl);
    data["theme"].Swap(theme);

    return data;
}

void AddShowButtonDirectiveAndActions(TContext& ctx) {
    NSc::TValue showButtonsDirective;
    showButtonsDirective["screen_id"].SetString("cloud_ui");

    size_t itemNumber = 0;
    for (const auto& [text, name] : ONBOARDING_IMAGES_DATA) {
        const TString actionId = CreateGuidAsString();

        // add TFrameAction
        const auto frameAction = ConstructFrameAction(itemNumber);
        ctx.AddFrameActionBlock(actionId, frameAction);

        // add button data
        const TAvatar* avatar = ctx.Avatar(TStringBuf("computer_vision"), name);
        showButtonsDirective["buttons"].Push() = ConstructShowButtonData(actionId, text, avatar->Https);

        ++itemNumber;
    }
    ctx.AddCommand<TShowButtonsDirective>(SHOW_BUTTONS_DIRECTIVE, std::move(showButtonsDirective));

    NSc::TValue fillCloudUiDirective;
    fillCloudUiDirective["text"].SetString(CLOUD_UI_HARDCODED_TEXT);
    ctx.AddCommand<TFillCloudUiDirective>(FILL_CLOUD_UI_DIRECTIVE, std::move(fillCloudUiDirective));
}

void AttachImageRecognizerOnboarding(TContext& context) {
    NSc::TValue cardContent;
    for (const auto& imageName : ONBOARDING_IMAGES) {
        if (const TAvatar* avatar = context.Avatar(TStringBuf("computer_vision"), imageName)) {
            cardContent[imageName]["icon_url"].SetString(avatar->Https);
        } else {
            cardContent = NSc::Null();
            break;
        }
    }
    if (!cardContent.IsNull()) {
        context.AddTextCardBlock(TStringBuf("onboarding_image__message"));
        context.AddDivCardBlock(NComputerVisionForms::ONBOARDING_IMAGE_CARD_ID, std::move(cardContent));
    }
    if (context.ClientFeatures().SupportsCloudUi()
            && context.HasExpFlag(NAlice::NExperiments::ONBOARDING_USE_CLOUD_UI)) {
        AddShowButtonDirectiveAndActions(context);
    }
}

void AttachUnsupportedOperationError(TContext& context, bool isOnboarding) {
    if (context.Meta().Utterance().Get(TStringBuf(""))) {
        context.AddSearchSuggest();
    }
    context.AddOnboardingSuggest();
    NSc::TValue unsupportedOperationError;
    unsupportedOperationError[TStringBuf("code")].SetString("unsupported_operation");
    if (isOnboarding) {
        context.AddErrorBlock(TError(TError::EType::ONBOARDINGERROR,
                                     TStringBuf("image_onboarding_error")),
                              std::move(unsupportedOperationError));
    } else {
        context.AddErrorBlock(TError(TError::EType::IMAGEERROR,
                                     TStringBuf("image_recognizer_error")),
                              std::move(unsupportedOperationError));
    }
}

bool ShouldStopHandling(TContext& context, bool isOnboarding) {
    const auto& clientFeatures = context.ClientFeatures();
    if (!clientFeatures.SupportsImageRecognizer()) {
        AttachUnsupportedOperationError(context, isOnboarding);
        return true;
    }
    return false;
}

} // namespace

// IComputerVisionBaseHandler --------------------------------------------------
TResultValue IComputerVisionBaseHandler::Do(TRequestHandler& r) {
    TContext& context = r.Ctx();
    context.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::IMAGES_WHAT_IS_THIS);
    const TInstant deadline = TInstant::Now() + context.GetConfig().Vins().ComputerVision().Timeout();
    if (ShouldStopHandling(context, /* isOnboarding */ false)) {
        return TResultValue();
    }

    TResultValue result;
    try {
        auto cvContext = MakeHolder<TComputerVisionContext>(*this, context, deadline);
        result = WrappedDo(*cvContext);
        cvContext->OutputResult();
    } catch (const yexception& e) {
        return TError(TError::EType::SYSTEM, e.what());
    }
    return result;
}

TMaybe<TString> IComputerVisionBaseHandler::GetForcingString() const {
    return Nothing();
}

// TComputerVisionMainHandler --------------------------------------------------

const TAnswersList TComputerVisionMainHandler::StubAnswers = {
    answerBarcode.Get(),
    answerDark.Get(),
    answerGruesome.Get(),
    answerIziPushkinMuseum.Get(),
    answerPorn.Get(),
};

const TFlagAnswerPairs TComputerVisionMainHandler::ForcingAnswers = {
    {TStringBuf("computer_vision_force_clothes"),       *answerClothes},
    {TStringBuf("computer_vision_force_dark"),          *answerDark},
    {TStringBuf("computer_vision_force_entity_search"), *answerEntitySearch},
    {TStringBuf("computer_vision_force_face"),          *answerFace},
    {TStringBuf("computer_vision_force_gruesome"),      *answerGruesome},
    {TStringBuf("computer_vision_force_market"),        *answerMarket},
    {TStringBuf("computer_vision_force_ocr"),           *answerOcr},
    {TStringBuf("computer_vision_force_people"),        *answerPeople},
    {TStringBuf("computer_vision_force_porno"),         *answerPorn},
    {TStringBuf("computer_vision_force_similargallery"),*answerSimilar},
    {TStringBuf("computer_vision_force_tag"),           *answerTag},
    {TStringBuf("computer_vision_force_similar_people"), *answerSimilarPeople},
    {TCVAnswerOcrVoice::ForceName(),                      *answerOcrVoice},
    {TCVAnswerOfficeLens::ForceName(),                    *answerOfficeLens},
    {TCVAnswerInfo::ForceName(),                          *answerInfo},
    {TCVAnswerSimilarLike::ForceName(),                   *answerSimilarLike},
    {TCVAnswerBarcode::ForceName(),                       *answerBarcode},
    {TCVAnswerTranslate::ForceName(),                     *answerTranslate},
};

const TIntentToAnswerMap TComputerVisionContext::AnswerAlternatives = {
    {ECbirIntents::CI_CLOTHES,  *answerClothes},
    {ECbirIntents::CI_ENTITY,   *answerEntitySearch},
    {ECbirIntents::CI_FACES,    *answerFace},
    {ECbirIntents::CI_MARKET,   *answerMarket},
    {ECbirIntents::CI_OCR,      *answerOcr},
    {ECbirIntents::CI_PEOPLE,   *answerFace},
    {ECbirIntents::CI_SIMILAR,  *answerSimilar},
    {ECbirIntents::CI_TAGS,     *answerSimilar},
    {ECbirIntents::CI_SIMILAR_PEOPLE, *answerSimilarPeople},
    {ECbirIntents::CI_OCR_VOICE, *answerOcrVoiceSuggest},
    {ECbirIntents::CI_INFO,      *answerInfo},
    {ECbirIntents::CI_SIMILAR_LIKE,      *answerSimilarLike},
    {ECbirIntents::CI_SIMILAR_ARTWORK,   *answerSimilarArtwork},
};

const TCaptureModeToAnswerMap TComputerVisionMainHandler::CaptureModes = {
        {ECaptureMode::DOCUMENT, *answerOfficeLens},
        {ECaptureMode::OCR, *answerOcr},
        {ECaptureMode::OCR_VOICE, *answerOcrVoice},
        {ECaptureMode::MARKET, *answerMarket},
        {ECaptureMode::CLOTHES, *answerClothes},
        {ECaptureMode::SIMILAR_LIKE, *answerSimilar},
        {ECaptureMode::SIMILAR_PEOPLE_FRONTAL, *answerSimilarPeople},
        {ECaptureMode::SIMILAR_PEOPLE, *answerSimilarPeople},
        {ECaptureMode::BARCODE, *answerBarcode},
        {ECaptureMode::TRANSLATE, *answerTranslate},
        {ECaptureMode::SIMILAR_ARTWORK, *answerSimilarArtwork},
};

TComputerVisionMainHandler::TComputerVisionMainHandler(ECaptureMode captureMode, bool frontal)
    : CaptureMode(captureMode)
    , Frontal(frontal)
{

}

void TComputerVisionMainHandler::Init() {
    answerEntitySearch->LoadData();
    answerOcrVoice->LoadData();
}

bool TComputerVisionMainHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    for (const auto answer : StubAnswers) {
        if (answer->TryApplyTo(cvContext)) {
            return true;
        }
    }

    if (cvContext.HasExpFlag(NComputerVisionFlags::RANDOM_SCENARIO)) {
        return MakeRandomAnswer(cvContext);
    }

    NSc::TValue failedIntents;
    failedIntents.SetArray();
    for (const auto* intent : cvContext.GetPrioritizedAnswers()) {
        Y_ENSURE(intent != nullptr);
        if (intent->TryApplyTo(cvContext)) {
            if (!failedIntents.ArrayEmpty()) {
                cvContext.Output("failed_intents") = failedIntents;
                Y_STATS_INC_COUNTER(TStringBuf("bass_computer_vision_result_error_cannot_apply_intent"));
            }
            return true;
        }
        failedIntents.Push(intent->GetAnswerId());
    }
    return false;
}

bool TComputerVisionMainHandler::MakeRandomAnswer(TComputerVisionContext& cvContext) const {
    TVector<const IComputerVisionAnswer*> answers = {
            answerClothes.Get(),
            answerEntitySearch.Get(),
            answerFace.Get(),
            answerMarket.Get(),
            answerOcr.Get(),
            answerSimilar.Get(),
            answerSimilarPeople.Get(),
    };

    ui64 seed = CityHash64(cvContext.GetCbirId());
    Shuffle(answers.begin(), answers.end(), TReallyFastRng32(seed));

    for (const auto answer : answers) {
        if (answer->TryApplyTo(cvContext)) {
            return true;
        }
    }
    return false;
}

const IComputerVisionAnswer* TComputerVisionMainHandler::TryExtractForceAnswer(TComputerVisionContext& cvContext,
                                                                               bool& needSimilarAnswer) const {
    needSimilarAnswer = false;
    ECaptureMode captureMode = cvContext.GetCaptureMode();
    if (CaptureModes.contains(captureMode)) {
        return &CaptureModes.at(captureMode);
    }

    const TMaybe<TStringBuf> forceAnswer = cvContext.GetForceAnswerSlot();

    for (const auto& forcingAnswer : ForcingAnswers) {
        if (cvContext.HasExpFlag(forcingAnswer.first) || forcingAnswer.first == forceAnswer) {
            const IComputerVisionAnswer* result = &forcingAnswer.second;
            needSimilarAnswer = result->IsSimilarSubstituteAllowed(cvContext);
            return result;
        }
    }

    return nullptr;
}

TResultValue TComputerVisionMainHandler::WrappedDo(TComputerVisionContext& cvContext) const {
    bool needSimilar = false;
    const IComputerVisionAnswer* forceAnswer = TryExtractForceAnswer(cvContext, needSimilar);
    const bool needOcrData = IsNeedOcrData(forceAnswer);
    const bool needOfficeLensData = IsNeedOfficeLensData(forceAnswer);
    const TStringBuf additionalFlag = GetAdditionalFlag(forceAnswer);
    if (cvContext.PrepareComputerVisionData(CaptureMode, Frontal, needOcrData, needOfficeLensData,
                                            additionalFlag)) {
        bool answerResult = false;
        if (forceAnswer) {
            answerResult = forceAnswer->TryApplyTo(cvContext, /* force */ true, /* should attach alternative intents */ false);
            if (!answerResult) {
                if (needSimilar) {
                    answerResult = answerSimilar->TryApplyTo(cvContext, /* force */ true,
                                                             /* should attach alternative intents */ false);
                } else {
                    forceAnswer->AttachCannotApplyMessage(cvContext);
                    answerResult = true;
                }
            }
        } else {
            answerResult = MakeBestAnswer(cvContext);
        }
        if (!answerResult) {
            cvContext.Output("code").SetString("cannot_recognize");
            Y_STATS_INC_COUNTER("bass_computer_vision_result_error_cannot_recognize");
        }
        cvContext.HandlerContext().AddOnboardingSuggest();
    } else {
        const TMaybe<TString> forceAnswerMaybe = GetForcingString();
        if (forceAnswerMaybe.Defined()) {
            cvContext.HandlerContext().CreateSlot("forcing_answer", "string", true, NSc::TValue(forceAnswerMaybe.GetRef()));
        }
    }
    return TResultValue();
}

bool TComputerVisionMainHandler::IsNeedOcrData(const IComputerVisionAnswer* forceAnswer) const {
    return forceAnswer ? forceAnswer->IsNeedOcrData() : false;
}

bool TComputerVisionMainHandler::IsNeedOfficeLensData(const IComputerVisionAnswer* forceAnswer) const {
    return forceAnswer ? forceAnswer->IsNeedOfficeLensData() : false;
}

const TStringBuf TComputerVisionMainHandler::GetAdditionalFlag(const IComputerVisionAnswer* forceAnswer) const {
    return forceAnswer ? forceAnswer->AdditionalFlag() : TStringBuf();
}

void TComputerVisionMainHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionMainHandler>(ECaptureMode::PHOTO, false);
    };
    auto handlerFrontal = []() {
        return MakeHolder<TComputerVisionMainHandler>(ECaptureMode::PHOTO, true);
    };

    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS, handler);
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_FRONTAL, handlerFrontal);
}

void TComputerVisionMainHandler::SetAsResponse(TContext& ctx, TStringBuf /*subtypeCmd*/) {
    TContext::TPtr newCtx = ctx.SetResponseForm(NComputerVisionForms::IMAGE_WHAT_IS_THIS, false);
    Y_ENSURE(newCtx);
}

bool TComputerVisionMainHandler::IsSupported(const TContext& ctx) {
    return ctx.ClientFeatures().SupportsImageRecognizer();
}

// TComputerVisionOnboardingHandler --------------------------------------------
void TComputerVisionOnboardingHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionOnboardingHandler>();
    };
    handlers->emplace(NComputerVisionForms::ONBOARDING_IMAGE_SEARCH, handler);
}

TResultValue TComputerVisionOnboardingHandler::Do(TRequestHandler& r) {
    TContext& context = r.Ctx();
    context.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::IMAGES_WHAT_IS_THIS);
    if (!ShouldStopHandling(context, /* isOnboarding */ true)) {
        AttachImageRecognizerOnboarding(context);
        context.AddOnboardingSuggest();
    }
    return TResultValue();
}

// TComputerVisionClothesBoxHandler -----------------------------------------------
void TComputerVisionClothesBoxHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionClothesBoxHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_CLOTHES_BOXES, handler);
}

TResultValue TComputerVisionClothesBoxHandler::WrappedDo(TComputerVisionContext& cvContext) const {
    if (const TResultValue fetchError = cvContext.RequestClothesEllipsis()) {
        return fetchError;
    }
    cvContext.AddClothesEllipsisAnswer();
    cvContext.AppendFeedbackOption(NComputerVisionFeedbackOptions::USELESS);
    cvContext.AppendFeedbackOption(NComputerVisionFeedbackOptions::OTHER);
    cvContext.HandlerContext().AddOnboardingSuggest();
    return TResultValue();
}

// TComputerVisionSimilarHandler
TComputerVisionSimilarHandler::TComputerVisionSimilarHandler()
        : TComputerVisionMainHandler(ECaptureMode::SIMILAR_LIKE, false)
{
}

void TComputerVisionSimilarHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionSimilarHandler>();
    };
    handlers->emplace(TComputerVisionSimilarHandler::FormName(), handler);
}

// TComputerVisionEllipsisClothesHandler ---------------------------------------
void TComputerVisionEllipsisClothesHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionEllipsisClothesHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_CLOTHES, handler);
}

bool TComputerVisionEllipsisClothesHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    if (!answerClothes->TryApplyTo(cvContext, /* force */ true)) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_clothes_empty");
    }
    return true;
}

// TComputerVisionEllipsisDetailsHandler ---------------------------------------
void TComputerVisionEllipsisDetailsHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionEllipsisDetailsHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_DETAILS, handler);
}

bool TComputerVisionEllipsisDetailsHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    return answerSimilar->TryApplyTo(cvContext, /* force */ true);
}

// TComputerVisionEllipsisOcrHandler -------------------------------------------
void TComputerVisionEllipsisOcrHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionEllipsisOcrHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_OCR, handler);
}

bool TComputerVisionEllipsisOcrHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    if (!cvContext.HasCbirId()) {
        cvContext.Output("code").SetString("ocr_form_error");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_ocr_error_form");
        return false;
    } else {
        cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
        if (cvContext.GetOcrResultCategory() > EOcrResultCategory::ORC_NONE) {
            cvContext.RedirectTo("ocr", "imageocr");
            cvContext.AttachTextCard();
            cvContext.Output("ocr_ellipsis").SetBool(true);
            Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_ocr_success");
        } else {
            answerOcr->AttachCannotApplyMessage(cvContext);
            Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_ocr_notext");
        }
        cvContext.AttachSimilarSearchSuggest(/*showButton*/ false);
        cvContext.HandlerContext().AddOnboardingSuggest();
        cvContext.TryFillLastForceAnswer(ECaptureMode::OCR);
    }
    return true;
}

// TComputerVisionEllipsisMarketHandler ----------------------------------------
void TComputerVisionEllipsisMarketHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionEllipsisMarketHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_MARKET, handler);
}

bool TComputerVisionEllipsisMarketHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    if (!answerMarket->TryApplyTo(cvContext, /* force */ true)) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_market_empty");
    }
    return true;
}

// TComputerVisionNegativeFeedbackHandler --------------------------------------
void TComputerVisionNegativeFeedbackHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionNegativeFeedbackHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_NEGATIVE_FEEDBACK, handler);
}

TResultValue TComputerVisionNegativeFeedbackHandler::WrappedDo(TComputerVisionContext& cvContext) const {
    cvContext.HandlerContext().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::FEEDBACK);
    const bool seenBarcode = cvContext.AnswerHas("barcode_uri") || cvContext.AnswerHas("barcode_text");
    const bool seenClothes = cvContext.AnswerHas("clothes_crops");
    const bool seenEntity = cvContext.AnswerHas("is_entity_search") || cvContext.AnswerHas("is_entity_search_joke");
    const bool seenFace = cvContext.AnswerHas("is_face");
    const bool seenMarket = cvContext.AnswerHas("has_market_gallery");
    const bool seenOcr = cvContext.AnswerHas("is_ocr_text");
    const bool seenPeople = cvContext.AnswerHas("has_people_gallery");
    const bool seenTag = cvContext.AnswerHas("tag");

    const bool seenSimilar = cvContext.AnswerHas("is_similar") || seenFace || seenOcr || seenPeople
                             || seenTag && !(seenClothes || seenEntity || seenMarket);

    if (seenTag || seenEntity) {
        cvContext.AttachFeedbackSuggest("images__tag_wrong");
    }
    if (seenTag && seenSimilar) {
        cvContext.AttachFeedbackSuggest("_offensive_answer");
    }

    if (seenBarcode) {
        cvContext.AttachFeedbackSuggest("images__barcode_wrong");
        cvContext.AttachFeedbackSuggest("images__barcode_unwanted");
    } else if (seenClothes) {
        cvContext.AttachFeedbackSuggest("images__clothes_unwanted");
    } else if (seenMarket) {
        cvContext.AttachFeedbackSuggest("images__market_poor");
        cvContext.AttachFeedbackSuggest("images__market_unwanted");
        cvContext.AttachFeedbackSuggest("images__market_link");
    } else if (seenOcr) {
        cvContext.AttachFeedbackSuggest("images__ocr_poor");
        cvContext.AttachFeedbackSuggest("images__ocr_translate");
        cvContext.AttachFeedbackSuggest("images__ocr_unwanted");
    } else {
        cvContext.AttachFeedbackSuggest("images__useless");
    }

    if (seenSimilar) {
        cvContext.AttachFeedbackSuggest("images__unsimilar");
    }
    cvContext.AttachFeedbackSuggest("_other");
    cvContext.AttachFeedbackSuggest("_all_good");
    cvContext.HandlerContext().AddOnboardingSuggest();
    return TResultValue();
}

bool TComputerVisionContext::IsEntityAnswerSuitable() const {
    return !CbirIntents.empty()
           && CbirIntents.front() == ECbirIntents::CI_ENTITY
           && answerEntitySearch->IsSuitable(*this, false);
}

// TComputerVisionFrontalSimilarPeople ------------------------------------------
TComputerVisionFrontalSimilarPeople::TComputerVisionFrontalSimilarPeople()
    : TComputerVisionMainHandler(ECaptureMode::SIMILAR_PEOPLE_FRONTAL, true)
{}

void TComputerVisionFrontalSimilarPeople::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionFrontalSimilarPeople>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_FRONTAL_SIMILAR_PEOPLE, handler);
}

bool TComputerVisionFrontalSimilarPeople::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    if (!answerSimilarPeople->TryApplyTo(cvContext, /* force */ true)) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_similar_people_empty");
    }
    return true;
}

TComputerVisionSimilarPeople::TComputerVisionSimilarPeople()
        : TComputerVisionMainHandler(ECaptureMode::SIMILAR_PEOPLE, false)
{}

TMaybe<TString> TComputerVisionFrontalSimilarPeople::GetForcingString() const {
    return "computer_vision_force_similar_people";
}

// TComputerVisionSimilarPeople ------------------------------------------
void TComputerVisionSimilarPeople::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionSimilarPeople>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_SIMILAR_PEOPLE, handler);
}

bool TComputerVisionSimilarPeople::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    if (!answerSimilarPeople->TryApplyTo(cvContext, /* force */ true)) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_similar_people_empty");
    }
    return true;
}

TMaybe<TString> TComputerVisionSimilarPeople::GetForcingString() const {
    return "computer_vision_force_similar_people";
}

// TComputerVisionEllipsisSimilarPeopleHandler ----------------------------------------
void TComputerVisionEllipsisSimilarPeopleHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TComputerVisionEllipsisSimilarPeopleHandler>();
    };
    handlers->emplace(NComputerVisionForms::IMAGE_WHAT_IS_THIS_ELLIPSIS_SIMILAR_PEOPLE, handler);
}

const TStringBuf TComputerVisionEllipsisSimilarPeopleHandler::GetAdditionalFlag(const IComputerVisionAnswer*) const {
    return answerSimilarPeople->AdditionalFlag();
}

bool TComputerVisionEllipsisSimilarPeopleHandler::MakeBestAnswer(TComputerVisionContext& cvContext) const {
    cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
    if (!answerSimilarPeople->TryApplyTo(cvContext, /* force */ true)) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_ellipsis_similar_people_empty");
    }
    return true;
}

} // NBASS
