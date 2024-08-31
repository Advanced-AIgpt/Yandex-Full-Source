#include "response.h"

#include <alice/library/network/common.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/json/json_value.h>

namespace NAlice {

// TScenarioErrors -------------------------------------------------------------
void TScenariosErrors::ToHttpResponse(IHttpResponse& response) const {
    response.SetHttpCode(HTTP_INTERNAL_SERVER_ERROR)
        .SetContentType(NContentTypes::APPLICATION_JSON)
        .SetContent(ErrorsToString());
}

TString TScenariosErrors::ErrorsToString() const {
    NJson::TJsonValue result;
    ForEachError([&result](const TString& scenarioName, const TString& stage, const TError& error) {
        TSpeechKitResponseProto::TMeta errorMeta;
        errorMeta.SetType("error");
        errorMeta.SetErrorType(ToString(error.Type));
        errorMeta.SetMessage(ErrorToString(scenarioName, stage, error));
        result.AppendValue(JsonFromProto(errorMeta));
    });
    return ToString(result);
}

// static
TString TScenariosErrors::ErrorToString(const TString& scenarioName, const TString& stage, const TError& error) {
    NJson::TJsonValue result;
    result.InsertValue("scenario_name", scenarioName);
    result.InsertValue("stage", stage);
    result.InsertValue("message", error.ErrorMsg);
    return ToString(result);
}

} // namespace NAlice
