#include <alice/hollywood/library/scenarios/image_what_is_this/answers/office_lens_disk.h>
#include <alice/hollywood/library/scenarios/image_what_is_this/proto/render_request.pb.h>

using namespace NAlice::NHollywood::NImage::NAnswers;

namespace {

constexpr TStringBuf ANSWER_NAME = "alice.image_what_is_this_office_lens_disk";
constexpr TStringBuf SHORT_ANSWER_NAME = "office_lens_disk";
constexpr TStringBuf DISABLE_FLAG = "alice.disable_image_what_is_this_office_lens_disk";
constexpr TStringBuf DIR_PATH = "disk:/Документы";

}

TOfficeLensDisk::TOfficeLensDisk()
    : IAnswer(ANSWER_NAME, SHORT_ANSWER_NAME, DISABLE_FLAG)
{
    IsRepeatable = true;
}

bool TOfficeLensDisk::IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool /* force */) const {
    return !ctx.GetState().GetOfficeLensScan().Empty(); //true;
}

TOfficeLensDisk* TOfficeLensDisk::GetPtr() {
    static TOfficeLensDisk* answer = new TOfficeLensDisk;
    return answer;
}

void TOfficeLensDisk::MakeRequests(TImageWhatIsThisApplyContext& ctx) const {
    if (!CanSave(ctx)) {
        ctx.AddRenderRequest(TRenderRequestProto());
        return;
    }
    if (!ctx.GetDiskCreateDirResponseStatusCode().Defined()) {
        ctx.AddDiskCreateDirRequest(DIR_PATH);
    } else {
        int statusCode = ctx.GetDiskCreateDirResponseStatusCode().GetRef();
        TString filename;
        if (statusCode == 200 || statusCode == 409) {
            const TString currentTime = TInstant::Now().FormatLocalTime("%d %m %y %H:%M:%S");
            filename = "Скан " + currentTime + ".jpg";
            const TString fileUrl = TString(DIR_PATH) + "/" + filename;
            ctx.AddDiskSaveFileRequest(fileUrl, ctx.GetState().GetOfficeLensScan());
        }
        TRenderRequestProto renderRequest;
        if (!filename.empty()) {
            renderRequest.SetOfficeLensFilename(filename);
        }
        ctx.AddRenderRequest(renderRequest);
    }
}

bool TOfficeLensDisk::RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const {
    ctx.AddTextCard("render_cannot_apply_office_lens_disk_error", {});
    return true;
}


void TOfficeLensDisk::CleanUp(TImageWhatIsThisApplyContext&) const {
}

bool TOfficeLensDisk::CanSave(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetRequest().ClientInfo().IsSearchApp()) {
        return false;
    }
    TString userTicket = ctx.GetContext().RequestMeta.GetUserTicket();
    if (userTicket.empty()) {
        return false;
    }
    return true;
}

void TOfficeLensDisk::ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const {
    if (!ctx.GetRequest().ClientInfo().IsSearchApp()) {
        ctx.StatIncCounter("hollywood_computer_vision_result_error_office_lens_disk_need_search_app");
        ctx.GetAnalyticsInfoBuilder().AddObject("office_lens_disk_need_search_app", "office_lens_disk_need_search_app", "1");
        ctx.AddTextCard("render_common_error", {});
        return;
    }

    if (ctx.GetContext().RequestMeta.GetUserTicket().Empty()) {
        ctx.AddOpenUriButton("auth_office_lens_disk", "yandex-auth://?theme=light");
        ctx.AddTextCard("render_auth_office_lens_disk", {});
        ctx.StatIncCounter("hollywood_computer_vision_result_error_office_lens_disk_need_authorization");
        return;
    }

    int statusCode = ctx.GetDiskCreateDirResponseStatusCode().GetRef();
    if (statusCode != 200 && statusCode != 409) {
        ctx.StatIncCounter("hollywood_computer_vision_result_error_office_lens_disk_create_dir");
        ctx.GetAnalyticsInfoBuilder().AddObject("office_lens_disk_create_dir", "office_lens_disk_create_dir", "1");
        ctx.AddTextCard("render_common_error", {});
        return;
    }

    const TString dir = "Документы";
    const TString currentTime = TInstant::Now().FormatLocalTime("%d %m %y %H:%M:%S");
    const TString filename = "Скан " + currentTime + ".jpg";
    const TString pathToFile = dir + "/" + filename;

    TCgiParameters cgi;
    cgi.InsertUnescaped("dialog", "slider");
    cgi.InsertEscaped("idDialog", "/disk/" + pathToFile);
    const TString webFileUrl = "https://disk.yandex.ru/client/disk/" + dir + "?" + cgi.Print();

    NSc::TValue cardData;
    cardData["filename"] = filename;
    ctx.AddOpenUriButton("office_lens_disk_open_file", webFileUrl);
    ctx.AddTextCard("render_office_lens_disk_saved", cardData);

    ctx.StatIncCounter("hollywood_computer_vision_result_answer_office_lens_disk");
}
