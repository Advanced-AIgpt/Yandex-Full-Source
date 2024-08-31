#include "happy_new_year.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace {

constexpr TStringBuf PREFIX = "https://yastatic.net/s3/milab/2020/data/hny/";
constexpr size_t NUM_IMAGES = 10000;
constexpr size_t MAX_DIGITS = 6;
constexpr TStringBuf NLG_TEMPLATE_NAME = "happy_new_year";

}

namespace NAlice::NHollywood {

void THappyNewYearRunHandle::Do(TScenarioHandleContext& ctx) const {
    NScenarios::TScenarioRunRequest requestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM));
    const TScenarioRunRequestWrapper request(requestProto, ctx.ServiceCtx);
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{ctx.Ctx.Logger(), request};

    const auto& clientInfo = request.ClientInfo();
    if (clientInfo.IsNavigator() || clientInfo.IsYaAuto() || clientInfo.IsElariWatch() ||
        clientInfo.IsSmartSpeaker() || clientInfo.IsTvDevice()) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "render_reject", {}, nlgData);
    } else {
        std::stringstream stream;
        stream << PREFIX << std::setfill('0') << std::setw(MAX_DIGITS) << ctx.Rng.RandomInteger(NUM_IMAGES) << ".jpg";

        nlgData.Context["image_url"] = TString(stream.str());
        nlgData.Context["render_share"] = !clientInfo.IsYaBrowserDesktop();
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE_NAME, "render_text", {}, nlgData);
        bodyBuilder.AddRenderedDivCard(NLG_TEMPLATE_NAME, "render_card", nlgData);
    }

    ctx.ServiceCtx.AddProtobufItem(*std::move(builder).BuildResponse(), RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood
