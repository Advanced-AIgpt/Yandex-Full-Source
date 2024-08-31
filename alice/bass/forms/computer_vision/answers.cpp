#include "context.h"

#include "ocr_voice_handler.h"

#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/uri/uri.h>
#include <kernel/urlnorm/normalize.h>

#include <util/string/builder.h>

#include <cmath>

namespace NBASS {

namespace NComputerVisionFlags {

const TStringBuf ENABLE_COMMON_TAGS = "image_recognizer_common_tags";
const TStringBuf ENABLE_MULTI_ENTITY_OBJECTS = "image_multi_entity_objects";
const TStringBuf DISABLE_AUTO_OCR = "image_recognizer_disable_auto_ocr";
const TStringBuf ENABLE_PEOPLE = "image_recognizer_people";

const TStringBuf DISABLE_CLOTHES = "computer_vision_disable_clothes";
const TStringBuf DISABLE_ENTITY = "computer_vision_disable_entity";
const TStringBuf DISABLE_FACES = "computer_vision_disable_faces";
const TStringBuf DISABLE_MARKET = "computer_vision_disable_market";
const TStringBuf DISABLE_OCR = "computer_vision_disable_ocr";
const TStringBuf DISABLE_SIMILAR = "computer_vision_disable_similar";
const TStringBuf DISABLE_SIMILAR_PEOPLE = "computer_vision_disable_similar_people";
const TStringBuf DISABLE_TAG = "computer_vision_disable_tag";

} // NComputerVisionFlags

using namespace NImages::NCbir;

namespace {

constexpr TStringBuf ENTITY_AVATAR = "/ObjectResponses[0]/base_info/image/avatar";
constexpr TStringBuf ENTITY_BASE_NAME = "/ObjectResponses[0]/base_info/name";
constexpr TStringBuf ENTITY_BASE_TITLE = "/ObjectResponses[0]/base_info/title";
constexpr TStringBuf ENTITY_DESC_URL = "/ObjectResponses[0]/base_info/description_source/url";
constexpr TStringBuf ENTITY_DESCRIPTION = "/ObjectResponses[0]/base_info/description";
constexpr TStringBuf ENTITY_ID = "/ObjectResponses[0]/base_info/id";
constexpr TStringBuf ENTITY_REQUEST = "/ObjectResponses[0]/base_info/search_request";
constexpr TStringBuf ENTITY_URL = "/ObjectResponses[0]/base_info/source/url";
constexpr TStringBuf ENTITY_VOICE = "/ObjectResponses[0]/voiceInfo/ru[0]/text";

constexpr TStringBuf MUSEUM_PROTMO_PATH = "/Promo/museum";
constexpr std::pair<int, int> MUSEUM_IMAGE_SIZE = std::make_pair(120, 120);

TString SerializeCropBox(const NSc::TValue& crop) {
    TStringBuilder box;
    box << crop["x0"].GetNumber() << TStringBuf(";");
    box << crop["y0"].GetNumber() << TStringBuf(";");
    box << crop["x1"].GetNumber() << TStringBuf(";");
    box << crop["y1"].GetNumber();
    return box;
}

} // namespace

// TCVAnswerPorn ---------------------------------------------------------------
bool TCVAnswerPorn::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    constexpr double thresholdBinaryPorn = 0.86;
    constexpr double thresholdMobilePorn = 0.37;

    const bool isEntityAnswerSuitable = ctx.IsEntityAnswerSuitable();

    const NSc::TValue& classes = ctx.GetData()["Classes"];
    return (force ||
            (!isEntityAnswerSuitable &&
             ((classes["binary_porn"].GetNumber() > thresholdBinaryPorn) ||
              (classes["mobile_porn"].GetNumber() > thresholdMobilePorn))));
}

void TCVAnswerPorn::Compose(TComputerVisionContext& ctx) const {
    ctx.Output("is_binary_porn").SetBool(true);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_binary_porn");
    ctx.AttachTextCard();
    if (ctx.HandlerContext().GetContentRestrictionLevel() != EContentRestrictionLevel::Children) {
        ctx.AttachSimilarSearchSuggest();
    }
}

