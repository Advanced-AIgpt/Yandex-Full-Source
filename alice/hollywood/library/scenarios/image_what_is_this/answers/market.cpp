#include <alice/hollywood/library/scenarios/image_what_is_this/answers/market.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/tag.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/ocr.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>

#include <util/charset/wide.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/string/subst.h>

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_market";
constexpr TStringBuf SHORT_ANSWER_NAME = "market";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_market";

constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";

TString MakeMarketOfferUrl(const TStringBuf offer, NGeobase::TId region) {
    TStringBuilder marketLink;
    marketLink << TStringBuf("https:") << offer;
    if (marketLink.Contains('?')) {
        marketLink << TStringBuf("&");
    } else {
        marketLink << TStringBuf("?");
    }
    marketLink << TStringBuf("lr=") << region;
    marketLink << TStringBuf("&clid=900");
    return marketLink;
}

TString FixImageShapeInUrl(const TStringBuf imageUrl, TStringBuf shapeTo) {
    static const TStringBuf shapeFrom{"/190x250"};
    TStringBuilder newUrl;
    newUrl << TStringBuf("https:") << imageUrl;
    if (newUrl.EndsWith(shapeFrom)) {
        SubstGlobal(newUrl, shapeFrom, shapeTo, newUrl.find_last_of(TStringBuf("/")));
    }
    return newUrl;
}

TString OpinionsCount(int opinions) {
    int rem10 = opinions % 10;
    int rem100 = opinions % 100;
    TString suffix;

    if ((rem100 >= 10 && rem100 <= 19)
            || (rem10 >= 5 && rem10 <= 9)
            || rem10 == 0) {
        suffix =  "отзывов";
    } else if (rem10 == 1) {
        suffix = "отзыв";
    } else {
        /* if (rem10 >= 2 && rem10 <= 4) */
        suffix = "отзыва";
    }

    return ToString(opinions) + " " + suffix;
}

}

namespace NAlice::NHollywood::NImages::NFlags {

const TString DISABLE_MARKET_INACTIVE_ITEMS = "image_recognizer_disable_market_inactive_items";
const TString CV_EXP_MARKET_CARD_V3_ALICE = "image_recognizer_market_v3_alice";
const TString CV_EXP_MARKET_CARD_V3_MARKET = "image_recognizer_market_v3_market";
const TString CV_EXP_MARKET_CARD_V3_EMPTY = "image_recognizer_market_v3_empty";
const TString CV_EXP_MARKET_CARD_DOUBLE_BEST_CARDS = "image_recognizer_market_double_best_cards";
const TString IMAGE_THUMB_FOR_MARKET_ITEMS = "image_recognizer_image_thumb_for_market";
const TString RANDOM_REARRANGE_MARKET = "image_recognizer_random_rearrange_market";

}

namespace NAlice::NHollywood::NImage::NComputerVisionFeedbackOptions {
const TStringBuf MARKET_LINK = "feedback_negative_images__market_link";
const TStringBuf MARKET_POOR = "feedback_negative_images__market_poor";
const TStringBuf MARKET_UNWANTED = "feedback_negative_images__market_unwanted";
}

using namespace NAlice::NHollywood::NImages;
using namespace NAlice::NHollywood::NImage::NAnswers;

TMarket::TMarket()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    AllowedIntents = {
        ::NImages::NCbir::ECbirIntents::CI_CLOTHES,
        ::NImages::NCbir::ECbirIntents::CI_OCR,
        ::NImages::NCbir::ECbirIntents::CI_OCR_VOICE,
    };

     LastForceAlternativeSuggest = {
         ::NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
         ::NImages::NCbir::ECbirIntents::CI_INFO,
     };

     CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Market;
     Intent = ::NImages::NCbir::ECbirIntents::CI_MARKET;
     IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/15681/icon-market/orig";
     IsSupportSmartCamera = true;

     AliceSmartMode = ALICE_SMART_MODE;
}

TMarket* TMarket::GetPtr() {
    static TMarket* answer = new TMarket;
    return answer;
}

void TMarket::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetImageAliceResponse().Defined()) {
        ctx.AddImageAliceRequest({{"flag", "force_market"}});
    }
}

bool TMarket::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool /* force */) const {
    const TMaybe<NSc::TValue>& imageAliceResponseMaybe = ctx.GetImageAliceResponse();
    if (!imageAliceResponseMaybe.Defined()) {
        return false;
    }
    const NSc::TValue& imageAliceResponse = imageAliceResponseMaybe.GetRef();

    for (const auto& item : imageAliceResponse["Market"].GetArray()) {
        if (CheckMarketItem(ctx, item)) {
            return true;
        }
    }
    return false;
}

