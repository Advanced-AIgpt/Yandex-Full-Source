#pragma once

#include <alice/hollywood/library/gif_card/proto/gif.pb.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywood {

void RenderGifCard(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TGif& gif, TResponseBodyBuilder& bodyBuilder, TStringBuf nlgTemplateName);

TGif GifFromJson(const NJson::TJsonValue& json);

} // namespace NAlice::NHollywood
