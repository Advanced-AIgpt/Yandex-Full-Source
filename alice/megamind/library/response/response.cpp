#include "response.h"

#include <alice/library/json/json.h>

namespace NAlice {

namespace {

TScenarioResponseBuilderProto ConstructProto(TScenarioResponseCommonProto&& commonProto,
                                             TResponseBuilderProto&& builderProto)
{
    TScenarioResponseBuilderProto proto;

    proto.MutableProto()->Swap(&builderProto);
    proto.MutableCommon()->Swap(&commonProto);

    return proto;
}

template <typename T>
T ConstructProto(TScenarioResponseCommonProto&& commonProto) {
    T proto;
    *proto.MutableCommon() = std::move(commonProto);
    return proto;
}

} // namespace

// TScenarioResponse::TResponseBuilderCase -------------------------------------
TScenarioResponse::TResponseBuilderCase::TResponseBuilderCase(const TSpeechKitRequest& skr,
                                                              const TRequest& requestModel,
                                                              const NMegamind::IGuidGenerator& guidGenerator,
                                                              TString&& scenarioName,
                                                              TScenarioResponseCommonProto&& commonProto,
                                                              const TMaybe<TString>& serializerScenarioName)
    : Proto{ConstructProto<TScenarioResponseBuilderProto>(std::move(commonProto))}
    , Builder(std::make_unique<TResponseBuilder>(skr, requestModel, std::move(scenarioName),
                                                 *Proto.MutableProto(), guidGenerator, serializerScenarioName))
{
}

TScenarioResponse::TResponseBuilderCase::TResponseBuilderCase(const TSpeechKitRequest& skr,
                                                              const TRequest& requestModel,
                                                              const NMegamind::IGuidGenerator& guidGenerator,
                                                              TResponseBuilderProto&& builderProto,
                                                              TScenarioResponseCommonProto&& commonProto)
    : Proto{ConstructProto(std::move(commonProto), std::move(builderProto))}
    , Builder(std::make_unique<TResponseBuilder>(TResponseBuilder::FromProto(skr, requestModel,
                                                                             *Proto.MutableProto(), guidGenerator)))
{
}

TScenarioResponse::TResponseBuilderCase::TResponseBuilderCase(TScenarioResponseCommonProto&& commonProto)
    : Proto{ConstructProto<TScenarioResponseBuilderProto>(std::move(commonProto))}
{
}

// TScenarioResponse -----------------------------------------------------------
TScenarioResponse::TScenarioResponse(const TString& scenarioName,
                                     const TVector<TSemanticFrame>& scenarioSemanticFrames,
                                     bool scenarioAcceptsAnyUtterance)
    : RequestSemanticFrames{scenarioSemanticFrames}
    , ScenarioType{TScenarioResponse::EScenarioType::Other}
    , RequestIsExpected{false}
    , ScenarioName{scenarioName}
{
    CommonProto().SetScenarioAcceptsAnyUtterance(scenarioAcceptsAnyUtterance);
}

const TString& TScenarioResponse::GetScenarioName() const {
    return Response ? Response->Name() : ScenarioName;
}

TResponseBuilder* TScenarioResponse::BuilderIfExists() {
    return Response ? &*Response->Builder : nullptr;
}

const TResponseBuilder* TScenarioResponse::BuilderIfExists() const {
    return Response ? &*Response->Builder : nullptr;
}

TSemanticFrame TScenarioResponse::GetResponseSemanticFrame() const {
    if (ResponseBody.Defined()) {
        return ResponseBody->GetSemanticFrame();
    }
    return Response ? Response->GetResponseSemanticFrame() : TSemanticFrame{};
}

TResponseBuilder& TScenarioResponse::ForceBuilder(const TSpeechKitRequest& skr,
                                                  const TRequest& requestModel,
                                                  const NMegamind::IGuidGenerator& guidGenerator,
                                                  const TMaybe<TString>& serializerScenarioName) {
    if (auto* builder = BuilderIfExists()) {
        return *builder;
    }
    Response = std::make_unique<TResponseBuilderCase>(skr, requestModel, guidGenerator,
                                                      std::move(ScenarioName), std::move(CommonProto()), serializerScenarioName);
    return *Response->Builder;
}

TResponseBuilder& TScenarioResponse::ForceBuilder(const TSpeechKitRequest& skr,
                                                  const TRequest& requestModel,
                                                  const NMegamind::IGuidGenerator& guidGenerator,
                                                  TResponseBuilderProto&& initializedProto) {
    if (auto* builder = BuilderIfExists()) {
        return *builder;
    }

    Response = std::make_unique<TResponseBuilderCase>(skr, requestModel, guidGenerator,
                                                      std::move(initializedProto), std::move(CommonProto()));
    return *Response->Builder;
}

TResponseBuilder* TScenarioResponse::ForceBuilderFromSession(const TSpeechKitRequest& skr,
                                                             const TRequest& requestModel,
                                                             const NMegamind::IGuidGenerator& guidGenerator,
                                                             const ISession& session)
{
    if (auto* builder = BuilderIfExists()) {
        return builder;
    }

    if (auto proto = session.GetScenarioResponseBuilder(); proto.Defined()) {
        return &ForceBuilder(skr, requestModel, guidGenerator, std::move(*proto));
    }

    return nullptr;
}

void TScenarioResponse::SetHttpCode(HttpCodes httpCode, TMaybe<TString> reason) {
    CommonProto().SetHttpCode(httpCode);
    if (reason.Defined()) {
        CommonProto().SetHttpErrorReason(*reason);
    }
}

TMaybe<HttpCodes> TScenarioResponse::GetHttpCode() const {
    ui32 httpCode = CommonProto().GetHttpCode();
    if (IsHttpCode(httpCode)) {
        return static_cast<HttpCodes>(httpCode);
    }
    return Nothing();
}

TMaybe<TString> TScenarioResponse::GetHttpErrorReason() const {
    if (const auto& reason = CommonProto().GetHttpErrorReason(); !reason.empty()) {
        return reason;
    }
    return Nothing();
}

void TScenarioResponse::SetScenarioType(TScenarioResponse::EScenarioType scenarioType) {
    ScenarioType = scenarioType;
}
TScenarioResponse::EScenarioType TScenarioResponse::GetScenarioType() const {
    return ScenarioType;
}

const TScenarioResponseCommonProto& TScenarioResponse::CommonProto() const {
    return Response ? Response->CommonProto() : CommonProtoInit;
}

TScenarioResponseCommonProto& TScenarioResponse::CommonProto() {
    return Response ? Response->CommonProto() : CommonProtoInit;
}

NScenarios::TScenarioResponseBody* TScenarioResponse::ResponseBodyIfExists() {
    return ResponseBody.Defined() ? ResponseBody.Get() : nullptr;
}

const NScenarios::TScenarioResponseBody* TScenarioResponse::ResponseBodyIfExists() const {
    return ResponseBody.Defined() ? ResponseBody.Get() : nullptr;
}

void TScenarioResponse::SetResponseBody(const TResponseBody& responseBody) {
    ResponseBody = responseBody;
}

NScenarios::TScenarioContinueResponse* TScenarioResponse::ContinueResponseIfExists() {
    return ContinueResponse.Defined() ? ContinueResponse.Get() : nullptr;
}

const NScenarios::TScenarioContinueResponse* TScenarioResponse::ContinueResponseIfExists() const {
    return ContinueResponse.Defined() ? ContinueResponse.Get() : nullptr;
}

void TScenarioResponse::SetContinueResponse(const NScenarios::TScenarioContinueResponse& continueResponse){
    ContinueResponse = continueResponse;
}

} // namespace NAlice