void TMarket::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const double marketTopItemRelevance = ctx.GetImageAliceResponse().GetRef().TrySelect("/Market[0]/relevance").GetNumber();
    constexpr double thresholdMarketCard = 0.95;
    const bool hasMarketCard = marketTopItemRelevance > thresholdMarketCard;
    TTag* tagAnswer = TTag::GetPtr();
    if (hasMarketCard) {
        AddMarketCard(ctx);
    } else {
        if (tagAnswer->CheckForTagAnswer(ctx)) {
            tagAnswer->AddTagAnswer(ctx, "render_market_with_tag");
        } else {
            ctx.AddTextCard("render_market_no_tag", {});
        }
    }
    const bool ignoreFirst = hasMarketCard && !(ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_ALICE) ||
                                                ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_MARKET) ||
                                                ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_DOUBLE_BEST_CARDS));

    AddMarketGallery(ctx, /* ignore first*/ ignoreFirst);
    TOcr* ocrAnswer = TOcr::GetPtr();
    ocrAnswer->AddOcrAnswer(ctx, EOcrResultCategory::ORC_VAGUE);
}

bool TMarket::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (ctx.GetImageAliceResponse().Defined()) {
        ctx.AddTextCard("render_cannot_apply_market_error", {});
        return true;
    }
    return false;
}

bool TMarket::CheckMarketItem(TImageWhatIsThisApplyContext& ctx, const NSc::TValue& marketItem) const {
    if (marketItem["market_id"]["type"] == "schema.org") {
        return !marketItem["shop_url"].IsNull() && !marketItem["thmb_href"].IsNull();
    }
    return !marketItem["market_image"].IsNull()
        && (!marketItem["shop_url"].IsNull()
            || (marketItem["market_id"]["type"] != "offer" && !ctx.HasFlag(NFlags::DISABLE_MARKET_INACTIVE_ITEMS)));
}

void TMarket::AddMarketCard(TImageWhatIsThisApplyContext& ctx) const {
    NSc::TValue marketDivCard;

    const auto& marketItem = ctx.GetImageAliceResponse().GetRef()["Market"].GetArray()[0];

    TString markeCard = "";
    if (ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_ALICE)) {
        markeCard = "market_with_alice";
    } else if (ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_MARKET)) {
        markeCard = "market_external_link";
    } else if (ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_EMPTY)) {
        markeCard = "market_empty";
    }

    FillMarketStars(marketDivCard);
    FillMarketItem(ctx, marketItem, marketDivCard, /* use orig image */ true, /* use image thumb */ false, markeCard);

    TTag* tagAnswer = TTag::GetPtr();
    tagAnswer->PutTag(marketDivCard["title"], ::NImages::NCbir::ETagConfidenceCategory::TCC_HIGH, ctx, "render_tag");

    ctx.AddDivCardBlock("market_card", marketDivCard);
}

