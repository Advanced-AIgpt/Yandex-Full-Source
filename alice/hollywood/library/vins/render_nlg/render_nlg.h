#pragma once

#include <alice/hollywood/library/framework/core/request.h>
#include <alice/vins/api/vins_api/speechkit/protos/vins_response.pb.h>


namespace NAlice::NHollywoodFw {

void RenderNlg(
    const NHollywoodFw::TRequest& request,
    const TStringBuf nlgTemplateName,
    const NProtoVins::TNlgRenderData& vinsNlgRenderData,
    NScenarios::TScenarioResponseBody& responseBody);

} // namespace NAlice::NHollywood
