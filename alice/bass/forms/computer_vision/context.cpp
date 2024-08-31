#include "context.h"
#include <alice/bass/forms/computer_vision/handler.h>
#include <alice/bass/forms/computer_vision/similarlike_cv_handler.h>

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/scheme/util/scheme_holder.h>
#include <library/cpp/uri/uri.h>

#include <util/charset/wide.h>
#include <util/digest/city.h>
#include <util/generic/algorithm.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>


namespace NBASS {

namespace NComputerVisionFlags {
    const TStringBuf ENABLE_SNIPPET_INSTEAD_TITLE = "image_recognizer_snippet_instead_title";
    const TString USE_TAG_NO_TEMPLATE = "image_recognizer_use_tag_no_";
    const TStringBuf RANDOM_REARRANGE_MARKET = "image_recognizer_random_rearrange_market";
    const TStringBuf DISABLE_MARKET_INACTIVE_ITEMS = "image_recognizer_disable_market_inactive_items";
    const TStringBuf IMAGE_THUMB_FOR_MARKET_ITEMS = "image_recognizer_image_thumb_for_market";
    const TStringBuf ALLOW_EXTENDED_CAMERA_MODES = "image_recognizer_allow_extended_camera_modes";
    const TStringBuf FIX_OFFICE_LENS_SCAN_TIME = "image_recognizer_fix_office_lens_scan_time";
    extern const TStringBuf DISABLE_TAG;

    //CV-1503
    const TStringBuf CV_EXP_MARKET_CARD_V3_ALICE = "image_recognizer_market_v3_alice";
    const TStringBuf CV_EXP_MARKET_CARD_V3_MARKET = "image_recognizer_market_v3_market";
    const TStringBuf CV_EXP_MARKET_CARD_V3_EMPTY = "image_recognizer_market_v3_empty";
    const TStringBuf CV_EXP_MARKET_CARD_DOUBLE_BEST_CARDS = "image_recognizer_market_double_best_cards";

    // CV-1524
    const TStringBuf CV_EXP_OFFICE_LENS_CROP = "image_recognizer_office_lens_crop";
    const TStringBuf CV_EXP_OFFICE_LENS_BUTTONS = "image_recognizer_enable_office_lens_buttons";
}

namespace NComputerVisionFeedbackOptions {

const TStringBuf BARCODE_WRONG = "feedback_negative_images__barcode_wrong";
const TStringBuf BARCODE_UNWANTED = "feedback_negative_images__barcode_unwanted";
const TStringBuf CLOTHES_UNWANTED = "feedback_negative_images__clothes_unwanted";
const TStringBuf MARKET_LINK = "feedback_negative_images__market_link";
const TStringBuf MARKET_POOR = "feedback_negative_images__market_poor";
const TStringBuf MARKET_UNWANTED = "feedback_negative_images__market_unwanted";
const TStringBuf OCR_POOR = "feedback_negative_images__ocr_poor";
const TStringBuf OCR_TRANSLATE = "feedback_negative_images__ocr_translate";
const TStringBuf OCR_UNWANTED = "feedback_negative_images__ocr_unwanted";
const TStringBuf TAG_WRONG = "feedback_negative_images__tag_wrong";
const TStringBuf UNSIMILAR = "feedback_negative_images__unsimilar";
const TStringBuf USELESS = "feedback_negative_images__useless";
const TStringBuf OFFENSIVE_ANSWER = "feedback_negative__offensive_answer";
const TStringBuf OTHER = "feedback_negative__other";

} // NComputerVisionFeedbackOptions

namespace {

void GatherComputerVisionSourceMetrics(const NSc::TValue& computerVisionAnswer) {
    Y_STATS_INC_COUNTER("bass_computer_vision_source_answer_received");
    if (computerVisionAnswer["Tags"].ArraySize()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_source_tags_received");
    }
    if (computerVisionAnswer["Market"].ArraySize()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_source_market_received");
    }
    if (computerVisionAnswer["People"].ArraySize()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_source_people_received");
    }
    if (computerVisionAnswer["Similars"].ArraySize()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_source_similars_received");
    }
    if (computerVisionAnswer["ObjectResponses"].ArraySize()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_source_objectresponses_received");
    }
    if (computerVisionAnswer["FastOcr"].Has("fulltext")) {
        Y_STATS_INC_COUNTER("bass_computer_vision_source_fastocr_has_fulltext");
        if (computerVisionAnswer["FastOcr"]["fulltext"].ArraySize()) {
            Y_STATS_INC_COUNTER("bass_computer_vision_source_fastocr_fulltext_received");
        }
    }
}

void LogIntentFactors(const NSc::TValue& cvAnswer) {
    const NSc::TValue& factors = cvAnswer["IntentFactors"];
    if (factors.ArraySize() == 0) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_factors_empty");
        return;
    }
    TStringBuilder serializedFactors;

    for (const auto& factor : factors.GetArray()) {
        const TStringBuf name = factor["name"].GetString();
        const double value = factor["value"].GetNumber(-1.);
        if (name.empty() || value < 0.) {
            Y_STATS_INC_COUNTER("bass_computer_vision_result_error_factors_broken");
            LOG(DEBUG) << TStringBuf("[CV FACTORS ERROR]") << factors << Endl;
            return;
        }
        serializedFactors << name << ':' << value << ';';
    }
    LOG(INFO) << TStringBuf("[CV FACTORS]") << serializedFactors << Endl;
}

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

TString MakeImageUrl(const TStringBuf thumb) {
    TStringBuilder image;
    image << TStringBuf("https:") << thumb;
    if (image.Contains('?')) {
        image << TStringBuf("&");
    } else {
        image << TStringBuf("?");
    }
    image << TStringBuf("n=13");
    return image;
}

NSc::TValue GetFirstMatchedById(const TContext& context) {
    const TContext::TSlot* slotAnswer = context.GetSlot("answer");
    const TContext::TSlot* slotSelectedClothesCategoryId =
        context.GetSlot("selected_clothes_category_id");

    if (slotSelectedClothesCategoryId && slotAnswer) {
        const NSc::TValue& answerValue = slotAnswer->Value;
        const NSc::TValue& cropValue = slotSelectedClothesCategoryId->Value;
        if (cropValue.Has("category_id")) {
            for (const auto& item : answerValue["clothes_crops"].GetArray()) {
                if (item["category_id"] == cropValue["category_id"]) {
                    return item;
                }
            }
        }
    }
    return NSc::TValue();
}

TCgiParameters CommonHandlesCgiParams(TContext& ctx) {
    const auto& meta = ctx.Meta();
    TCgiParameters result;
    result.InsertUnescaped(TStringBuf("ll"), TStringBuilder() << meta.Location().Lon() << ','
                                                               << meta.Location().Lat());
    result.InsertUnescaped(TStringBuf("lr"), ToString(ctx.UserRegion()));
    result.InsertUnescaped(TStringBuf("service"), TStringBuf("assistant.yandex"));
    result.InsertUnescaped(TStringBuf("ui"), ctx.MetaClientInfo().GetSearchRequestUI());

    if (meta.HasUUID()) {
        result.InsertUnescaped(TStringBuf("uuid"), meta.UUID());
    }
    if (meta.HasYandexUID()) {
        result.InsertUnescaped(TStringBuf("yandexuid"), meta.YandexUID());
    }
    return result;
}

NSc::TValue GetAsValue(const TString& str) {
    return (str.size() > 0) ? NSc::TValue(str) : NSc::TValue::Null();
}

TVector<std::pair<float, float>> LoadCropCoordinates(const TString& coordinatesStr) {
    TVector<std::pair<float, float>> coordinates;
    TVector<TString> items;
    Split(coordinatesStr, ";", items);
    if (items.size() % 2  != 0) {
        return coordinates;
    }
    for (size_t i = 0; i < items.size(); i += 2) {
        coordinates.push_back({FromString<float>(items[i]), FromString<float>(items[i + 1])});
    }
    return coordinates;
}

} // namespace

