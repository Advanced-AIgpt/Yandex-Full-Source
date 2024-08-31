#include <alice/hollywood/library/scenarios/image_what_is_this/answers/clothes.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/market.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/tag.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/answers/similars.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/context.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>


using namespace NAlice::NHollywood::NImage;
using namespace NAlice::NHollywood::NImage::NAnswers;

namespace NAlice::NHollywood::NImage::NComputerVisionFeedbackOptions {

const TStringBuf CLOTHES_UNWANTED = "feedback_negative_images__clothes_unwanted";

}

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_clothes";
constexpr TStringBuf SHORT_ANSWER_NAME = "clothes";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_clothes";

constexpr TStringBuf ALICE_SMART_MODE = "smartcamera";

TString SerializeCropBox(const NSc::TValue& crop) {
    TStringBuilder box;
    box << crop["x0"].GetNumber() << TStringBuf(";");
    box << crop["y0"].GetNumber() << TStringBuf(";");
    box << crop["x1"].GetNumber() << TStringBuf(";");
    box << crop["y1"].GetNumber();
    return box;
}

}

TClothes::TClothes()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    Intent = NImages::NCbir::ECbirIntents::CI_CLOTHES;
    IntentButtonIcon = "https://avatars.mds.yandex.net/get-images-similar-mturk/38061/icon-clothes/orig";
    IsSupportSmartCamera = true;

    AllowedIntents = {
        NImages::NCbir::ECbirIntents::CI_OCR,
        NImages::NCbir::ECbirIntents::CI_SIMILAR,
        NImages::NCbir::ECbirIntents::CI_OCR_VOICE,
    };

    LastForceAlternativeSuggest = {
        NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
        NImages::NCbir::ECbirIntents::CI_INFO,
    };

    AliceSmartMode = ALICE_SMART_MODE;
}

bool TClothes::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool /* force */) const {
    const TMaybe<NSc::TValue>& aliceResponse = ctx.GetImageAliceResponse();
    return aliceResponse.Defined() && aliceResponse.GetRef()["Crops"].ArraySize() > 0;
}

TClothes* TClothes::GetPtr() {
    static TClothes* answer = new TClothes;
    return answer;
}

void TClothes::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetImageAliceResponse().Defined()) {
        ctx.AddImageAliceRequest();
    } else if (ctx.GetDetectedObjectsResponse().empty()) {
        NSc::TValue foundCrops = MakeClothesCrops(ctx);
        if (!foundCrops.IsArray()
                || foundCrops.ArraySize() == 0) {
            return;
        }
        auto& crops = foundCrops.GetArrayMutable();
        SortBy(crops.rbegin(),
               crops.rend(),
               [](const NSc::TValue& item){ return item["crop_area"].GetNumber(); });

        for (const auto& crop : crops) {
            ctx.AddDetectedObjectRequest(crop["crop_orig"].GetString());
        }
    }
}

void TClothes::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    TTag* tagAnswer = TTag::GetPtr();
    if (tagAnswer->CheckForTagAnswer(ctx)) {
        tagAnswer->AddTagAnswer(ctx, "render_clothes_with_tag");
    } else {
        ctx.AddTextCard("render_clothes", {});
    }

    NSc::TValue crops = MakeClothesCrops(ctx);

    if (!AddClothesTabsGallery(ctx, crops)) {
        TSimilars* answerSimilars = TSimilars::GetPtr();
        answerSimilars->AddSimilarsGallery(ctx);
    }

    //ctx.AttachTextCard();
    ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::CLOTHES_UNWANTED);
    //ctx.AddOcrAnswer(EOcrResultCategory::ORC_VAGUE);
}

bool TClothes::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (ctx.GetImageAliceResponse().Defined()) {
        ctx.AddTextCard("render_cannot_apply_clothes_error", {});
        return true;
    }
    return false;
}


NSc::TValue TClothes::MakeClothesCrops(TImageWhatIsThisApplyContext& ctx) const {
    NSc::TValue crops;
    crops.SetArray();

    for (const auto& item : ctx.GetImageAliceResponse().GetRef()["Crops"].GetArray()) {
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

bool TClothes::AddClothesTabsGallery(TImageWhatIsThisApplyContext& ctx, NSc::TValue& foundCrops) const {
    if (!foundCrops.IsArray()
            || foundCrops.ArraySize() == 0) {
        return false;
    }
    auto& crops = foundCrops.GetArrayMutable();
    SortBy(crops.rbegin(),
           crops.rend(),
           [](const NSc::TValue& item){ return item["crop_area"].GetNumber(); });

    const TMarket* marketAnswer = TMarket::GetPtr();
    NSc::TValue result;
    auto& resultTabs = result.GetArrayMutable();
    for (size_t cropIdx = 0; cropIdx < ctx.GetDetectedObjectsResponse().size(); ++cropIdx) {
        NSc::TValue tabItem;
        tabItem["category"] = crops[cropIdx]["category"];
        tabItem["gallery"] = marketAnswer->MakeMarketGallery(ctx,
                                                             ctx.GetDetectedObjectsResponse().at(cropIdx),
                                                             /* ignoreFirst */ false,
                                                             /* appendTailLink */ true,
                                                             crops[cropIdx]["crop_id"]);
        if (tabItem["gallery"]["market_gallery"].ArraySize() > 0) {
            resultTabs.emplace_back(std::move(tabItem));
        }
    }

    if (result.ArraySize() == 0) {
        return false;
    }
    ctx.AddDivCardBlock(TStringBuf("image__clothes_tabs_gallery"), std::move(result));
    return true;
}

TMaybe<ECaptureMode> TClothes::GetOpenCaptureMode() const {
    return ECaptureMode::TInput_TImage_ECaptureMode_Market;
}