// TCVAnswerGruesome -----------------------------------------------------------
bool TCVAnswerGruesome::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    constexpr double thresholdGruesome = 0.85;

    const NSc::TValue& classes = ctx.GetData()["Classes"];
    return (force || classes["gruesome"].GetNumber() > thresholdGruesome);
}

void TCVAnswerGruesome::Compose(TComputerVisionContext& ctx) const {
    ctx.Output("is_perversion").SetBool(true);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_perversion");
    ctx.AttachTextCard();
    if (ctx.HandlerContext().GetContentRestrictionLevel() != EContentRestrictionLevel::Children) {
        ctx.AttachSimilarSearchSuggest();
    }
}

// TCVAnswerDark ---------------------------------------------------------------
bool TCVAnswerDark::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    constexpr double thresholdDark = 0.85;

    return (force || ctx.GetData()["Classes"]["dark"].GetNumber() > thresholdDark);
}

void TCVAnswerDark::Compose(TComputerVisionContext& ctx) const {
    ctx.Output("is_dark").SetBool(true);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_dark");
    ctx.AttachTextCard();
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
    ctx.AttachSimilarSearchSuggest();
}

// TCVAnswerEntitySearch -------------------------------------------------------
bool TCVAnswerEntitySearch::IsValidEntitySearchData(const NSc::TValue& cvData) const {
    for (const auto field : {ENTITY_AVATAR, ENTITY_DESC_URL, ENTITY_DESCRIPTION,
                             ENTITY_ID, ENTITY_REQUEST, ENTITY_URL,
                             ENTITY_VOICE, ENTITY_BASE_NAME, ENTITY_BASE_TITLE})
    {
        if (cvData.TrySelect(field).IsNull()) {
            return false;
        }
    }
    return true;
}

void TCVAnswerEntitySearch::PutEntitySearchResults(TComputerVisionContext& ctx, bool fillOnlySlot, bool &multipleObjectsGenerated) const {
    static const TStringBuf slotLabel("search_results");
    const TStringBuf divCardLabel = "new_image_search_object";

    const NSc::TValue& cvData = ctx.GetData();
    if (cvData["ObjectResponses"].GetArray().size() <= 1 || !ctx.HasExpFlag(NComputerVisionFlags::ENABLE_MULTI_ENTITY_OBJECTS)) {
        const TStringBuf entityName0 = cvData.TrySelect(ENTITY_REQUEST).GetString();
        TStringBuilder text;
        text << entityName0 << TStringBuf(" — ") << cvData.TrySelect(ENTITY_DESCRIPTION).GetString();

        NSc::TValue entityResults;
        entityResults["object"]["text"].SetString(text);
        entityResults["object"]["tts"].SetString(cvData.TrySelect(ENTITY_VOICE).GetString());
        entityResults["object"]["url"].SetString(AddUtmReferrer(ctx.HandlerContext().MetaClientInfo(), cvData.TrySelect(ENTITY_URL).GetString()));
        entityResults["serp"]["url"].SetString(
            GenerateSearchUri(&ctx.HandlerContext(), entityName0));
        entityResults["image_serp"]["url"].SetString(ctx.GenerateImagesSearchUrl("similar"));

        if (!fillOnlySlot) {
            NSc::TValue entityObject = CreateEntitySearchObject(ctx, entityResults);
            ctx.HandlerContext().AddDivCardBlock(divCardLabel, std::move(entityObject));
        }
        ctx.HandlerContext().CreateSlot(slotLabel, slotLabel, /* optional */ true, std::move(entityResults));
        ctx.AttachUtteranceSuggest(entityName0);
    } else {
        TString mainOnto;
        NSc::TValue galleryDivCard;
        for (const auto& entity: cvData["ObjectResponses"].GetArray()) {
            const TStringBuf entityName = entity.TrySelect("/base_info/search_request").GetString();
            const TStringBuf avatar = entity.TrySelect("/base_info/image/avatar").GetString();
            const TString url = NUrlNorm::NormalizeUrl(entity.TrySelect("/base_info/source/url").GetString());
            const TString urlBeautified = [&]() {
                if (NUri::TUri uri; uri.Parse(url) == NUri::TUri::TState::ParsedOK) {
                    return TString{uri.GetField(NUri::TUri::TField::FieldHost)};
                }
                return url;
            }();

            NSc::TValue galleryItem;
            galleryItem["image"].SetString(avatar.StartsWith('/') ? TStringBuilder() << TStringBuf("https:") << avatar : avatar);
            galleryItem["link"] = AddUtmReferrer(ctx.HandlerContext().MetaClientInfo(), url);
            galleryItem["title"] = entityName;
            galleryItem["html_host"] = urlBeautified;

            if (mainOnto.empty()) {
                mainOnto = entityName;
            }

            galleryDivCard["similar_gallery"].Push(std::move(galleryItem));
        }
        ctx.HandlerContext().AddDivCardBlock(TStringBuf("image__onto_gallery"), std::move(galleryDivCard));
        multipleObjectsGenerated = true;

        if (!mainOnto.empty()) {
            ctx.Output("tag") = mainOnto;
            ctx.Output("tag_confidence") = ToString(NImages::NCbir::ETagConfidenceCategory::TCC_HIGH);
        }
    }
}