// TComputerVisionContext ------------------------------------------------------
TComputerVisionContext::TComputerVisionContext(const IComputerVisionBaseHandler& handler,
                                               TContext& ctx,
                                               const TInstant& deadline)
    : Handler(handler)
    , Context(ctx)
    , Deadline(deadline)
    , Rng(ctx.GetRng())
{
    const TContext::TSlot* slotAnswer = ctx.GetSlot("answer");
    if (nullptr != slotAnswer && !slotAnswer->Value.IsNull()) {
        CbirId = slotAnswer->Value["cbir_id"].GetString();
        ImageUrl = slotAnswer->Value["query_url"].GetString();
    }

    for (int i = 1; i < 11; ++i) {
        const TString tagFlagWithNo = NComputerVisionFlags::USE_TAG_NO_TEMPLATE + ToString(i);
        if (Context.HasExpFlag(tagFlagWithNo)) {
            UsedTagNo = i - 1;
            break;
        }
    }
    UsedTagPath = "/Tags[" + ToString(UsedTagNo) + "]";
}

TString TComputerVisionContext::GenerateImagesSearchUrl(const TStringBuf aliceSource, const TStringBuf report,
                                                        bool disablePtr, const TStringBuf cbirPage) const {
    return GenerateSimilarImagesSearchUrl(Context, CbirId, aliceSource, cbirPage, report, /* needNextPage */ false, disablePtr);
}

TString TComputerVisionContext::GenerateMarketDealsLink(int cropId) const {
    return GenerateSimilarImagesSearchUrl(Context, CbirId, /* aliceSource */ cropId >= 0 ? "clothes" : "market",
                                          /* cbirPage */ "products", /* report */ "imageview",
                                          /* needNextPage */ false, /* disablePtr */ false, cropId);
}

ECaptureMode TComputerVisionContext::GetCaptureMode() {
    ECaptureMode captureMode = ECaptureMode::PHOTO;
    const NSc::TValue& imageRequest = *Context.Meta().UtteranceData().GetRawValue();
    if (imageRequest["data"].Has("capture_mode")) {
        const TString& captureModeStr = imageRequest["data"]["capture_mode"].ForceString();
        TryFromString<ECaptureMode>(captureModeStr, captureMode);
    }
    if (captureMode == ECaptureMode::MARKET) {
        const TMaybe<TStringBuf> subCaptureMode = GetSubCaptureModeSlot();
        if (subCaptureMode.Defined() && subCaptureMode == ToString(ECaptureMode::CLOTHES)) {
            captureMode = ECaptureMode::CLOTHES;
        }
    }
    else if (captureMode == ECaptureMode::OCR) {
        const TMaybe<TStringBuf> subCaptureMode = GetSubCaptureModeSlot();
        if (subCaptureMode.Defined() && subCaptureMode == ToString(ECaptureMode::TRANSLATE)) {
            captureMode = ECaptureMode::TRANSLATE;
        }
    } else if (captureMode == ECaptureMode::PHOTO) {
        const TMaybe<TStringBuf> subCaptureMode = GetSubCaptureModeSlot();
        if (subCaptureMode.Defined()) {
            TryFromString<ECaptureMode>(subCaptureMode.GetRef(), captureMode);
        }
    }

    HandlerContext().CreateSlot("subcapture_mode", "string", true, NSc::TValue(ToString(ECaptureMode::PHOTO)));

    return captureMode;
}

void TComputerVisionContext::FillCaptureMode(ECaptureMode captureMode, NSc::TValue& payload) {
    switch (captureMode) {
        case ECaptureMode::DOCUMENT:
        case ECaptureMode::OCR:
        case ECaptureMode::OCR_VOICE:
        case ECaptureMode::MARKET:
        case ECaptureMode::PHOTO:
            payload["image_search_mode_name"] = ToString(captureMode);
            break;
        case ECaptureMode::CLOTHES:
            payload["image_search_mode_name"] = ToString(ECaptureMode::MARKET);
            break;
        case ECaptureMode::TRANSLATE:
            payload["image_search_mode_name"] = ToString(ECaptureMode::OCR);
            break;
        default:
            payload["image_search_mode_name"] = ToString(ECaptureMode::PHOTO);
            break;
    }
}

NHttpFetcher::THandle::TRef TComputerVisionContext::MakeMarketRequest(
    const NSc::TValue& data,
    NHttpFetcher::IMultiRequest::TRef& requests) const
{
    TCgiParameters marketCgi = CommonHandlesCgiParams(Context);
    marketCgi.InsertUnescaped(TStringBuf("avatarurl"),
                              data.Has("query_url") ? data["query_url"].GetString() : ImageUrl);
    marketCgi.InsertUnescaped(TStringBuf("crop"),
                              data["crop_orig"].GetString());
    marketCgi.InsertUnescaped(TStringBuf("flag"),
                              TStringBuf("images_alice_response=da"));
    marketCgi.InsertUnescaped(TStringBuf("region"),
                              ToString(Context.UserRegion()));

    NHttpFetcher::TRequestPtr req = Context.GetSources().ComputerVisionClothes().AttachRequest(requests);
    req->AddCgiParams(marketCgi);
    req->AddHeader("User-Agent", Context.MetaClientInfo().UserAgent);
    return req->Fetch();
}

NHttpFetcher::THandle::TRef TComputerVisionContext::MakeSimilarsRequest(
    const NSc::TValue& data,
    NHttpFetcher::IMultiRequest::TRef& requests) const
{
    TCgiParameters similarsCgi = CommonHandlesCgiParams(Context);
    similarsCgi.InsertUnescaped(TStringBuf("url"), data["query_url"].GetString());
    similarsCgi.InsertUnescaped(TStringBuf("crop"), data["crop_orig"].GetString());

    NHttpFetcher::TRequestPtr req = Context.GetSources().ComputerVision().AttachRequest(requests);
    req->AddCgiParams(similarsCgi);
    req->AddHeader("User-Agent", Context.MetaClientInfo().UserAgent);
    return req->Fetch();
}

bool TComputerVisionContext::HasTagAnswer() const {
    return VisionData["Tags"].ArraySize() > UsedTagNo;
}

bool TComputerVisionContext::CheckForTagAnswer() const {
    const auto thresholdTagMinimum = HasExpFlag(NComputerVisionFlags::ENABLE_COMMON_TAGS)
        ? NImages::NCbir::ETagConfidenceCategory::TCC_TINY
        : NImages::NCbir::ETagConfidenceCategory::TCC_LOW;

    return (HasTagAnswer()
            && !HasExpFlag(NComputerVisionFlags::DISABLE_TAG)
            && (GetTagConfidenceCategory() >= thresholdTagMinimum));
}

bool TComputerVisionContext::CheckForSimilarAnswer() const {
    return (VisionData["Similars"].ArraySize() > 0 && CbirId);
}

bool TComputerVisionContext::CheckForMarketAnswer(bool allowAny) const {
    constexpr double thresholdMarket = 0.3;

    if (VisionData["Market"].ArraySize() > 0 &&
        (allowAny || VisionData.TrySelect("/Market[0]/relevance").GetNumber() > thresholdMarket))
    {
        for (const auto& item : VisionData["Market"].GetArray()) {
            if (CheckMarketItem(item)) {
                return true;
            }
        }
    }
    return false;
}

bool TComputerVisionContext::CheckMarketItem(const NSc::TValue& marketItem) const {
    if (marketItem["market_id"]["type"] == "schema.org") {
        return !marketItem["shop_url"].IsNull() && !marketItem["thmb_href"].IsNull();
    }
    return !marketItem["market_image"].IsNull()
        && (!marketItem["shop_url"].IsNull()
            || (marketItem["market_id"]["type"] != "offer" && !HasExpFlag(NComputerVisionFlags::DISABLE_MARKET_INACTIVE_ITEMS)));
}

EOcrResultCategory TComputerVisionContext::GetOcrResultCategory() const {
    constexpr size_t minWordsCountVague = 2;
    constexpr size_t minWordsCountCertain = 8;
    constexpr size_t minLinesCountCertain = 3;

    if (!CbirId) {
        return ORC_NONE;
    }
    const NSc::TValue& textLines = VisionData["FastOcr"]["fulltext"];
    size_t wordsCount = 0;
    const TUtf16String wideDelim(u" ");
    const TSetDelimiter<const TChar> delim(wideDelim.data());
    for (const auto& item : textLines.GetArray()) {
        const auto text = UTF8ToWide(item["Text"].ForceString());
        TVector<TUtf16String> tokens;
        TContainerConsumer<TVector<TUtf16String>> consumer(&tokens);
        SplitString(text.begin(), text.end(),  delim, consumer);
        wordsCount += tokens.size();
        if (wordsCount >= minWordsCountCertain) {
            return (textLines.ArraySize() >= minLinesCountCertain) ? ORC_CERTAIN : ORC_VAGUE;
        }
    }
    if (wordsCount >= minWordsCountVague) {
        return ORC_VAGUE;
    }
    return (wordsCount > 0) ? ORC_ANY : ORC_NONE;
}

