#pragma once

#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/div/div2card.pb.h>

namespace NAlice::NHollywood::NResponseMerger {

TDiv2Card Div2CardFromRenderResponse(const NRenderer::TRenderResponse* renderResponse, TRTLogger& logger);

} // namespace NAlice::NHollywood::NResponserMerger