NSc::TValue TCVAnswerEntitySearch::CreateEntitySearchObject(const TComputerVisionContext& ctx,
                                                            const NSc::TValue& entityResults) const {
    const NSc::TValue& cvData = ctx.GetData();
    TStringBuf titleOrName = !cvData.TrySelect(ENTITY_BASE_TITLE).IsNull()
        ? cvData.TrySelect(ENTITY_BASE_TITLE).GetString()
        : cvData.TrySelect(ENTITY_BASE_NAME).GetString();

    NSc::TValue entityObject;
    entityObject["tts"] = entityResults["object"]["tts"];
    entityObject["title"] = titleOrName;
    entityObject["text"] = cvData.TrySelect(ENTITY_DESCRIPTION).GetString();
    entityObject["url"] = AddUtmReferrer(ctx.HandlerContext().MetaClientInfo(), cvData.TrySelect(ENTITY_DESC_URL).GetString());

    const TStringBuf avatar = cvData.TrySelect(ENTITY_AVATAR).GetString();

    entityObject["image"]["src"].SetString(
        avatar.StartsWith('/') ? TStringBuilder() << TStringBuf("https:") << avatar
                               : avatar);
    entityObject["image"]["w"] = 120;
    entityObject["image"]["h"] = 120;

    return entityObject;
}

void TCVAnswerEntitySearch::LoadData() {
    LOG(INFO) << TStringBuf("Loading entity_search_jokes.json") << Endl;

    TString content;
    if (!NResource::FindExact("entity_search_jokes.json", &content)) {
        ythrow yexception() <<
            TStringBuf("Unable to load built-in resource 'entity_search_jokes.json'");
    }

    EntitySearchJokes = NSc::TValue::FromJsonThrow(content);
    LOG(INFO) << TStringBuf("entity_search_jokes.json loaded") << Endl;
}

bool TCVAnswerEntitySearch::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    const NSc::TValue& cvData = ctx.GetData();
    return (ctx.CheckForTagAnswer() &&
            cvData["ObjectResponses"].ArraySize() > 0 &&
            IsValidEntitySearchData(cvData));
}

TStringBuf TCVAnswerEntitySearch::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_ENTITY;
}