bool TComputerVisionContext::AddClothesTabsGallery(NSc::TValue foundCrops) {
    Y_ASSERT(foundCrops.IsArray() && foundCrops.ArraySize() > 0);
    auto& crops = foundCrops.GetArrayMutable();
    SortBy(crops.rbegin(),
           crops.rend(),
           [](const NSc::TValue& item){ return item["crop_area"].GetNumber(); });

    TVector<NHttpFetcher::THandle::TRef> requests;
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
    for (const auto& crop : crops) {
        requests.emplace_back(MakeMarketRequest(crop, multiRequest));
    }
    multiRequest->WaitAll(Deadline);

    NSc::TValue result;
    auto& resultTabs = result.GetArrayMutable();
    for (size_t cropIdx = 0; cropIdx < requests.size(); ++cropIdx) {
        NHttpFetcher::TResponse::TRef marketResponse = requests[cropIdx]->Wait(Deadline);
        if (!marketResponse->IsHttpOk()) {
            Y_STATS_INC_COUNTER("bass_computer_vision_result_error_clothes_market_fetching_error");
            continue;
        }
        NSc::TValue response;
        if (!NSc::TValue::FromJson(response, marketResponse->Data)) {
            Y_STATS_INC_COUNTER("bass_computer_vision_result_error_clothes_market_json_parse_error");
            continue;
        }

        NSc::TValue tabItem;
        tabItem["category"] = crops[cropIdx]["category"];
        tabItem["gallery"] = MakeMarketGallery(response, /* ignoreFirst */ false,
                                               /* appendTailLink */ true, crops[cropIdx]["crop_id"]);
        if (tabItem["gallery"]["market_gallery"].ArraySize() > 0) {
            resultTabs.emplace_back(std::move(tabItem));
        }
    }

    if (result.ArraySize() == 0) {
        return false;
    }
    HandlerContext().AddDivCardBlock(TStringBuf("image__clothes_tabs_gallery"), std::move(result));
    return true;
}

NSc::TValue TComputerVisionContext::MakeMarketGallery(const NSc::TValue& visionData,
                                                      bool ignoreFirst,
                                                      bool appendTailLink,
                                                      int cropId) const {
    constexpr size_t marketGallerySizeLimit = 10;
    const bool imageThumb = HasExpFlag(NComputerVisionFlags::IMAGE_THUMB_FOR_MARKET_ITEMS);

    NSc::TValue galleryDivCard;
    if (const TAvatar* avatar = Context.Avatar(TStringBuf("computer_vision"), TStringBuf("market"))) {
        galleryDivCard["market_buy_ico"].SetString(avatar->Https);
    }

    // Fill stars icons for market
    FillMarketStars(galleryDivCard);

    size_t count = 0;
    bool isFirst = true;

    TString markeCard = "";
    if (HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_ALICE)) {
        markeCard = "market_with_alice";
    } else if (HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_MARKET)) {
        markeCard = "market_external_link";
    } else if (HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_EMPTY)) {
        markeCard = "market_empty";
    }

    for (const auto& item : visionData["Market"].GetArray()) {
        if (isFirst && ignoreFirst) {
            isFirst = false;
            continue;
        }
        if (CheckMarketItem(item)) {
            NSc::TValue marketItem;
            FillMarketItem(item, marketItem, /* use orig image*/ false, imageThumb, markeCard);

            galleryDivCard["market_gallery"].Push(marketItem);
            if (++count >= marketGallerySizeLimit) {
                if (appendTailLink) {
                    galleryDivCard["tail_link"] = GenerateMarketDealsLink(cropId);
                }
                break;
            }
        }
    }

    if (HasExpFlag(NComputerVisionFlags::RANDOM_REARRANGE_MARKET)) {
        auto& marketGallery = galleryDivCard["market_gallery"].GetArrayMutable();
        ui64 seed = CityHash64(CbirId);
        Shuffle(marketGallery.begin(), marketGallery.end(), TReallyFastRng32(seed));
    }
    return galleryDivCard;
}

void TComputerVisionContext::AddMarketGallery(bool ignoreFirst) {
    auto galleryDivCard = MakeMarketGallery(VisionData, ignoreFirst, /* appendTailLink */ true);
    galleryDivCard["new_market_flag"] = true;
    TStringBuf marketGalleryCard = "image__market_gallery";
    if (HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_ALICE) ||
            HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_MARKET) ||
            HasExpFlag(NComputerVisionFlags::CV_EXP_MARKET_CARD_V3_EMPTY)) {
        marketGalleryCard = TStringBuf("image__market_gallery_v3");
    }
    HandlerContext().AddDivCardBlock(marketGalleryCard, std::move(galleryDivCard));
    Result["has_market_gallery"].SetBool(true);

    AppendFeedbackOption(NComputerVisionFeedbackOptions::MARKET_POOR);
    AppendFeedbackOption(NComputerVisionFeedbackOptions::MARKET_UNWANTED);
    AppendFeedbackOption(NComputerVisionFeedbackOptions::MARKET_LINK);
}

