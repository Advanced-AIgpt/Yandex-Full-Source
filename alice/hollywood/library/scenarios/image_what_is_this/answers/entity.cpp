#include <alice/hollywood/library/scenarios/image_what_is_this/answers/entity.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/tag.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similars.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/image_what_is_this_resources.h>

#include <alice/library/url_builder/url_builder.h>

#include <library/cpp/uri/uri.h>
#include <library/cpp/resource/resource.h>
#include <kernel/urlnorm/normalize.h>

using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace NAlice::NHollywood::NImage::NFlags {

const TString ENABLE_MULTI_ENTITY_OBJECTS = "image_multi_entity_objects";

}

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

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_entity";
constexpr TStringBuf SHORT_ANSWER_NAME = "entity";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_entity";

constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";

}

TEntity::TEntity()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    Intent = NImages::NCbir::ECbirIntents::CI_ENTITY;
    AllowedIntents = {
        NImages::NCbir::ECbirIntents::CI_CLOTHES,
        NImages::NCbir::ECbirIntents::CI_MARKET,
        NImages::NCbir::ECbirIntents::CI_OCR,
        NImages::NCbir::ECbirIntents::CI_OCR_VOICE,
        NImages::NCbir::ECbirIntents::CI_SIMILAR,
    };

     LastForceAlternativeSuggest = {
         NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
         NImages::NCbir::ECbirIntents::CI_INFO,
     };

     AliceSmartMode = ALICE_SMART_MODE;
}


bool TEntity::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool /*force*/) const {
    const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
    if (!imageAliceResponseMaybe.Defined()) {
        return false;
    }

    const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();

    // TODO: Exp tags no
    TTag* tagAnswer = TTag::GetPtr();
    return (tagAnswer->CheckForTagAnswer(ctx)
            && imageAliceResponse["ObjectResponses"].ArraySize() > 0
            && IsValidEntitySearchData(imageAliceResponse));
}

void TEntity::AppendFeedbackOptions(TImageWhatIsThisApplyContext& ctx) const {
    IAnswer::AppendFeedbackOptions(ctx);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::TAG_WRONG);
}

TEntity* TEntity::GetPtr() {
    static TEntity* answer = new TEntity;
    return answer;
}

bool TEntity::IsValidEntitySearchData(const NSc::TValue& cvData) const {
    for (const auto field : {ENTITY_AVATAR, ENTITY_BASE_NAME, ENTITY_BASE_TITLE,
                             ENTITY_DESC_URL, ENTITY_DESCRIPTION,
                             ENTITY_ID, ENTITY_REQUEST, ENTITY_URL, ENTITY_VOICE})
    {
        if (cvData.TrySelect(field).IsNull()) {
            return false;
        }
    }
    return true;
}

void TEntity::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const NSc::TValue& entitySearchJokes = ctx.GetResources().GetEntitySearchJokes();
    const NSc::TValue& cvData = ctx.GetImageAliceResponse().GetRef();
    const NSc::TValue& entity = cvData.TrySelect(ENTITY_ID);
    const NSc::TValue& jokes = entitySearchJokes[entity.ForceString()];
    const size_t esJokesCount = jokes.ArraySize();
    bool multipleObjectsGenerated = false;
    if (esJokesCount > 0) {
        ctx.StatIncCounter("hollywood_computer_vision_result_answer_entity_search_joke");

        const size_t randomJokeNumber = ctx.GetScenarioHandleContext().Rng.RandomInteger(esJokesCount);
        NSc::TValue card;
        card["random_joke_number"] = randomJokeNumber;
        card["easter_egg"] = jokes[randomJokeNumber];
        ctx.AddTextCard("render_entity_search_joke", card);
        ctx.GetAnalyticsInfoBuilder().AddObject("easter_egg", "easter_egg", TString(jokes[randomJokeNumber].GetString()));

        TSimilars* answerSimilars = TSimilars::GetPtr();
        answerSimilars->AddSimilarsGallery(ctx);
        PutEntitySearchResults(ctx, /* fillOnlySlot */ true, multipleObjectsGenerated);
    } else {
        ctx.StatIncCounter("hollywood_computer_vision_result_answer_entity_search");
        PutEntitySearchResults(ctx, /* fillOnlySlot */ false, multipleObjectsGenerated);
    }
    ctx.GetAnalyticsInfoBuilder().AddObject("entity", "entity", TString(entity.GetString()));
    TOcr* ocrAnswer = TOcr::GetPtr();
    ocrAnswer->AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE);
}