void TCVAnswerEntitySearch::Compose(TComputerVisionContext& ctx) const {
    const NSc::TValue& entity = ctx.GetData().TrySelect(ENTITY_ID);
    const NSc::TValue& jokes = EntitySearchJokes[entity.ForceString()];
    const size_t esJokesCount = jokes.ArraySize();
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::TAG_WRONG);
    bool multipleObjectsGenerated = false;
    if (esJokesCount > 0) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_entity_search_joke");
        ctx.AddSimilarsGallery();
        ctx.AttachTextCard();
        const size_t randomJokeNumber = ctx.GetRng().RandomInteger(esJokesCount);
        ctx.Output("random_joke_number") = randomJokeNumber;
        ctx.Output("is_entity_search_joke") = true;
        ctx.Output("easter_egg") = jokes[randomJokeNumber];
        PutEntitySearchResults(ctx, /* fillOnlySlot */ true, multipleObjectsGenerated);
    } else {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_entity_search");
        ctx.Output("is_entity_search").SetBool(true);
        PutEntitySearchResults(ctx, /* fillOnlySlot */ false, multipleObjectsGenerated);
    }
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

void TCVAnswerEntitySearch::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
        {ECbirIntents::CI_CLOTHES,
         ECbirIntents::CI_MARKET,
         ECbirIntents::CI_OCR,
         ECbirIntents::CI_OCR_VOICE,
         ECbirIntents::CI_SIMILAR},
         {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
          NImages::NCbir::ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false);
}

// TCVAnswerPeople ------------------------------------------------------------
bool TCVAnswerPeople::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    return ctx.GetData()["People"].ArraySize() > 0;
}

void TCVAnswerPeople::Compose(TComputerVisionContext& ctx) const {
    const size_t maxPeopleCount = 10;

    NSc::TValue galleryDivCard;
    for (const auto& item : ctx.GetData()["People"].GetArray()) {
        NSc::TValue personItem;
        personItem["link"] = AddUtmReferrer(ctx.HandlerContext().MetaClientInfo(), item["url"]);
        personItem["network"] = item["network"];
        personItem["avatar"] = item["avatar"];
        personItem["name"] = item["name"];
        personItem["age"] = item["age"];
        personItem["geo"] = item["geo"];

        galleryDivCard["people_gallery"].Push(personItem);
        if (galleryDivCard["people_gallery"].ArraySize() >= maxPeopleCount)
            break;
    }
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_people");
    ctx.HandlerContext().AddDivCardBlock(TStringBuf("image__people_gallery"),
                                         std::move(galleryDivCard));
    ctx.Output("has_people_gallery").SetBool(true);
    ctx.AttachTextCard();
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

// TCVAnswerClothes ------------------------------------------------------------
bool TCVAnswerClothes::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    return ctx.GetData()["Crops"].ArraySize() > 0;
}

TStringBuf TCVAnswerClothes::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_CLOTHES;
}

void TCVAnswerClothes::Compose(TComputerVisionContext& ctx) const {
    if (ctx.CheckForTagAnswer()) {
        ctx.AddTagAnswer();
    }
    NSc::TValue crops = MakeClothesCrops(ctx);
    ctx.Output("clothes_crops") = crops;
    // TODO: Remove after deprecation in VINS
    ctx.Output("clothes_original_image") = ctx.GetImageUrl();
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_clothes");

    if (!ctx.AddClothesTabsGallery(std::move(crops))) {
        ctx.AddSimilarsGallery();
    }
    ctx.AttachTextCard();
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::CLOTHES_UNWANTED);
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

NSc::TValue TCVAnswerClothes::MakeClothesCrops(const TComputerVisionContext& ctx) const {
    NSc::TValue crops;
    crops.SetArray();
    for (const auto& item : ctx.GetData()["Crops"].GetArray()) {
        NSc::TValue cropsItem;
        cropsItem["crop_id"] = item["crop_id"];
        cropsItem["category"] = item["category"];
        cropsItem["category_id"] = item["category_id"];
        const NSc::TValue& orig = item["orig"];
        cropsItem["crop_orig"] = SerializeCropBox(orig);
        cropsItem["crop_area"] = (orig["x1"].GetNumber() - orig["x0"].GetNumber())
                                 * (orig["y1"].GetNumber() - orig["y0"].GetNumber());
        TStringBuilder cropUrl;
        cropUrl << TStringBuf("https:") << item["image"].GetString();
        cropsItem["image"] = cropUrl;
        crops.Push(cropsItem);
    }
    return crops;
}