void TComputerVisionContext::FillMarketItem(const NSc::TValue& item, NSc::TValue &marketItem,
                                            bool isOrigImage, bool imageThumb, const TString& cardType) const {
    constexpr size_t marketCardTitleLimit = 40;
    constexpr size_t maxLineLengthCardV3 = 25;
    const bool schemaOrg = item["market_id"]["type"] == "schema.org";

    if (schemaOrg) {
        marketItem["link"] = item["shop_url"].GetString();
    } else {
        marketItem["link"] = MakeMarketOfferUrl(item["market_link"].GetString(),
                                                Context.UserRegion());
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

            marketItem["market_link"] = MakeMarketOfferUrl(item["market_link"].GetString(),
                                                           Context.UserRegion());
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
    marketItem["model_opinions_count"] = item["model_opinions_count"];
    marketItem["is_offer"] = item["market_id"]["type"].GetString() == "offer" || schemaOrg;
    marketItem["currency"] = (item["currency"].IsNull() || item["currency"].GetString() == "RUB") ? "RUR" : item["currency"];
    marketItem["green_url"] = CleanUrl(item["shop_domain"].GetString());

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

void TComputerVisionContext::FillMarketStars(NSc::TValue& divCard) const {
    divCard["FillIcon"].SetString(Context.Avatar("poi", "Fill")->Https);
    divCard["HalfIcon"].SetString(Context.Avatar("poi", "Half")->Https);
    divCard["NoneIcon"].SetString(Context.Avatar("poi", "None")->Https);
}

bool TComputerVisionContext::AddSimilarsGallery() {
    constexpr size_t similarsGallerySizeLimit = 10;

    if (!CheckForSimilarAnswer()) {
        return false;
    }

    NSc::TValue galleryDivCard;
    size_t count = 0;
    for (const auto& item : VisionData["Similars"].GetArray()) {
        NSc::TValue galleryItem;
        galleryItem["image"] = MakeImageUrl(item["thmb_href"].GetString());
        galleryItem["link"].SetString(
                GenerateSimilarsGalleryLink(HandlerContext(), CbirId, item["url"].GetString()));
        if (HandlerContext().HasExpFlag(NComputerVisionFlags::ENABLE_SNIPPET_INSTEAD_TITLE)) {
            galleryItem["title"] = item["text"].GetString();
        } else {
            galleryItem["title"] = item["title"].GetString();
        }
        galleryItem["html_href"] = item["html_href"].GetString();
        galleryItem["html_host"] = CleanUrl(item["html_host"].GetString());
        galleryDivCard["similar_gallery"].Push(galleryItem);
        count += 1;
        if (count >= similarsGallerySizeLimit) {
            galleryDivCard["tail_link"] = GenerateImagesSearchUrl("similar", "imageview", false, "similar");
            break;
        }
    }
    HandlerContext().AddDivCardBlock(TStringBuf("image__similar_gallery_v2"), std::move(galleryDivCard));
    AppendFeedbackOption(NComputerVisionFeedbackOptions::UNSIMILAR);
    return true;
}

NImages::NCbir::ETagConfidenceCategory TComputerVisionContext::GetTagConfidenceCategory() const {
    NImages::NCbir::ETagConfidenceCategory tagConfidence;
    return TryFromString(VisionData.TrySelect("/TagConfidence").GetString(), tagConfidence)
        ? tagConfidence
        : NImages::NCbir::ETagConfidenceCategory::TCC_NONE;
}

bool TComputerVisionContext::AddTagAnswer() {
    const auto& tag = VisionData.TrySelect(UsedTagPath + "[0]");
    if (tag.IsNull()) {
        return false;
    }
    const auto& confidence = GetTagConfidenceCategory();
    if (NImages::NCbir::ETagConfidenceCategory::TCC_NONE == confidence) {
        return false;
    }

    PutTag(tag, confidence);

    return true;
}

void TComputerVisionContext::PutTag(const NSc::TValue& tag, NImages::NCbir::ETagConfidenceCategory confidence) {
    static const TStringBuf slotLabel("search_results");

    Result["tag"] = tag;
    Result["tag_confidence"] = ToString(confidence);
    AttachUtteranceSuggest(tag.GetString());
    AppendFeedbackOption(NComputerVisionFeedbackOptions::TAG_WRONG);

    NSc::TValue searchResults;
    searchResults["serp"]["url"].SetString(GenerateSearchUri(&HandlerContext(), tag));
    HandlerContext().CreateSlot(slotLabel, slotLabel, /* optional */ true, std::move(searchResults));

    Y_STATS_INC_COUNTER(TStringBuilder() << TStringBuf("bass_computer_vision_result_answer_tag_")
                                         << Result["tag_confidence"]);

}

void TComputerVisionContext::AttachSearchSuggest() {
    if (CheckForTagAnswer() && !VisionData.TrySelect(UsedTagPath + "[0]").IsNull()) {
        const TString searchSuggestText = VisionData["Tags"][UsedTagNo][0].ForceString();
        NSc::TValue searchSuggestContent;
        searchSuggestContent["uri"] = GenerateSearchUri(&HandlerContext(), searchSuggestText);
        searchSuggestContent["text"] = searchSuggestText;
        HandlerContext().AddSuggest(TStringBuf("image_what_is_this__open_search"),
                                    std::move(searchSuggestContent));
    }
}

void TComputerVisionContext::AttachFeedbackSuggest(const TStringBuf feedbackTypeSuffix) {
    TStringBuilder feedbackType;
    feedbackType << TStringBuf("feedback_negative_") << feedbackTypeSuffix;
    NSc::TValue formUpdate;
    formUpdate["name"] = TStringBuilder()
        << TStringBuf("personal_assistant.feedback.")
        << ((feedbackTypeSuffix.at(0) == '_')
            ? feedbackType
            : TStringBuf("feedback_negative_images__confirm"));
    HandlerContext().AddSuggest(feedbackType, NSc::Null(), formUpdate);
}

void TComputerVisionContext::AttachUtteranceSuggest(const TStringBuf text) {
    NSc::TValue suggestContent;
    suggestContent["text"] = text;
    HandlerContext().AddSuggest(TStringBuf("image_what_is_this__search_tag"),
                                std::move(suggestContent));
}

void TComputerVisionContext::AttachCustomUriSuggest(const TStringBuf suggestType,
                                                    const TStringBuf uri,
                                                    const TStringBuf label) {
    NSc::TValue suggestContent;
    suggestContent["uri"].SetString(uri);
    if (!label.Empty()) {
        suggestContent["label"].SetString(label);
    }
    HandlerContext().AddSuggest(suggestType, std::move(suggestContent));
}

void TComputerVisionContext::AttachUpdateFormSuggest(const TStringBuf suggestType, const TStringBuf formName, const TStringBuf label) {
    NSc::TValue formUpdate;
    formUpdate["name"] = formName;

    NSc::TValue data;
    if (!label.empty()) {
        data["label"] = label;
    }

    HandlerContext().AddSuggest(suggestType, data, formUpdate);
}

TString TComputerVisionContext::AttachOcrSuggest(const TStringBuf label) {
    const TString ocrUrl = GenerateImagesSearchUrl("ocr", TStringBuf("imageocr"), /* disable ptr */ true);
    AttachCustomUriSuggest(TStringBuf("image_what_is_this__open_ocr_result"), ocrUrl, label);
    return ocrUrl;
}

void TComputerVisionContext::RedirectTo(const TStringBuf aliceSource, const TStringBuf report, bool disablePtr) {
    NSc::TValue link;
    link["uri"].SetString(GenerateImagesSearchUrl(aliceSource, report, disablePtr));
    HandlerContext().AddCommand<TCVRedirectDirective>(TStringBuf("open_uri"), std::move(link));
}

NJson::TJsonValue TComputerVisionContext::RequestDiskInfo(const TString& path) const {
    NHttpFetcher::TRequestPtr req = HandlerContext().GetSources().CloudApiDisk().Request();

    req->AddHeader("X-Ya-User-Ticket", UserTicket);

    TCgiParameters cgi;
    cgi.InsertUnescaped("path", path);
    req->AddCgiParams(cgi);

    NHttpFetcher::THandle::TRef ref = req->Fetch();

    NHttpFetcher::TResponse::TRef resp = ref->Wait(Deadline);
    if (resp->IsError()) {
        LOG(DEBUG) << "Image recognizer request disk info error. " << resp->GetErrorText() << Endl;
        return NJson::TJsonValue();
    }

    return NJson::ReadJsonFastTree(resp->Data);
}

bool TComputerVisionContext::RequestDiskCreateDir(const TString& path) const {
    NHttpFetcher::TRequestPtr req = HandlerContext().GetSources().CloudApiDisk().Request();

    req->SetMethod("PUT");
    req->AddHeader("X-Ya-User-Ticket", UserTicket);

    TCgiParameters cgi;
    cgi.InsertUnescaped("path", path);
    req->AddCgiParams(cgi);

    NHttpFetcher::THandle::TRef ref = req->Fetch();

    NHttpFetcher::TResponse::TRef resp = ref->Wait(Deadline);

    if (resp->IsError()) {
        LOG(DEBUG) << "Image recognizer request disk create dir error. " << resp->GetErrorText() << Endl;
        return false;
    }
    return true;
}

bool TComputerVisionContext::RequestDiskPutFile(const TString& path, const TStringBuf imageUrl) const {
    NHttpFetcher::TRequestPtr req = HandlerContext().GetSources().CloudApiDiskUpload().Request();

    req->AddHeader("X-Ya-User-Ticket", UserTicket);

    TCgiParameters cgi;
    cgi.InsertUnescaped("url", imageUrl);
    cgi.InsertUnescaped("path", path);
    req->AddCgiParams(cgi);
    req->SetMethod("POST");

    NHttpFetcher::THandle::TRef ref = req->Fetch();
    NHttpFetcher::TResponse::TRef resp = ref->Wait(Deadline);
    if (resp->IsError()) {
        LOG(DEBUG) << "Image recognizer request disk put error. " << resp->GetErrorText() << Endl;
        return false;
    }
    return true;
}

bool TComputerVisionContext::AddOcrAnswer(const EOcrResultCategory minToSuggest, bool force) {
    const EOcrResultCategory ocrResult = GetOcrResultCategory();
    if (ocrResult > EOcrResultCategory::ORC_NONE) {
        if (ocrResult >= minToSuggest && force) {
            AttachOcrSuggest();
        }
        Result["fast_ocr"] = ToString(ocrResult);
        return true;
    }
    return false;
}

void TComputerVisionContext::AttachAlternativeIntentsSuggest(
    const TSet<NImages::NCbir::ECbirIntents>& allowedIntents,
    const TVector<NImages::NCbir::ECbirIntents>& forcedIntents,
    const TVector<NImages::NCbir::ECbirIntents>& firstForcedIntents)
{
    NSc::TValue intentsButtonsDivCard;
    auto addIntent = [&intentsButtonsDivCard, this] (NImages::NCbir::ECbirIntents intent) {
        if (!AnswerAlternatives.contains(intent)) {
            return;
        }
        NSc::TValue descr = AnswerAlternatives.at(intent).GetAnswerSwitchingDescriptor(*this);
        if (descr.IsNull()) {
            return;
        }

        auto& context = descr["context"].SetDict();
        context["cbir_id"].SetString(CbirId);
        context["query_url"].SetString(ImageUrl);
        HandlerContext().AddSuggest(descr["id"].GetString());
        intentsButtonsDivCard.Push(std::move(descr));
    };

    for (const auto& intent : firstForcedIntents) {
        addIntent(intent);
    }
    for (const auto& intent : CbirIntents) {
        if (allowedIntents.contains(intent)) {
            addIntent(intent);
        }
    }
    for (const auto& intent : forcedIntents) {
        addIntent(intent);
    }
    if (!intentsButtonsDivCard.IsNull() && Handler.ShouldShowIntentButtons())
    {
        NSc::TValue intentsDivCard;
        intentsDivCard["buttons"] = intentsButtonsDivCard;
        HandlerContext().AddDivCardBlock(TStringBuf("image__alternative_intents"),
                                         std::move(intentsDivCard));
    }
}

void TComputerVisionContext::AttachSimilarSearchSuggest(bool showButton) {
    if (CbirId) {
        HandlerContext().AddSuggest(TStringBuf("image_what_is_this__similar"));
        if (showButton) {
            AttachCustomUriSuggest(TStringBuf("image_what_is_this__open_similar_search"),
                                   GenerateImagesSearchUrl("similar", "imageview", false, "similar"));
        }
    }
}

bool TComputerVisionContext::CheckAndSetImageUrl(ECaptureMode captureMode, bool frontalCamera) {
    const NSc::TValue& imageRequest = *Context.Meta().UtteranceData().GetRawValue();
    if (!imageRequest["data"].Has("img_url")) {
        Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
        NSc::TValue payload;
        if (frontalCamera) {
            payload["camera_type"] = "front";
        } else {
            payload = NSc::Null();
        }
        if (!frontalCamera) {
            if (HasExpFlag(NComputerVisionFlags::ALLOW_EXTENDED_CAMERA_MODES)) {
                payload["image_search_mode"] = static_cast<int>(captureMode);
            } else {
                payload["image_search_mode"] = (captureMode == ECaptureMode::DOCUMENT ? 1 : 0);
            }
            FillCaptureMode(captureMode, payload);
        }
        HandlerContext().CreateSlot("subcapture_mode", "string", true, NSc::TValue(ToString(captureMode)));
        HandlerContext().CreateSlot("source_is_frontal", "num", true, NSc::TValue(frontalCamera));
        HandlerContext().AddCommand<TCVImageRecognizerOnCameraDirective>(TStringBuf("start_image_recognizer"), payload);

        const TContext::TSlot* isSilentSlot = Context.GetSlot("silent_mode", "num");
        if (isSilentSlot && isSilentSlot->Value.GetIntNumber()) {
            HandlerContext().AddAutoactionDelayMsBlock(0);
        }

        return false;
    }
    ImageUrl = imageRequest["data"]["img_url"].ForceString();
    SourceImageUrl = imageRequest["data"]["source_image_url"].GetString("");
    CropCoordinates = LoadCropCoordinates(TString(imageRequest["data"]["crop_coordinates"].GetString("")));

    NUri::TUri uri;
    NUri::TState::EParsed uriResult =
        uri.Parse(ImageUrl, NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);

    if (NUri::TState::EParsed::ParsedOK != uriResult) {
        Result["code"].SetString("bad_img_url");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_bad_img_url");
    } else {
        if (TStringBuf("avatars.mds.yandex.net") == uri.GetHost()) {
            return true;
        } else {
            Result["code"].SetString("img_url_from_forbidden_host");
            Y_STATS_INC_COUNTER("bass_computer_vision_result_error_img_url_from_forbidden_host");
        }
    }
    return false;
}

TString TComputerVisionContext::CutText(const TString& text, size_t len) const {
    TVector<TString> words;
    Split(text, " ", words);
    TString cuttedText;
    size_t currentLen = 0;
    auto endWord = words.begin();
    for ( ; endWord != words.end(); ++endWord) {
        size_t wordLen = GetNumberOfUTF8Chars(*endWord);
        if (currentLen + wordLen <= len) {
            if (currentLen != 0) {
                currentLen += 1;
            }
            currentLen += wordLen;
        } else {
            break;
        }
    }
    cuttedText = JoinRange(" ", words.begin(), endWord);

    return cuttedText;
}

TString TComputerVisionContext::CutTexts(const NSc::TArray& texts, size_t len, bool appendEllipsis) const {
    TVector<TString> sentences;
    TString currentText;
    size_t currentLen = 0;
    for (size_t i = 0; i != texts.size(); ++i) {
        TString originalText = TString{texts[i].GetString()};
        if (originalText.empty()) {
            continue;
        }
        TString addedText = CutText(originalText, len - currentLen);

        if (!addedText.empty()) {
            sentences.push_back(addedText);
            currentLen += GetNumberOfUTF8Chars(addedText);
        }

        if (addedText.empty()
                || addedText.size() < originalText.size()) {
            if (appendEllipsis && !sentences.empty()) {
                if (!sentences.back().empty() && sentences.back().back() == '.') {
                    sentences.back() += "..";
                } else {
                    sentences.back() += "...";
                }
            }
            break;
        }
    }
    return JoinRange(" ", sentences.begin(), sentences.end());
}

TStringBuf TComputerVisionContext::CleanUrl(const TStringBuf& url) const {
    TStringBuf cleanedUrl = CutSchemePrefix(url);
    const size_t wwwLen = 4;
    if (cleanedUrl.StartsWith("www.")) {
        cleanedUrl = cleanedUrl.Tail(wwwLen);
    }

    return cleanedUrl;
}

void TComputerVisionContext::DecreaseSentencesLength(TString& text) {
    const TString delimiters = ".!?";
    TFindFirstOf<const char> splitFinder(delimiters.Data());
    const size_t maxSentenceSize = 500;

    // Splits sentences every 500 characters by dots because Speechkit can not read long sentences now.
    size_t currentStringPosition = 0;
    while (currentStringPosition < text.size()) {
        const char* sentencePtr = text.begin() + currentStringPosition;
        const char* sentenceEnd = splitFinder.FindFirstOf(sentencePtr, text.end());
        size_t sentenceSize = sentenceEnd - sentencePtr;

        if (sentenceSize <= maxSentenceSize) {
            currentStringPosition += sentenceSize + 1;
            continue;
        }

        TStringBuf subSentence(sentencePtr, maxSentenceSize);

        size_t splitPos = subSentence.rfind(" ");
        if (splitPos == TStringBuf::npos) {
            currentStringPosition += sentenceSize + 1;
            continue;
        }

        text.insert(currentStringPosition + splitPos, ".");
        currentStringPosition = currentStringPosition + splitPos + 2;
    }
}

bool TComputerVisionContext::IsContainsEnoughLongWords(const NSc::TArray& texts, size_t wordLength, int enoughWords, const TString& delimeters) {
    int countLongWords = 0;
    TFindFirstOf<const char> splitFinder(delimeters.Data());

    // Checks whether is text contains words count that is longer than word length
    for (const auto& text : texts) {
        const TString textStr = TString{text.GetString()};
        const char* currentStringPosition = textStr.begin();
        const char* lastStringPosition = textStr.end();
        while (currentStringPosition < lastStringPosition) {
            const char* delimPosition = splitFinder.FindFirstOf(currentStringPosition, lastStringPosition);
            size_t wordSize = 0;
            if (GetNumberOfUTF8Chars(currentStringPosition, delimPosition - currentStringPosition, wordSize)) {
                if (wordSize >= wordLength) {
                    ++countLongWords;
                    if (countLongWords >= enoughWords) {
                        return true;
                    }
                }
            }
            currentStringPosition = delimPosition + 1;
        }
    }

    return false;
}

bool TComputerVisionContext::IsObscene(const TUtf16String& word, const THashSet<TUtf16String>& swearWords) {
    TUtf16String lowerWord(word);
    lowerWord.to_lower();
    return swearWords.contains(lowerWord);
}

bool TComputerVisionContext::FixSwear(const NSc::TArray& texts, const THashSet<TUtf16String>& swearWords,
                                      NSc::TValue& result) {
    TString delimiters = ".,; \n";
    const int enoughLongWords = 10;
    const size_t longWordSize = 3;
    bool isContainsEnoughLongWords = IsContainsEnoughLongWords(texts, longWordSize, enoughLongWords, delimiters);

    TFindFirstOf<const char> splitFinder(delimiters.Data());
    const TString replaceString = "***";

    NSc::TArray& resultTextsArray = result.SetArray().GetArrayMutable();

    // Checks every word on obscene. If the text is short, then we refuses to read. Otherwise, we replace these word by '***' in text
    // and by empty string in speech text
    for (const auto& text : texts) {
        TString textStr = TString{text.GetString()};

        size_t currentStringPosition = 0;
        while (currentStringPosition < textStr.size()) {
            const char* currentStartString = textStr.begin() + currentStringPosition;
            const char* delimPosition = splitFinder.FindFirstOf(currentStartString, textStr.end());

            const size_t wordSize = delimPosition - currentStartString;

            TStringBuf word(currentStartString, wordSize);
            if (!word.empty()) {
                TUtf16String wideWord = UTF8ToWide(word);
                if (IsObscene(wideWord, swearWords)) {
                    if (isContainsEnoughLongWords) {
                        textStr.replace(currentStringPosition, wordSize, replaceString);
                        currentStringPosition += 4;
                    } else {
                        return true;
                    }
                } else {
                    currentStringPosition += word.size() + 1;
                }
            } else {
                currentStringPosition += 1;
            }
        }
        resultTextsArray.push_back(TStringBuf(textStr));
    }

    return false;
}

bool TComputerVisionContext::ReplaceAsterisks(NSc::TArray &texts)
{
    bool hasAnyText = false;
    for (auto& text : texts) {
        TUtf16String textStr = UTF8ToWide(TString{text.GetString()});
        size_t replacedCount = SubstGlobal(textStr, u"***", u"");
        if (replacedCount) {
            text.SetString(WideToUTF8(textStr));
        }
        if (!hasAnyText) {
            for (const auto c: textStr) {
                if (IsAlnum(c)) {
                    hasAnyText = true;
                }
            }
        }
    }

    return hasAnyText;
}


void TComputerVisionContext::Switch(const TStringBuf formName) {
    SwitchedContext.Reset(
        Context.SetResponseForm(formName, /* setCurrentFormAsCallback */ false));
    Y_ENSURE(SwitchedContext);
    Result.Clear();
}

NSc::TValue TComputerVisionContext::ExtractEllipsisInfo() const {
    const TContext& context = HandlerContext();
    const TContext::TSlot* slotAnswer = context.GetSlot("answer");
    const TContext::TSlot* slotSelectedClothesCrop =
        context.GetSlot("selected_clothes_crop");

    NSc::TValue cropInfo;
    NSc::TValue ellipsisInfo;
    if (nullptr != slotAnswer && slotAnswer->Value.Has("query_url")) {
        ellipsisInfo["query_url"] = slotAnswer->Value["query_url"];
    }
    if (nullptr != slotSelectedClothesCrop) {
        cropInfo.CopyFrom(slotSelectedClothesCrop->Value);
        if (cropInfo.Has("clothes_original_image")) {
            ellipsisInfo["query_url"] = cropInfo["clothes_original_image"];
        }
    } else {
        cropInfo = GetFirstMatchedById(context);
    }

    if (cropInfo.Has("crop_orig") &&
        ellipsisInfo.Has("query_url"))
    {
        ellipsisInfo["crop_orig"] = cropInfo["crop_orig"];
    }
    return ellipsisInfo;
}

TResultValue TComputerVisionContext::RequestClothesEllipsis() {
    const NSc::TValue ellipsisInfo = ExtractEllipsisInfo();
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
    NHttpFetcher::THandle::TRef marketRequest =
        MakeMarketRequest(ellipsisInfo, multiRequest);
    NHttpFetcher::THandle::TRef similarsRequest =
        MakeSimilarsRequest(ellipsisInfo, multiRequest);

    multiRequest->WaitAll(Deadline);
    NHttpFetcher::TResponse::TRef marketResponse = marketRequest->Wait(Deadline);
    NHttpFetcher::TResponse::TRef similarsResponse = similarsRequest->Wait(Deadline);
    if (!marketResponse->IsHttpOk()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_market_fetching_error");
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << TStringBuf("CV market error: ")
                                       << marketResponse->GetErrorText());
    } else if (!similarsResponse->IsHttpOk()) {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_similars_fetching_error");
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << TStringBuf("CV similars error: ")
                                       << similarsResponse->GetErrorText());
    }
    NSc::TValue result;
    if (!NSc::TValue::FromJson(result, similarsResponse->Data) ||
        !NSc::TValue::MergeUpdateJson(result, marketResponse->Data))
    {
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_computer_vision_json_parse_error");
        return TError(TError::EType::IMAGEERROR,
                      TStringBuf("ComputerVision answer error: cannot parse JSON"));
    }
    VisionData.Swap(result);
    SetCbirId();
    SetCbirIntents();
    return TResultValue();
}

