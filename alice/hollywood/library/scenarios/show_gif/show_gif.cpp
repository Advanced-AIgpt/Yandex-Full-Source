#include "show_gif.h"

#include <alice/hollywood/library/scenarios/show_gif/nlg/register.h>
#include <alice/hollywood/library/scenarios/show_gif/show_gif_resources.h>

#include <alice/hollywood/library/gif_card/gif_card.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/proto/proto.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf FORCE_DISPLAY_CARDS_DIRECTIVE = "force_display_cards";
constexpr TStringBuf LED_SCREEN_DIRECTIVE = "draw_led_screen";
constexpr TStringBuf SHOW_GIF_FRAME_NAME = "alice.show_gif";
constexpr TStringBuf SHOW_GIF_SCENARIO_NAME = "show_gif";

} // namespace

void TShowGifRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder(&nlgWrapper);
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();
    TNlgData nlgData{ctx.Ctx.Logger(), request};

    bool hasError = true; // TODO: split to functions
    if (requestProto.GetBaseRequest().GetInterfaces().GetHasLedDisplay()) {
        const auto framePtr = request.Input().FindSemanticFrame(SHOW_GIF_FRAME_NAME);
        if (framePtr) {
            i32 number = 0; // TODO: validation
            for (const auto& slot : framePtr->GetSlots()) {
                if (slot.GetName() == "number") {
                    TryFromString(slot.GetValue(), number);
                }
            }
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(SHOW_GIF_SCENARIO_NAME, "render_result", /* buttons = */ {}, nlgData);
            bodyBuilder.AddClientActionDirective(TString{FORCE_DISPLAY_CARDS_DIRECTIVE}, {});

            NJson::TJsonValue image;
            image["frontal_led_image"] = TString::Join("https://static-alice.s3.yandex.net/led/", ToString(number), ".gif");
            image["endless"] = true;
            NJson::TJsonValue data;
            data["payload"].AppendValue(std::move(image)); // TODO: rename to animation_sequence
            bodyBuilder.AddClientActionDirective(TString{LED_SCREEN_DIRECTIVE}, data); // TODO: move to library
            hasError = false;
        }
    } else if (requestProto.GetBaseRequest().GetInterfaces().GetCanShowGif()) {
        const auto& gifs = ctx.Ctx.ScenarioResources<TShowGifResources>().Gifs();
        if (!gifs.empty()) {
            bodyBuilder.AddRenderedTextWithButtonsAndVoice(SHOW_GIF_SCENARIO_NAME, "render_result", /* buttons = */ {}, nlgData);
            const auto& gif = gifs[ctx.Rng.RandomInteger() % gifs.size()];
            RenderGifCard(ctx.Ctx.Logger(), request, gif, bodyBuilder, SHOW_GIF_SCENARIO_NAME);
            bodyBuilder.SetShouldListen(true); // for more gifs
            hasError = false;
        }
    }
    if (hasError) {
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(SHOW_GIF_SCENARIO_NAME, "render_gifs_not_supported_error", /* buttons = */ {}, nlgData);
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("show_gif",
                  AddHandle<TShowGifRunHandle>()
                  .SetResources<TShowGifResources>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NShowGif::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