void TMarket::FillMarketStars(NSc::TValue& divCard) const {
    divCard["NoneIcon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/40186/NoneStar/orig";
    divCard["HalfIcon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/15681/HalfStar/orig";
    divCard["FillIcon"] = "http://avatars.mds.yandex.net/get-images-similar-mturk/15681/FillStar/orig";
}

void TMarket::FillMarketItem(TImageWhatIsThisApplyContext& ctx, const NSc::TValue& item, NSc::TValue &marketItem,
                             bool isOrigImage, bool imageThumb, const TString& cardType) const {
    constexpr size_t marketCardTitleLimit = 40;
    constexpr size_t maxLineLengthCardV3 = 25;
    const bool schemaOrg = item["market_id"]["type"] == "schema.org";

    const NGeobase::TId userRegion = ctx.GetUserRegion();

    if (schemaOrg) {
        marketItem["link"] = item["shop_url"].GetString();
    } else {
        marketItem["link"] = MakeMarketOfferUrl(item["market_link"].GetString(), userRegion);

        if (!cardType.empty()) {
            if (!item["url"].IsNull()) {
                marketItem["link"] = TString("https:") + item["url"].GetString();
            }

            if (item["market_id"]["type"].GetString("") == "model") {
                marketItem["market_type"] = "model";
                marketItem["market_id"] = FromString<ui64>(item["market_id"]["id"]);
            } else if (item["market_id"]["type"].GetString("") == "cluster") {
                marketItem["market_type"] = "model";
                marketItem["market_id"] = FromString<ui64>(item["market_id"]["id"]);
            } else {
                marketItem["market_type"] = "offer";
                marketItem["market_id"] = item["market_id"]["id"];
            }
            marketItem["market_link"] = MakeMarketOfferUrl(item["market_link"].GetString(), userRegion);
        }
    }

    TString title = CutText(TString{item["title"].GetString(" ")}, marketCardTitleLimit);
    if (!cardType.empty() && UTF8ToWide(title).length() < maxLineLengthCardV3) {
        title += "\n ";
    }
    marketItem["title"] = title;

    marketItem["price"] = item["price"];
    marketItem["min_model_price"] = item["min_model_price"];
    marketItem["model_rating"] = item["model_rating"];
    marketItem["is_offer"] = item["market_id"]["type"].GetString() == "offer" || schemaOrg;
    marketItem["currency"] = (item["currency"].IsNull() || item["currency"].GetString() == "RUB") ? "RUR" : item["currency"];
    marketItem["green_url"] = CleanUrl(item["shop_domain"].GetString());


    NSc::TValue modelOpinionsCount = item["model_opinions_count"];
    if (modelOpinionsCount.IsIntNumber()) {
        marketItem["model_opinions_count"] = OpinionsCount(modelOpinionsCount);
    } else if (modelOpinionsCount.IsString()) {
        marketItem["model_opinions_count"] = OpinionsCount(FromString<int>(modelOpinionsCount));
    } else {
        marketItem["model_opinions_count"] = OpinionsCount(0);
    }

    if (schemaOrg) {
        marketItem["image"] = TString("https:") + item["thmb_href"].GetString() + "&n=13";
    } else if (!cardType.empty()) {
        TString imgUrl(item["market_image"].GetString());
        if (imgUrl.find("avatars.mds.yandex.net/get-marketpic") != imgUrl.npos || imgUrl.find("avatars.mds.yandex.net/get-mpic") != imgUrl.npos) {
            if (!imgUrl.StartsWith("http")) {
                imgUrl = TStringBuf("https:") + imgUrl;
            }
            imgUrl = imgUrl.substr(0, imgUrl.rfind('/')) + "/x166_trim";
        }
        marketItem["image"] = imgUrl;
    } else {
        if (imageThumb) {
            marketItem["image"] = TString("https:") + item["thmb_href"].GetString() + "&n=13";
        } else {
            const TStringBuf shapeTo = isOrigImage ? "/orig" : "/9hq";
            marketItem["image"] = FixImageShapeInUrl(item["market_image"].GetString(), shapeTo);
        }
    }
    if (!cardType.empty()) {
        marketItem["market_card_type"] = cardType;
    }
}

NSc::TValue TMarket::MakeMarketGallery(TImageWhatIsThisApplyContext& ctx, const NSc::TValue& visionData,
                                       bool ignoreFirst, bool appendTailLink, int cropId) const {
    constexpr size_t marketGallerySizeLimit = 10;
    const bool imageThumb = ctx.HasFlag(NFlags::IMAGE_THUMB_FOR_MARKET_ITEMS);

    NSc::TValue galleryDivCard;

    // Fill stars icons for market
    FillMarketStars(galleryDivCard);

    size_t count = 0;
    bool isFirst = true;

    TString markeCard = "";
    if (ctx.HasFlag(NFlags::CV_EXP_MARKET_CARD_V3_ALICE)) {
        markeCard = "market_with_alice";
    } else if (ctx.HasFlag(NFlags::CV_EXP_MARKET_CARD_V3_MARKET)) {
        markeCard = "market_external_link";
    } else if (ctx.HasFlag(NFlags::CV_EXP_MARKET_CARD_V3_EMPTY)) {
        markeCard = "market_empty";
    }

    for (const auto& item : visionData["Market"].GetArray()) {
        if (isFirst && ignoreFirst) {
            isFirst = false;
            continue;
        }
        if (CheckMarketItem(ctx, item)) {
            NSc::TValue marketItem;
            FillMarketItem(ctx, item, marketItem, /* use orig image*/ false, imageThumb, markeCard);

            galleryDivCard["market_gallery"].Push(marketItem);
            if (++count >= marketGallerySizeLimit) {
                if (appendTailLink) {
                    galleryDivCard["tail_link"] = ctx.GenerateMarketDealsLink(cropId);
                }
                break;
            }
        }
    }

    if (ctx.HasFlag(NFlags::RANDOM_REARRANGE_MARKET)) {
        auto& marketGallery = galleryDivCard["market_gallery"].GetArrayMutable();
        ui64 seed = ctx.GetContext().Rng.RandomInteger();
        Shuffle(marketGallery.begin(), marketGallery.end(), TReallyFastRng32(seed));
    }
    return galleryDivCard;
}

void TMarket::AddMarketGallery(TImageWhatIsThisApplyContext& ctx, bool ignoreFirst) const {
    auto galleryDivCard = MakeMarketGallery(ctx, ctx.GetImageAliceResponse().GetRef(), ignoreFirst, /* appendTailLink */ true);
    galleryDivCard["new_market_flag"] = true;
    TStringBuf marketGalleryCard = "image__market_gallery";
    if (ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_ALICE) ||
            ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_MARKET) ||
            ctx.HasFlag(NImages::NFlags::CV_EXP_MARKET_CARD_V3_EMPTY)) {
        marketGalleryCard = TStringBuf("image__market_gallery_v3");
    }
    if (!galleryDivCard["market_gallery"].GetArray().empty()) {
        ctx.AddDivCardBlock(marketGalleryCard, galleryDivCard);
    }

    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::MARKET_POOR);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::MARKET_UNWANTED);
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::MARKET_LINK);
}
