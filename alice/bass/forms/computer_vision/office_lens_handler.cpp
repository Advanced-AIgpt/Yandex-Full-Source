#include "office_lens_handler.h"

#include <alice/bass/libs/metrics/metrics.h>

#include <util/datetime/base.h>


using namespace NBASS;
using namespace NImages::NCbir;

TCVAnswerOfficeLens::TCVAnswerOfficeLens()
        : IComputerVisionAnswer(
        TComputerVisionOfficeLensHandler::FormShortName(),
        TStringBuf("https://avatars.mds.yandex.net/get-images-similar-mturk/39305/icon-office_lens/orig"))
{
}

bool TCVAnswerOfficeLens::TryApplyTo(TComputerVisionContext& ctx, bool force, bool shouldAttachAlternativeIntents) const {
    if (IsSuitable(ctx, force) && !DisabledByFlag(ctx)) {
        Compose(ctx);
        AttachAlternativeIntentsSuggest(ctx);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::USELESS);
        ctx.AppendFeedbackOption(NComputerVisionFeedbackOptions::OTHER);
        return true;
    }

    if (ctx.GetOcrData()["cbirdaemon"]["enhancedDocumentImage"][0]["sizes"].IsNull()) {
        if (force && shouldAttachAlternativeIntents) {
            AttachCannotApplyMessage(ctx);
        }
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_empty_office_lens");
        return false;
    }

    return false;
}

bool TCVAnswerOfficeLens::IsSuitable(const TComputerVisionContext& ctx, bool force) const {
    return force && !ctx.GetOcrData()["cbirdaemon"]["enhancedDocumentImage"][0]["sizes"].IsNull();
}

void TCVAnswerOfficeLens::Compose(TComputerVisionContext& ctx) const {
    ctx.AddOfficeLensAnswer();
    ctx.AttachTextCard();

    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_office_lens");
}

bool TCVAnswerOfficeLens::IsSuggestible(const TComputerVisionContext& ) const {
    return true;
}

TStringBuf TCVAnswerOfficeLens::GetCannotApplyMessage() const {
    return TStringBuf("cannot_apply_office_lens");
}

void TCVAnswerOfficeLens::AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const {
    ctx.AttachAlternativeIntentsSuggest(
            {ECbirIntents::CI_SIMILAR, ECbirIntents::CI_CLOTHES, ECbirIntents::CI_MARKET, ECbirIntents::CI_OCR, ECbirIntents::CI_OCR_VOICE},
            {NImages::NCbir::ECbirIntents::CI_SIMILAR_LIKE, NImages::NCbir::ECbirIntents::CI_INFO});
}

// TComputerVisionOfficeLensHandler ------------------------------------------------
TComputerVisionOfficeLensHandler::TComputerVisionOfficeLensHandler()
        : TComputerVisionMainHandler(ECaptureMode::DOCUMENT, false)
{}

void TComputerVisionOfficeLensHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionOfficeLensHandler>();
    };
    handlers->emplace(TComputerVisionOfficeLensHandler::FormName(), handler);
}

TMaybe<TString> TComputerVisionOfficeLensHandler::GetForcingString() const {
    return MakeMaybe<TString>(TCVAnswerOfficeLens::ForceName());
}

void TComputerVisionOfficeLensDiskHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TComputerVisionOfficeLensDiskHandler>();
    };
    handlers->emplace(TComputerVisionOfficeLensDiskHandler::FormName(), handler);
}

TResultValue TComputerVisionOfficeLensDiskHandler::WrappedDo(TComputerVisionContext& cvContext) const {
    if (!cvContext.HandlerContext().ClientFeatures().IsSearchApp()) {
        cvContext.Output("office_lens_disk_need_search_app") = true;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_need_search_app");
        return TResultValue();
    }
    cvContext.ExtractPersonalData();
    if (!cvContext.ExtractUserTicket()) {
        cvContext.Switch(NComputerVisionForms::IMAGE_WHAT_IS_THIS);
        cvContext.AttachTextCard();
        cvContext.AttachCustomUriSuggest(TStringBuf("image_what_is_this__auth_office_lens"), "yandex-auth://?theme=light");
        cvContext.AttachUpdateFormSuggest(TStringBuf("image_what_is_this__office_lens_disk"), TComputerVisionOfficeLensDiskHandler::FormName());
        cvContext.Output("auth_office_lens") = true;
        LOG(DEBUG) << "Office lens need authorization" << Endl;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_need_authorization");
        return TResultValue();
    }

    const NJson::TJsonValue rootDir = cvContext.RequestDiskInfo("disk:/");
    if (!rootDir.IsDefined()) {
        cvContext.Output("code").SetString("office_lens_disk_error");
        LOG(DEBUG) << "Office lens error. Can not get root dir" << Endl;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_no_root_dir");
        return TResultValue();
    }

    bool hasDocumentsDir = false;
    for (const auto& item : rootDir["_embedded"]["items"].GetArray()) {
        if (item["name"].GetString() == "Документы") {
            hasDocumentsDir = true;
            break;
        }
    }

    if (!hasDocumentsDir) {
        if (!cvContext.RequestDiskCreateDir("disk:/Документы")) {
            cvContext.Output("code").SetString("office_lens_disk_error");
            LOG(DEBUG) << "Office lens error. Can not create dir" << Endl;
            Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_can_not_create_dir");
            return TResultValue();
        }
    }

    const TString currentTime = cvContext.HandlerContext().Now().FormatLocalTime("%d %m %y %H:%M:%S");
    const TString filename = "Скан " + currentTime;
    const TString dir = "Документы";
    const TString pathToFile = dir + '/' + filename + ".jpg";
    const TString fileUrl = "disk:/" + pathToFile;

    const TContext::TSlot* imageUrlSlot = cvContext.HandlerContext().GetSlot("download_image", "string");
    if (!imageUrlSlot) {
        cvContext.Output("code").SetString("office_lens_disk_error");
        LOG(DEBUG) << "Office lens error. Can not get download image url" << Endl;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_no_download_image");
        return TResultValue();
    }
    const TStringBuf imageUrl = imageUrlSlot->Value.GetString();
    if (imageUrl.empty()) {
        cvContext.Output("code").SetString("office_lens_disk_error");
        LOG(DEBUG) << "Office lens error. Can not get download image url from slot" << Endl;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_can_not_download_image");
        return TResultValue();
    }
    if (!cvContext.RequestDiskPutFile(fileUrl, imageUrl)) {
        cvContext.Output("code").SetString("office_lens_disk_error");
        LOG(DEBUG) << "Office lens error. Can not upload image" << Endl;
        Y_STATS_INC_COUNTER("bass_computer_vision_result_error_office_lens_disk_can_not_put_file");
        return TResultValue();
    }

    TCgiParameters cgi;
    cgi.InsertUnescaped("dialog", "slider");
    cgi.InsertEscaped("idDialog", "/disk/" + pathToFile);
    const TString webFileUrl = "https://disk.yandex.ru/client/disk/" + dir + "?" + cgi.Print();

    cvContext.Output("office_lens_disk") = true;
    cvContext.Output("filename") = filename;
    cvContext.AttachTextCard();
    cvContext.AttachCustomUriSuggest(TStringBuf("image_what_is_this__open_disk_uri"), webFileUrl);

    Y_STATS_INC_COUNTER("bass_computer_vision_result_answer_office_lens_disk");

    return TResultValue();
}