void TEntity::PutEntitySearchResults(TImageWhatIsThisApplyContext& ctx, bool fillOnlySlot, bool &multipleObjectsGenerated) const {
    static const TStringBuf slotLabel("render_entity");
    const TStringBuf divCardLabel = "new_image_search_object";

    const NSc::TValue& cvData = ctx.GetImageAliceResponse().GetRef();
    if (cvData["ObjectResponses"].GetArray().size() <= 1 || !ctx.HasFlag(NFlags::ENABLE_MULTI_ENTITY_OBJECTS)) {
        const TStringBuf entityName0 = cvData.TrySelect(ENTITY_REQUEST).GetString();
        TStringBuilder text;
        text << entityName0 << TStringBuf(" — ") << cvData.TrySelect(ENTITY_DESCRIPTION).GetString();

        NSc::TValue entityResults;
        entityResults["object"]["text"].SetString(text);
        entityResults["object"]["tts"].SetString(cvData.TrySelect(ENTITY_VOICE).GetString());
        entityResults["object"]["url"].SetString(NAlice::AddUtmReferrer(ctx.GetClientInfo(), cvData.TrySelect(ENTITY_URL).GetString()));
        entityResults["serp"]["url"].SetString(ctx.GenerateSearchUri(entityName0));
        const TString imageSearchUrl = ctx.GenerateImagesSearchUrl("similar", TStringBuf("imageview"),
                                                                   /* disable ptr */ false, "similar");
        entityResults["image_serp"]["url"].SetString(imageSearchUrl);

        if (!fillOnlySlot) {
            NSc::TValue entityObject = CreateEntitySearchObject(ctx, entityResults);
            ctx.AddDivCardBlock(divCardLabel, std::move(entityObject));
            ctx.AddTextCard(slotLabel, std::move(entityResults));
        }
        // TODO Why we save "search_results"?
        //ctx.HandlerContext().CreateSlot(slotLabel, slotLabel, /* optional */ true, std::move(entityResults));
        // TODO:
        //ctx.AttachUtteranceSuggest(entityName0);
        ctx.AddSearchSuggest(entityName0);
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
            galleryItem["link"] = AddUtmReferrer(ctx.GetClientInfo(), url);
            galleryItem["title"] = entityName;
            galleryItem["html_host"] = urlBeautified;

            if (mainOnto.empty()) {
                mainOnto = entityName;
            }

            galleryDivCard["similar_gallery"].Push(std::move(galleryItem));
        }
        ctx.AddDivCardBlock(TStringBuf("image__onto_gallery"), std::move(galleryDivCard));
        multipleObjectsGenerated = true;

        if (!mainOnto.empty()) {
            NSc::TValue tagData;
            tagData["tag_confidence"] = ToString(NImages::NCbir::ETagConfidenceCategory::TCC_HIGH);
            ctx.AddTextCard(TStringBuf("tag"), tagData);
        }
    }
}

NSc::TValue TEntity::CreateEntitySearchObject(const TImageWhatIsThisApplyContext& ctx,
                                                                   const NSc::TValue& entityResults) const {
    const NSc::TValue& cvData = ctx.GetImageAliceResponse().GetRef();
    TStringBuf titleOrName = !cvData.TrySelect(ENTITY_BASE_TITLE).IsNull()
        ? cvData.TrySelect(ENTITY_BASE_TITLE).GetString()
        : cvData.TrySelect(ENTITY_BASE_NAME).GetString();

    NSc::TValue entityObject;
    entityObject["tts"] = entityResults["object"]["tts"];
    entityObject["title"] = titleOrName;
    entityObject["text"] = cvData.TrySelect(ENTITY_DESCRIPTION).GetString();
    const TString url = AddUtmReferrer(ctx.GetRequest().ClientInfo(), cvData.TrySelect(ENTITY_DESC_URL).GetString());
    entityObject["url"] = url;
    entityObject["hostname"] = GetHostname(url);

    const TStringBuf avatar = cvData.TrySelect(ENTITY_AVATAR).GetString();

    entityObject["image"]["src"].SetString(
        avatar.StartsWith('/') ? TStringBuilder() << TStringBuf("https:") << avatar
                               : avatar);
    entityObject["image"]["w"] = 120;
    entityObject["image"]["h"] = 120;

    return entityObject;
}
