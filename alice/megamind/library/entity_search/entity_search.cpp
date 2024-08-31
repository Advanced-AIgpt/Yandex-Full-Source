#include "entity_search.h"

#include "entity_finder.h"

#include <alice/megamind/library/experiments/flags.h>

#include <alice/library/json/json.h>

namespace NAlice {

TSourcePrepareStatus CreateEntitySearchRequest(const NJson::TJsonValue& begemotResponse, const TSpeechKitRequest& skr, NHttpFetcher::IRequestBuilder& request) {
    if (skr.HasExpFlag(EXP_ENTITY_RESULT_FOR_VINS_DISABLE)) {
        return ESourcePrepareType::NotNeeded;
    }

    const auto entitySearchText = NEntitySearch::NEntityFinder::GetEntitiesString(begemotResponse);
    if (entitySearchText.empty()) {
        return ESourcePrepareType::NotNeeded;
    }

    request.AddCgiParam(TStringBuf("obj"), entitySearchText);
    request.AddCgiParam(TStringBuf("reqid"), skr.RequestId());
    request.AddCgiParam(TStringBuf("client"), TStringBuf("megamind"));

    return ESourcePrepareType::Succeeded;
}

TErrorOr<TEntitySearchResponse> ParseEntitySearchResponse(const TString& content) {
    try {
        NJson::TJsonValue json = JsonFromString(content);
        return TEntitySearchResponse(std::move(json));
    } catch (...) { // Any parsing error.
        return TError(TError::EType::Critical) << "EntitySearch response parsing error: " << CurrentExceptionMessage();
    }
}

} // namespace NAlice
