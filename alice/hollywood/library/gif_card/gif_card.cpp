#include "gif_card.h"

namespace NAlice::NHollywood {

namespace {

// Used for when gifs are not supported
constexpr TStringBuf EXP_DEBUG_GIF_TO_TEXT = "hw_debug_gif_to_text";
constexpr TStringBuf EXP_DEBUG_GIF_TO_DUMMY_TEXT = "hw_debug_gif_to_dummy_text";

} // namespace

void RenderGifCard(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TGif& gif, TResponseBodyBuilder& bodyBuilder, TStringBuf nlgTemplateName) {
    TNlgData nlgData{logger, request};
    nlgData.Context["gif_url"] = gif.GetUrl();
    nlgData.Context["gif_source_url"] = gif.GetSourceUrl();
    nlgData.Context["gif_source_text"] = gif.GetSourceText();

    if (request.HasExpFlag(EXP_DEBUG_GIF_TO_DUMMY_TEXT)) {
        bodyBuilder.AddRenderedText(nlgTemplateName, "render_gif_dummy_text", nlgData);
    } else if (request.HasExpFlag(EXP_DEBUG_GIF_TO_TEXT)) {
        bodyBuilder.AddRenderedText(nlgTemplateName, "render_gif_text", nlgData);
    } else {
        bodyBuilder.AddRenderedDiv2Card(nlgTemplateName, "render_gif", nlgData);
    }
}

TGif GifFromJson(const NJson::TJsonValue& json) {
    TGif gif;
    gif.SetUrl(json["gif_url"].GetStringSafe());
    gif.SetSourceUrl(json["gif_source_url"].GetStringSafe());
    gif.SetSourceText(json["gif_source_text"].GetStringSafe());
    return gif;
}

} // namespace NAlice::NHollywood