void TComputerVisionContext::AddClothesEllipsisAnswer() {
    if (CheckForMarketAnswer(/* allowAny */ false)) {
        AddMarketGallery();
    } else {
        LOG(DEBUG) << TStringBuf("No suitable market results") << Endl;
        if (!AddSimilarsGallery()) {
            Result["code"].SetString("computer_vision_ellipsis_no_gallery");
        }
    }
    AttachTextCard();
    AttachAlternativeIntentsSuggest({NImages::NCbir::ECbirIntents::CI_OCR,
                                     NImages::NCbir::ECbirIntents::CI_SIMILAR},
                                    {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
    AttachSimilarSearchSuggest(false /* showButton */);
}

void TComputerVisionContext::AppendContact(const NSc::TValue& value, const EContactType type, NSc::TArray& contacts) const {
    NSc::TValue cardContact;
    cardContact["value"] = value;
    cardContact["type"] = ToString(type);
    if (type == EContactType::CT_MAIL) {
        cardContact["uri"] = TString("mailto:") + value.GetString();
    } else if (type == EContactType::CT_PHONE) {
        cardContact["uri"] = GeneratePhoneUri(HandlerContext().MetaClientInfo(), value);
    } else if (type == EContactType::CT_URL) {
        const TStringBuf url = value.GetString();
        cardContact["uri"] = (url.StartsWith("http://") || url.StartsWith("https://"))
                             ? url
                             : TString("http://") + url;
    } else {
        Y_FAIL("Unsupported contact type!");
    }
    contacts.emplace_back(std::move(cardContact));
}

bool TComputerVisionContext::AddContactsCard(NSc::TValue card) {
    const NSc::TArray& contacts = card["contacts"].GetArray();
    if (contacts.empty()) {
        return false;
    }
    if (contacts.size() == 1) {
        const EContactType contactType = FromString(contacts.front()["type"].GetString());
        if (contactType == EContactType::CT_URL) {
            Output("is_url_contact").SetBool(true);
        } else if (contactType == EContactType::CT_PHONE) {
            Output("is_phone_contact").SetBool(true);
        } else if (contactType == EContactType::CT_MAIL) {
            Output("is_email_contact").SetBool(true);
        }
    } else {
        Output("is_contacts").SetBool(true);
    }
    HandlerContext().AddDivCardBlock(TStringBuf("ocr_contact"), std::move(card));
    return true;
}

NSc::TValue TComputerVisionContext::GetContactsOCR() const {
    static const TMap<TStringBuf, EContactType> metaToContactType = {
        {TStringBuf("url-eng-lexicon"), EContactType::CT_URL},
        {TStringBuf("phones-lexicon"), EContactType::CT_PHONE},
        {TStringBuf("email-eng-lexicon"), EContactType::CT_MAIL},
    };

    NSc::TValue card;
    NSc::TArray& cardContacts = card["contacts"].SetArray().GetArrayMutable();
    for (const auto& ocrBlock : GetData().TrySelect("/FastOcr")["blocks"].GetArray()) {
        for (const auto& ocrBox : ocrBlock["boxes"].GetArray()) {
            for (const auto& ocrLangBox : ocrBox["languages"].GetArray()) {
                for (const auto& ocrText : ocrLangBox["texts"].GetArray()) {
                    for (const auto& ocrWord : ocrText["words"].GetArray()) {
                        const TStringBuf meta = ocrWord["meta"].GetString();
                        if (!metaToContactType.contains(meta)) {
                            continue;
                        }
                        AppendContact(ocrWord["word"], metaToContactType.at(meta), cardContacts);
                        if (cardContacts.size() == 3) {
                            return card;
                        }
                    }
                }
            }
        }
    }
    return cardContacts.size() > 0 ? card : NSc::Null();
}

bool TComputerVisionContext::AddOcrVoiceAnswer(const THashSet<TUtf16String>& swearWords) {
    NSc::TValue text;
    NSc::TArray& texts = text.SetArray().GetArrayMutable();
    TVector<TStringBuf> blockTexts;
    bool isHypPrevBlock = false;
    bool hasAnyText = false;
    for (const auto& ocrBlock : OcrData["ocr"]["data"]["blocks"].GetArray()) {
        if (!isHypPrevBlock) {
            blockTexts.clear();
        }
        const TStringBuf language = ocrBlock["lang"].GetString();
        const TStringBuf type = ocrBlock["type"].GetString();
        hasAnyText |= type == "main";
        // There is "bak" language since it occurs very often in russian texts
        if ((language != "rus" && language != "bak" && language != "eng")
            || type != "main") {
            continue;
        }
        for (const auto &ocrBox : ocrBlock["boxes"].GetArray()) {
            for (const auto &ocrLangBox : ocrBox["languages"].GetArray()) {
                for (const auto &ocrText : ocrLangBox["texts"].GetArray()) {
                    TStringBuf text = ocrText["text"];
                    if (text.empty()) {
                        continue;
                    }

                    bool isHypCurrentBlock = text.back() == '-';
                    if (isHypCurrentBlock) {
                        text = text.Head(text.size() - 1);
                    }

                    if (isHypPrevBlock) {
                        blockTexts.push_back(text);
                    } else {
                        if (!blockTexts.empty()) {
                            blockTexts.push_back(TStringBuf(" "));
                        }
                        blockTexts.push_back(text);
                    }

                    isHypPrevBlock = isHypCurrentBlock;
                }
            }
        }
        if (!isHypPrevBlock) {
            TString joinedText = JoinSeq("", blockTexts);
            DecreaseSentencesLength(joinedText);
            texts.push_back(TStringBuf(joinedText));
        }
    }

    if (isHypPrevBlock) {
        TString joinedText = JoinSeq("", blockTexts);
        DecreaseSentencesLength(joinedText);
        texts.push_back(TStringBuf(joinedText));
    }

    NSc::TValue filteredTexts;
    if (FixSwear(texts, swearWords, filteredTexts)) {
        LOG(DEBUG) << TStringBuf("Ocr voice swear") << Endl;;
        Result["ocr_voice_swear"] = true;
        return true;
    }

    if (texts.empty()) {
        if (!hasAnyText) {
            LOG(DEBUG) << TStringBuf("No ocr voice text") << Endl;;
            Result["ocr_voice_no_text"] = true;
            return false;
        } else {
            LOG(DEBUG) << TStringBuf("Foreign ocr voice text") << Endl;;
            Result["is_ocr_foreign_text"] = true;
            return true;
        }
    }

    constexpr int textLen = 50;
    TString outText = CutTexts(filteredTexts, textLen, true);

    Result["text"] = outText;
    if (!ReplaceAsterisks(filteredTexts.GetArrayMutable())) {
        LOG(DEBUG) << TStringBuf("No ocr voice text") << Endl;;
        Result["ocr_voice_no_text"] = true;
        return false;
    }
    Result["voice"] = filteredTexts;
    Result["is_ocr_voice"] = true;

    return true;
}

void TComputerVisionContext::AddOfficeLensAnswer() {
    if (OcrData["cbirdaemon"]["enhancedDocumentImage"].Front()["sizes"].IsNull()) {
        return;
    }

    const auto& officeLensInfo = OcrData["cbirdaemon"]["enhancedDocumentImage"].Front()["sizes"];
    if (!officeLensInfo.Has("orig") || !officeLensInfo.Has("preview")) {
        return;
    }

    NSc::TValue card;
    Output("is_office_lens") = true;
    const auto& documentEdges = OcrData["cbirdaemon"]["documentEdges"].Front();
    TStringBuilder coordinates;
    if (!documentEdges.IsNull()) {
        const auto& points = documentEdges["coordinates"].GetArray();
        Y_ENSURE(points.size() >= 4);
        for (const auto& point : points) {
            Y_ENSURE(point.ArraySize() == 2);
            if (!coordinates.empty()) {
                coordinates << "; ";
            }
            coordinates << point[0].GetNumber() << ";" << point[1].GetNumber();
        }
        if (!coordinates.empty()) {
            Output("crop_coordinates") = coordinates;
        }
    }

    TString currentTime;
    if (!HasExpFlag(NComputerVisionFlags::FIX_OFFICE_LENS_SCAN_TIME)) {
        currentTime = Context.Now().FormatLocalTime("%d %m %y %H:%M");
    } else {
        currentTime = "fixed_time";
    }
    const TString filename = "scan" + currentTime + ".jpg";
    TString downloadImage = TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString();
    card["preview_image"] = TString("https://avatars.mds.yandex.net") + officeLensInfo["preview"]["path"].GetString();
    card["full_image"] = TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString();

    HandlerContext().CreateSlot("download_image", "string", true, NSc::TValue(downloadImage));
    HandlerContext().AddDivCardBlock(TStringBuf("new_office_lens"), std::move(card));

    NSc::TValue buttonsCard;
    const bool isEnoughVersionOfAndroid = Context.ClientFeatures().IsAndroidAppOfVersionOrNewer(9, 80);
    const bool isPp = Context.ClientFeatures().IsSearchApp();
    if (isPp && isEnoughVersionOfAndroid && HasExpFlag(NComputerVisionFlags::CV_EXP_OFFICE_LENS_CROP) && !coordinates.empty()) {
        NSc::TValue cropButton;
        cropButton["id"] = "image_what_is_this__office_lens_crop";
        cropButton["camera_open"] = true;
        auto& context = cropButton["context"].SetDict();
        context["cbir_id"].SetString(CbirId);
        context["query_url"].SetString(ImageUrl);
        context["crop_coordinates"] = coordinates;
        if (HasExpFlag(NComputerVisionFlags::ALLOW_EXTENDED_CAMERA_MODES)) {
            context["image_search_mode"] = static_cast<int>(ECaptureMode::DOCUMENT);
        } else {
            context["image_search_mode"] = 1;
        }
        context["image_search_mode_name"] = ToString(ECaptureMode::DOCUMENT);
        buttonsCard.Push(cropButton);
    }

    NSc::TValue downloadButton;
    downloadButton["id"] = "image_what_is_this__office_lens_download";
    downloadButton["external_url"] = TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString() + "?download=scan.jpg";
    buttonsCard.Push(downloadButton);
    if (Context.ClientFeatures().IsSearchApp()) {
        NSc::TValue diskButton;
        diskButton["id"] = "image_what_is_this__office_lens_disk";
        auto& context = diskButton["context"].SetDict();
        context["cbir_id"].SetString(CbirId);
        context["query_url"].SetString(ImageUrl);

        buttonsCard.Push(diskButton);
    }

    NSc::TValue buttonsDivCard;
    buttonsDivCard["buttons"] = buttonsCard;
    buttonsDivCard["disable_title"] = true;

    HandlerContext().AddDivCardBlock(TStringBuf("image__alternative_intents"), std::move(buttonsDivCard));

}

void TComputerVisionContext::OutputResult() {
    if (Result.IsNull()) {
        return;
    }
    if (Result.Has("code")) {
        HandlerContext().AddErrorBlock(TError(TError::EType::IMAGEERROR,
                                              TStringBuf("image_recognizer_error")),
                                       Result);
    }
    Result["cbir_id"] = GetAsValue(CbirId);
    Result["query_url"] = GetAsValue(ImageUrl);
    AppendSpecialButtons();
    HandlerContext().CreateSlot(TStringBuf("answer"),
                                TStringBuf("image_result"),
                                /* optional */ true,
                                std::move(Result));
}

void TComputerVisionContext::AppendFeedbackOption(const TStringBuf optionName) {
    NSc::TValue option;
    option.SetString(optionName);
    NegativeFeedbackOptions.SetArray().Push(std::move(option));
}

void TComputerVisionContext::AppendSpecialButtons() {
    static const TStringBuf buttonType(TStringBuf("feedback_negative"));
    if (!NegativeFeedbackOptions.GetArray().empty()) {
        NSc::TValue data;
        data["sub_list"].Swap(NegativeFeedbackOptions);
        HandlerContext().AddSpecialButtonBlock(buttonType, std::move(data));
    }
}

NHttpFetcher::THandle::TRef TComputerVisionContext::MakeComputerVisionCommonRequest(NHttpFetcher::IMultiRequest::TRef requests,
                                                                                    const TStringBuf additionalFlag) const {
    TCgiParameters cgi = CommonHandlesCgiParams(Context);
    cgi.InsertUnescaped(TStringBuf("flag"), TStringBuf("images_alice_intent_factors"));
    cgi.InsertUnescaped(TStringBuf("url"), ImageUrl);
    if (!additionalFlag.Empty()) {
        cgi.InsertUnescaped(TStringBuf("flag"), additionalFlag);
    }

    NHttpFetcher::TRequestPtr req = Context.GetSources().ComputerVision().AttachRequest(requests);
    req->AddCgiParams(cgi);
    req->AddHeader("User-Agent", Context.MetaClientInfo().UserAgent);

    TStringBuilder cookies;
    for (const auto& cookie : Context.Meta().Cookies()) {
        cookies << TStringBuf("; ") << cookie.Get();
    }
    req->AddHeader(TStringBuf("Cookie"), cookies);

    return req->Fetch();
}

NHttpFetcher::THandle::TRef TComputerVisionContext::MakeComputerVisionCbirFeaturesRequest(const TStringBuf apphostGraphApiNumber,
                                                                                          NHttpFetcher::IMultiRequest::TRef requests,
                                                                                          TCgiParameters& cgi) const {

    cgi.InsertUnescaped(TStringBuf("cbird"), apphostGraphApiNumber);
    cgi.InsertUnescaped(TStringBuf("type"), TStringBuf("json"));
    cgi.InsertUnescaped(TStringBuf("url"), ImageUrl);

    NHttpFetcher::TRequestPtr req = Context.GetSources().ComputerVisionCbirFeatures().AttachRequest(requests);
    req->AddCgiParams(cgi);
    req->AddHeader("User-Agent", Context.MetaClientInfo().UserAgent);

    TStringBuilder cookies;
    for (const auto& cookie : Context.Meta().Cookies()) {
        cookies << TStringBuf("; ") << cookie.Get();
    }
    req->AddHeader(TStringBuf("Cookie"), cookies);

    return req->Fetch();
}

bool TComputerVisionContext::FetchComputerVisionData(bool needOcrData, bool needOfficeLensData, const TStringBuf additionalFlag) {
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
    NHttpFetcher::THandle::TRef commonRequest = MakeComputerVisionCommonRequest(multiRequest, additionalFlag);
    NHttpFetcher::THandle::TRef ocrRequest;
    if (needOcrData) {
        const TStringBuf apphostGraphApiNumber = "28";
        TCgiParameters cgi;
        ocrRequest = MakeComputerVisionCbirFeaturesRequest(apphostGraphApiNumber, multiRequest, cgi);
    }
    NHttpFetcher::THandle::TRef officeLensRequest;
    if (needOfficeLensData) {
        const TStringBuf apphostGraphApiNumber = "30";
        TCgiParameters cgi;
        if (CropCoordinates.size() == 4) {
            NJson::TJsonValue request;
            NJson::TJsonValue& subParam = request["images_cbirdaemon_request"]["DocumentEdgesDetector"]["QuadrangleParameters"];
            subParam["X0"] = CropCoordinates[0].first;
            subParam["Y0"] = CropCoordinates[0].second;
            subParam["X1"] = CropCoordinates[1].first;
            subParam["Y1"] = CropCoordinates[1].second;
            subParam["X2"] = CropCoordinates[2].first;
            subParam["Y2"] = CropCoordinates[2].second;
            subParam["X3"] = CropCoordinates[3].first;
            subParam["Y3"] = CropCoordinates[3].second;
            cgi.InsertUnescaped(TStringBuf("request"), NJson::WriteJson(request, false));
        }
        officeLensRequest = MakeComputerVisionCbirFeaturesRequest(apphostGraphApiNumber, multiRequest, cgi);
    }

    multiRequest->WaitAll(Deadline);

    NHttpFetcher::TResponse::TRef commonResponse = commonRequest->Wait(Deadline);
    if (!CheckComputerVisionResp(commonResponse, VisionData)) {
        return false;
    }

    if (needOcrData) {
        NHttpFetcher::TResponse::TRef ocrResponse = ocrRequest->Wait(Deadline);
        if (!CheckComputerVisionResp(ocrResponse, OcrData)) {
            return false;
        }
    }

    if (needOfficeLensData) {
        NHttpFetcher::TResponse::TRef officeLensResponse = officeLensRequest->Wait(Deadline);
        if (!CheckComputerVisionResp(officeLensResponse, OcrData)) {
            return false;
        }
    }

    return true;
}

bool TComputerVisionContext::ShouldChangeImage(ECaptureMode captureMode) {
    if (!ImageUrl) {
        return true;
    }

    if (captureMode == ECaptureMode::SIMILAR_PEOPLE
        || captureMode == ECaptureMode::SIMILAR_PEOPLE_FRONTAL
        || captureMode == ECaptureMode::SIMILAR_ARTWORK) {
        return true;
    }

    if (captureMode == ECaptureMode::PHOTO) {
        return false;
    }

    const TSlot* slot = HandlerContext().GetSlot("last_answer", "string");
    if (!slot) {
        return false;
    }

    const NSc::TValue value = slot->Value;
    if (!value.IsString()) {
        return false;
    }

    return FromString<ECaptureMode>(value.GetString()) == captureMode;
}

bool TComputerVisionContext::PrepareComputerVisionData(ECaptureMode captureMode, bool frontalCamera, bool needOcrData,
                                                       bool needOfficeLensData, const TStringBuf additionalFlag) {
    if (ShouldChangeImage(captureMode) && !CheckAndSetImageUrl(captureMode, frontalCamera)) {
        return false;
    }

    if (!FetchComputerVisionData(needOcrData, needOfficeLensData, additionalFlag)) {
        return false;
    }

    GatherComputerVisionSourceMetrics(VisionData);
    LogIntentFactors(VisionData);
    SetCbirId();
    SetCbirIntents();
    return true;
}

bool TComputerVisionContext::CheckComputerVisionResp(NHttpFetcher::TResponse::TRef resp, NSc::TValue& result) {
    if (resp->IsError()) {
        TStringBuilder errText;
        errText << TStringBuf("ComputerVision fetching error: ") << resp->GetErrorText();
        LOG(ERR) << errText << Endl;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_computer_vision_fetching_error");
        ythrow yexception() << errText;
    }

    if (!NSc::TValue::FromJson(result, resp->Data)) {
        LOG(ERR) << TStringBuf("ComputerVision answer error: cannot parse JSON") << Endl;
        Result["code"].SetString("computer_vision_json_parse_error");
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_computer_vision_json_parse_error");
        return false;
    }
    return true;
}

TMaybe<TStringBuf> TComputerVisionContext::GetForceAnswerSlot() const {
    const TSlot* slot = HandlerContext().GetSlot("forcing_answer", "string");
    if (!slot) {
        return Nothing();
    }

    const NSc::TValue value = slot->Value;
    if (!value.IsString()) {
        return Nothing();
    }

    return value.GetString();
}

TMaybe<TStringBuf> TComputerVisionContext::GetSubCaptureModeSlot() const {
    const TSlot* slot = HandlerContext().GetSlot("subcapture_mode", "string");
    if (!slot) {
        return Nothing();
    }

    const NSc::TValue value = slot->Value;
    if (!value.IsString()) {
        return Nothing();
    }

    return value.GetString();
}

void TComputerVisionContext::TryFillLastForceAnswer(ECaptureMode captureMode) {
    HandlerContext().CreateSlot("last_answer", "string", true, NSc::TValue(ToString(captureMode)));
}

} // NBASS
