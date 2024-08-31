#include "helpers.h"


namespace NAlice {

namespace {

TString CreateAppHostItemName(const TStringBuf scenarioName, const TStringBuf tag, const TStringBuf itemType) {
    return TStringBuilder{} << "scenario_" << scenarioName << '_' << tag << '_' << itemType;
}

} // namespace

TAppHostItemNames::TAppHostItemNames(const TString& scenarioName, const TStringBuf itemRequestSuffix, const TStringBuf itemResponseSuffix)
    : ApplyRequest{CreateAppHostItemName(scenarioName, TAG_APPLY, itemRequestSuffix)}
    , ApplyResponse{CreateAppHostItemName(scenarioName, TAG_APPLY, itemResponseSuffix)}
    , CommitRequest{CreateAppHostItemName(scenarioName, TAG_COMMIT, itemRequestSuffix)}
    , CommitResponse{CreateAppHostItemName(scenarioName, TAG_COMMIT, itemResponseSuffix)}
    , ContinueRequest{CreateAppHostItemName(scenarioName, TAG_CONTINUE, itemRequestSuffix)}
    , ContinueResponse{CreateAppHostItemName(scenarioName, TAG_CONTINUE, itemResponseSuffix)}
    , RunRequest{CreateAppHostItemName(scenarioName, TAG_RUN, itemRequestSuffix)}
    , RunResponse{CreateAppHostItemName(scenarioName, TAG_RUN, itemResponseSuffix)}
    , RequestMeta{TStringBuilder{} << "scenario_" << scenarioName << "_request_meta"}
    , BaseRequest{TStringBuilder{} << "scenario_" << scenarioName << BASE_REQUEST_SUFFIX}
    , RequestInput{TStringBuilder{} << "scenario_" << scenarioName << REQUEST_INPUT_SUFFIX}
{
}

} // namespace NAlice
