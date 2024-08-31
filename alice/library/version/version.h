#pragma once

#include <alice/library/json/json.h>
#include <alice/library/network/common.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/svnversion/svnversion.h>

namespace NAlice {

inline const TString VERSION_STRING = TString::Join(GetBranch(), '@', GetArcadiaLastChange());

NJson::TJsonValue CreateVersionData();

template<typename TResponse>
TResponse& FillVersionData(TResponse& response) {
    return response.SetHttpCode(HttpCodes::HTTP_OK)
        .SetContentType(NContentTypes::TEXT_PLAIN)
        .SetContent(GetProgramSvnVersion());
}

template<typename TResponse>
TResponse& FillJsonVersionData(TResponse& response) {
    return response.SetHttpCode(HttpCodes::HTTP_OK)
        .SetContentType(NContentTypes::APPLICATION_JSON)
        .SetContent(JsonToString(CreateVersionData()) + "\n");
}

} // namespace
