#include <alice/hollywood/library/scenarios/image_what_is_this/answers/office_lens.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/utils.h>

using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_office_lens";
constexpr TStringBuf SHORT_ANSWER_NAME = "office_lens";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_office_lens";

constexpr TStringBuf CBIRD = "30";

constexpr TStringBuf ALICE_MODE = "document";
constexpr TStringBuf ALICE_SMART_MODE = "doc_scanner";

}

namespace NAlice::NHollywood::NImage::NFlags {

const TString FIX_OFFICE_LENS_SCAN_TIME = "image_recognizer_fix_office_lens_scan_time";
const TString CV_EXP_OFFICE_LENS_CROP = "image_recognizer_office_lens_crop";
const TString CV_EXP_OFFICE_LENS_BUTTONS = "image_recognizer_enable_office_lens_buttons";
const TString OFFICE_LENS_SAVE_DISK = "image_recognizer_enable_office_lens_save_disk";

}

TOfficeLens::TOfficeLens()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    IntentButtonIcon = "";
    CaptureMode = ECaptureMode::TInput_TImage_ECaptureMode_Document;
    AllowedIntents = {
        NImages::NCbir::ECbirIntents::CI_SIMILAR,
        NImages::NCbir::ECbirIntents::CI_CLOTHES,
        NImages::NCbir::ECbirIntents::CI_MARKET,
        NImages::NCbir::ECbirIntents::CI_OCR,
        NImages::NCbir::ECbirIntents::CI_OCR_VOICE
    };

    LastForceAlternativeSuggest = {
        NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE,
        NImages::NCbir::ECbirIntents::CI_INFO
    };

    IsSupportSmartCamera = true;

    AliceMode = ALICE_MODE;
    AliceSmartMode = ALICE_SMART_MODE;
}

void TOfficeLens::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetImageAliceResponse().Defined()) {
        ctx.AddImageAliceRequest();
    }
    if (!ctx.GetCbirFeaturesResponse().Defined()) {
        TCgiParameters cgi;
        const TCropCoordinates& cropCoordinates = ctx.GetCropCoordinates();
        if (cropCoordinates.size() == 4) {
            NJson::TJsonValue request;
            NJson::TJsonValue& subParam = request["images_cbirdaemon_request"]["DocumentEdgesDetector"]["QuadrangleParameters"];
            subParam["X0"] = cropCoordinates[0].first;
            subParam["Y0"] = cropCoordinates[0].second;
            subParam["X1"] = cropCoordinates[1].first;
            subParam["Y1"] = cropCoordinates[1].second;
            subParam["X2"] = cropCoordinates[2].first;
            subParam["Y2"] = cropCoordinates[2].second;
            subParam["X3"] = cropCoordinates[3].first;
            subParam["Y3"] = cropCoordinates[3].second;
            cgi.InsertUnescaped(TStringBuf("request"), NJson::WriteJson(request, false));
        }
        ctx.AddCbirFeaturesRequest(CBIRD, cgi.Print());
    }
}

void TOfficeLens::CleanUp(TImageWhatIsThisApplyContext&) const {
}

TOfficeLens* TOfficeLens::GetPtr() {
    static TOfficeLens* answer = new TOfficeLens;
    return answer;
}

bool TOfficeLens::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const {
    return force && ctx.GetCbirFeaturesResponse().Defined()
            && !ctx.GetCbirFeaturesResponse().GetRef()["cbirdaemon"]["enhancedDocumentImage"][0]["sizes"].IsNull();
}
void TOfficeLens::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    const NSc::TValue& ocrData = ctx.GetCbirFeaturesResponse().GetRef();
    if (ocrData["cbirdaemon"]["enhancedDocumentImage"].Front()["sizes"].IsNull()) {
        return;
    }

    const auto& officeLensInfo = ocrData["cbirdaemon"]["enhancedDocumentImage"].Front()["sizes"];
    if (!officeLensInfo.Has("orig") || !officeLensInfo.Has("preview")) {
        return;
    }

    NSc::TValue card;
    ctx.AddTextCard("render_office_lens_result", {});
    const auto& documentEdges = ocrData["cbirdaemon"]["documentEdges"].Front();
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
            // TODO:
            //Output("crop_coordinates") = coordinates;
        }
    }

    TString currentTime;
    if (!ctx.HasFlag(NFlags::FIX_OFFICE_LENS_SCAN_TIME)) {
        currentTime = TInstant::Now().FormatLocalTime("%d %m %y %H:%M");
    } else {
        currentTime = "fixed_time";
    }
    const TString filename = "scan" + currentTime + ".jpg";
    TString downloadImage = TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString();
    card["preview_image"] = TString("https://avatars.mds.yandex.net") + officeLensInfo["preview"]["path"].GetString();
    card["full_image"] = TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString();

    // TODO:
    //HandlerContext().CreateSlot("download_image", "string", true, NSc::TValue(downloadImage));
    ctx.AddDivCardBlock(TStringBuf("render_office_lens"), std::move(card));

    NSc::TValue buttonsCard;
    const TClientInfo& clientInfo = ctx.GetRequest().ClientInfo();
    const bool isEnoughVersionOfAndroid = clientInfo.IsAndroidAppOfVersionOrNewer(9, 80);
    const bool isPp = clientInfo.IsSearchApp();
    if (isPp && isEnoughVersionOfAndroid && !coordinates.empty()) {
        NSc::TValue cropButton;
        cropButton["id"] = "image_what_is_this_office_lens_crop";
        cropButton["camera_open"] = true;
        auto& context = cropButton["context"].SetDict();
        context["cbir_id"].SetString(ctx.GetCbirId().Defined() ? ctx.GetCbirId().GetRef() : "");
        context["query_url"].SetString(ctx.GetImageUrl());
        context["crop_coordinates"] = coordinates;
        context["image_search_mode_name"] = CaptureModeToString(ECaptureMode::TInput_TImage_ECaptureMode_Document);
        buttonsCard.Push(cropButton);
    }

    NSc::TValue downloadButton;
    downloadButton["id"] = "image_what_is_this_office_lens_download";
    downloadButton["external_url"] = TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString() + "?download=scan.jpg";
    buttonsCard.Push(downloadButton);

    if (ctx.HasFlag(NFlags::OFFICE_LENS_SAVE_DISK) && clientInfo.IsSearchApp()) {
        NSc::TValue diskButton;
        diskButton["id"] = "alice.image_what_is_this_office_lens_disk";
        auto& context = diskButton["context"].SetDict();
        context["cbir_id"].SetString(ctx.GetCbirId().Defined() ? ctx.GetCbirId().GetRef() : "");
        context["query_url"].SetString(ctx.GetImageUrl());

        buttonsCard.Push(diskButton);
        const TString& actionId = ctx.AddSwitchIntentAction(ToString(diskButton["id"].GetString()));
        ctx.AddActionSuggest("alice.image_what_is_this_office_lens_disk", actionId);
        ctx.GetState().SetOfficeLensScan(TString("https://avatars.mds.yandex.net") + officeLensInfo["orig"]["path"].GetString());
    }

    NSc::TValue buttonsDivCard;
    buttonsDivCard["buttons"] = buttonsCard;
    buttonsDivCard["disable_title"] = true;

    ctx.AddDivCardBlock(TStringBuf("image__alternative_intents"), std::move(buttonsDivCard));
}

bool TOfficeLens::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (ctx.GetCbirFeaturesResponse().Defined()) {
        ctx.AddTextCard("render_cannot_apply_office_lens_error", {});
        return true;
    }
    return false;
}

