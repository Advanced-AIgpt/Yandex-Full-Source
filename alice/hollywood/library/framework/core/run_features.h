#pragma once

#include <util/generic/fwd.h>
#include <google/protobuf/message.h>

//
// HOLLYWOOD FRAMEWORK
// Additional TRunRequest features can be used with Main() function
//

namespace NAlice::NScenarios {

class TScenarioRunResponse;
class TScenarioRunResponse_TFeatures;

} // namespace NAlice::NScenarios

namespace NAlice::NHollywoodFw {

class TProtoHwScene;
struct TRunFeaturesImpl;

//
// TRunFeatures class
// Additional helper object to store information about TRunResponse-specific objects to answer from Main() scene fn
// Usage:
// TRetMain MyScene::Main(const MySceneArgs&, const TRunRequest&, TStorage&, const TSource&) const {
//     ...
//     TRunFeatures features;
//     ... add required elements to `features`.
//     return TReturnValueRender(&MyRenderFn, MyRenderArgs, std::move(features));
// }
// TRunFeatures object can be also optionally added to following return objects:
//     return TReturnValueContinue(ContinueArgs, std::move(features));
//     return TReturnValueApply(ApplyArgs, std::move(features));
//     return TReturnValueCommit(&MyRenderFn, MyRenderArgs, CommitArgs, std::move(features));
//
class TRunFeatures {
public:
    TRunFeatures() = default;
    TRunFeatures(const NScenarios::TScenarioRunResponse_TFeatures& proto);

    // Set intent name for post classification
    void SetIntentName(const TString& intent);
    // See TScenarioRunResponse::TFeatures::TPlayerFeatures for more details
    void SetPlayerFeatures(bool restorePlayer, ui32 secondsSincePause);
    // See TScenarioRunResponse::TFeatures::IgnoresExpectedRequest for more details
    void IgnoresExpectedRequest(bool f);
    // Set extra features (TMusicFeatures/TVideoFeatures/TVinsFeatures/etc)
    void Set(const google::protobuf::Message& feature);

    bool Defined() const {
        return Impl_ != nullptr;
    }

    // Internal functions
    void ExportToProto(TProtoHwScene& sceneResults);
    void ExportToResponse(NScenarios::TScenarioRunResponse& response);

private:
    std::shared_ptr<TRunFeaturesImpl> Impl_;
};

} // namespace NAlice::NHollywoodFw