void TCVAnswerClothes::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest({ECbirIntents::CI_OCR, ECbirIntents::CI_SIMILAR, ECbirIntents::CI_OCR_VOICE},
                                        {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false /* showButton */);
}

// TCVAnswerMarket -------------------------------------------------------------
bool TCVAnswerMarket::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    return ctx.CheckForMarketAnswer(/* allowAny */ true);
}

TStringBuf TCVAnswerMarket::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_MARKET;
}

void TCVAnswerMarket::Compose(TComputerVisionContext& ctx) const {
    const double marketTopItemRelevance = ctx.GetData().TrySelect("/Market[0]/relevance").GetNumber();
    constexpr double thresholdMarketCard = 0.8;
    const bool hasMarketCard = marketTopItemRelevance > thresholdMarketCard;
    if (hasMarketCard) {
        AddMarketCard(ctx);
    } else {
        if (ctx.CheckForTagAnswer()) {
            ctx.AddTagAnswer();
        }
    }
    const bool ignoreFirst = hasMarketCard && !(ctx.HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_ALICE) ||
                                                ctx.HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_MARKET) ||
                                                ctx.HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_DOUBLE_BEST_CARDS));

    ctx.AddMarketGallery(/* ignore first*/ ignoreFirst);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_has_market_gallery");
    ctx.AttachTextCard();
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

void TCVAnswerMarket::AddMarketCard(TComputerVisionContext& ctx) const {
    NSc::TValue marketDivCard;

    const auto& marketItem = ctx.GetData()["Market"].GetArray()[0];

    TString markeCard = "";
    if (ctx.HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_ALICE)) {
        markeCard = "market_with_alice";
    } else if (ctx.HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_MARKET)) {
        markeCard = "market_external_link";
    } else if (ctx.HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_EMPTY)) {
        markeCard = "market_empty";
    }

    ctx.FillMarketStars(marketDivCard);
    ctx.FillMarketItem(marketItem, marketDivCard, /* use orig image */ true, /* use image thumb */ false, markeCard);

    ctx.PutTag(marketDivCard["title"], NImages::NCbir::ETagConfidenceCategory::TCC_HIGH);
    ctx.Output("has_market_card") = true;

    ctx.HandlerContext().AddDivCardBlock(TStringBuf("market_card"), std::move(marketDivCard));
}

TStringBuf TCVAnswerMarket::GetCannotApplyMessage() const {
    return TStringBuf("cannot_apply_market");
}

void TCVAnswerMarket::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest({ECbirIntents::CI_CLOTHES, ECbirIntents::CI_OCR, ECbirIntents::CI_OCR_VOICE},
                                        {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false /* showButton */);
}

// TCVAnswerTag ----------------------------------------------------------------
bool TCVAnswerTag::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    return ctx.HasTagAnswer() && (force || ctx.CheckForTagAnswer());
}

TStringBuf TCVAnswerTag::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_TAG;
}

void TCVAnswerTag::Compose(TComputerVisionContext& ctx) const {
    if (ctx.AddTagAnswer()) {
        ctx.AddSimilarsGallery();
        ctx.AttachTextCard();
        ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
        ctx.AttachSimilarSearchSuggest(false /* showButton */);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OFFENSIVE_ANSWER);
    } else {
        ctx.Output("code").SetString("missed_tag");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_missed_tag");
    }
}

// TCVAnswerOcr ----------------------------------------------------------------
bool TCVAnswerOcr::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    return ctx.GetOcrResultCategory() >
        (force ? EOcrResultCategory::ORC_NONE : EOcrResultCategory::ORC_ANY);
}

TStringBuf TCVAnswerOcr::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_OCR;
}

void TCVAnswerOcr::Compose(TComputerVisionContext& ctx) const {
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OCR_POOR);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OCR_TRANSLATE);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OCR_UNWANTED);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_ocr_text");
    ctx.AttachTextCard();
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE, /*force*/ true);

    ctx.AttachUpdateFormSuggest("image_what_is_this__ocr_voice_speech",
                                TComputerVisionOcrVoiceSuggestHandler::FormName());

    NSc::TValue contactsCard = ctx.GetContactsOCR();
    if (!ctx.AddContactsCard(std::move(contactsCard))) {
        ctx.Output("is_ocr_text").SetBool(true);
        if (!ctx.HasExpFlag(NComputerVisionFlags::DISABLE_AUTO_OCR)) {
            ctx.RedirectTo("ocr", "imageocr", /* disable ptr */ true);
        }
    }
}

TStringBuf TCVAnswerOcr::GetCannotApplyMessage() const {
    return TStringBuf("cannot_apply_ocr");
}

void TCVAnswerOcr::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
            {ECbirIntents::CI_CLOTHES, ECbirIntents::CI_MARKET, ECbirIntents::CI_SIMILAR, ECbirIntents::CI_OCR_VOICE},
            {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false /* showButton */);
}

// TCVAnswerFace ---------------------------------------------------------------
bool TCVAnswerFace::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    return ctx.GetData()["Faces"].ArraySize() > 0;
}

TStringBuf TCVAnswerFace::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_FACES;
}

void TCVAnswerFace::Compose(TComputerVisionContext& ctx) const {
    ctx.Output("is_face").SetBool(true);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_face");
    ctx.AddSimilarsGallery();
    ctx.AttachTextCard();
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

void TCVAnswerFace::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
        {ECbirIntents::CI_CLOTHES, ECbirIntents::CI_MARKET, ECbirIntents::CI_OCR, ECbirIntents::CI_SIMILAR, ECbirIntents::CI_OCR_VOICE},
        {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false /* showButton */);
}

// TCVAnswerSimilarGallery ------------------------------------------------------------
bool TCVAnswerSimilarGallery::IsSuitable(const TComputerVisionContext& ctx, bool /* force */) const {
    return ctx.CheckForSimilarAnswer();
}

TStringBuf TCVAnswerSimilarGallery::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_SIMILAR;
}

void TCVAnswerSimilarGallery::Compose(TComputerVisionContext& ctx) const {
    if (ctx.CheckForTagAnswer() && ctx.AddTagAnswer()) {
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OFFENSIVE_ANSWER);
    } else {
        ctx.Output("is_similar").SetBool(true);
        Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_is_similar");
    }
    ctx.AddSimilarsGallery();
    ctx.AttachTextCard();
    ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

void TCVAnswerSimilarGallery::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
        {ECbirIntents::CI_CLOTHES,
         ECbirIntents::CI_MARKET,
         ECbirIntents::CI_OCR,
         ECbirIntents::CI_OCR_VOICE},
        {ECbirIntents::CI_SIMILAR_ARTWORK,
         ECbirIntents::CI_SIMILAR_PEOPLE,
         ECbirIntents::CI_SIMILAR_LIKE,
         ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false /* showButton */);
}

// TCVAnswerIziPushkinMuseum ---------------------------------------------------
bool TCVAnswerIziPushkinMuseum::IsSuitable(const TComputerVisionContext &ctx, bool /* force */) const {
    const NSc::TValue& museumPromo = ctx.GetData().TrySelect(MUSEUM_PROTMO_PATH);
    return !museumPromo.IsNull() && !museumPromo.DictEmpty();
}

void TCVAnswerIziPushkinMuseum::Compose(TComputerVisionContext &ctx) const{
    const NSc::TValue& iziPushkinMuseumPromo = ctx.GetData().TrySelect(MUSEUM_PROTMO_PATH);
    auto& museumOutput = ctx.Output("museum");
    static const TMap<TString, TString> fieldsMap = {{"AliceTitle", "card_title"},
                                                     {"AliceText",  "card_text"},
                                                     {"SourceDescription",  "card_footer"},
                                                     {"AliceVoice", "tts"},
                                                     {"HasAudio", "has_audio"}};
    for (const auto& fieldMap : fieldsMap) {
        museumOutput[fieldMap.second] = iziPushkinMuseumPromo[fieldMap.first];
    }

    NSc::TValue museumCard;
    for (const auto& fieldMap : fieldsMap) {
        museumCard[fieldMap.second] = iziPushkinMuseumPromo[fieldMap.first];
    }

    museumCard["img"]["src"] = iziPushkinMuseumPromo["ImagePreview"];
    museumCard["img"]["w"] = MUSEUM_IMAGE_SIZE.first;
    museumCard["img"]["h"] = MUSEUM_IMAGE_SIZE.second;

    museumCard["card_url"].SetString(iziPushkinMuseumPromo["AliceLink"]);

    ctx.HandlerContext().AddDivCardBlock(TStringBuf("museum_object"),
                                         std::move(museumCard));
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::TAG_WRONG);
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_museum");
}

void TCVAnswerIziPushkinMuseum::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
        {ECbirIntents::CI_SIMILAR,
         ECbirIntents::CI_CLOTHES,
         ECbirIntents::CI_MARKET,
         ECbirIntents::CI_OCR,
         ECbirIntents::CI_OCR_VOICE},
        {ECbirIntents::CI_SIMILAR_LIKE,
         ECbirIntents::CI_INFO});
    ctx.AttachSimilarSearchSuggest(false /* showButton */);
}

// TCVAnswerSimilarPeople ---------------------------------------------------------
bool TCVAnswerSimilarPeople::IsSuitable(const TComputerVisionContext& ctx, bool) const {
    const NSc::TValue& cvData = ctx.GetData();
    return cvData["SimilarPeople"].ArraySize() > 0;
}

void TCVAnswerSimilarPeople::Compose(TComputerVisionContext& ctx) const {
    bool sourceIsFrontal = false;
    const TSlot* slot = ctx.HandlerContext().GetSlot("source_is_frontal", "num");
    if (slot) {
        const NSc::TValue value = slot->Value;
        if (value.IsIntNumber()) {
            sourceIsFrontal = value.GetIntNumber();
        }
    }
    const auto& similarPeople = ctx.GetData()["SimilarPeople"][0];
    ctx.Output("is_similar_people") = true;
    ctx.Output("name") = similarPeople["name"];

    NSc::TValue card;
    card["source_url"] = similarPeople["url"];
    card["title"] = similarPeople["title"];
    card["text"] = similarPeople["text"];
    card["image"] = similarPeople["image"];
    card["image_url"] = similarPeople["image_url"].GetString();
    card["similarity"] = std::lround(similarPeople["similarity"].GetNumber() * 100);
    card["search_url"] = GenerateSearchUri(&ctx.HandlerContext(), similarPeople["name"]);
    ctx.HandlerContext().AddDivCardBlock(TStringBuf("similar_people"), std::move(card));
    ctx.AttachTextCard();
    ctx.AddSimilarsGallery();
    if (sourceIsFrontal) {
        ctx.HandlerContext().AddSuggest("image_what_is_this__similar_people_frontal");
    } else {
        ctx.HandlerContext().AddSuggest("image_what_is_this__similar_people");
    }
    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_similar_people");
}

bool TCVAnswerSimilarPeople::IsSimilarSubstituteAllowed(const NBASS::TComputerVisionContext&) const {
    return true;
}

TStringBuf TCVAnswerSimilarPeople::GetDisableFlag() const {
    return NComputerVisionFlags::DISABLE_SIMILAR_PEOPLE;
}

TStringBuf TCVAnswerSimilarPeople::GetCannotApplyMessage() const {
    return TStringBuf("cannot_apply_similar_people");
}

void TCVAnswerSimilarPeople::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
        {ECbirIntents::CI_SIMILAR, ECbirIntents::CI_CLOTHES, ECbirIntents::CI_MARKET, ECbirIntents::CI_OCR, ECbirIntents::CI_OCR_VOICE},
        {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO},
        {ECbirIntents::CI_SIMILAR_ARTWORK});
}

} // NBASS
