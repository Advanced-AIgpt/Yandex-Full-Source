//
// HOLLYWOOD FRAMEWORK
// Possible return types for all functions
//

#pragma once

#include "error.h"
#include "run_features.h"
#include "setup.h"
#include "source.h"

#include <google/protobuf/any.pb.h>

#include <exception>

namespace NAlice::NRenderer {

class TDivRenderData;

} // namespace NAlice::NRenderer

namespace NAlice::NHollywoodFw {

class TRender;
class TProtoHwArguments;
class TProtoHwArgumentsRender;

//
// Private classes
// These classes can be created using TScene::TReturnXxx and TScenario::TReturnXxx members
//
namespace NPrivate {

constexpr TStringBuf PROTOBUF_PREFIX_NAME = "type.googleapis.com/";

//
// Scene arguments wrapper
//
class TRetSceneSelector {
public:
    // Default ctor to store user-defined protobuf
    TRetSceneSelector(google::protobuf::Any&& protoScene, const TString& sceneName, const TString& selectedIntent)
        :  ProtoScene_(std::move(protoScene))
        , SceneName_(sceneName)
        , SelectedIntent_(selectedIntent)
        , Defined_(true)
    {
    }
    TRetSceneSelector() = default;
    TRetSceneSelector(const TProtoHwArguments& protoArgs);
    void ToProto(TProtoHwArguments& protoArgs);

    const google::protobuf::Any& GetArguments() const {
        return ProtoScene_;
    }
    // Get a type of object stored in arguments
    const TString GetArgumentsClassName() const {
        TString res = ProtoScene_.type_url();
        // Valid objects must start with 'type.googleapis.com/' prefix
        if (res.StartsWith(PROTOBUF_PREFIX_NAME)) {
            res.erase(0, PROTOBUF_PREFIX_NAME.size());
        }
        return res;
    }
    const TString& GetSceneName() const {
        return SceneName_;
    }
    const TString& GetSelectedIntent() const {
        return SelectedIntent_;
    }
    bool Defined() const {
        return Defined_;
    }

private:
    google::protobuf::Any ProtoScene_; // Protobuf for Scene arguments
    TString SceneName_;
    TString SelectedIntent_;
    bool Defined_ = false;
};

//
// Renderer
//
class TRetRenderSelectorBase {
public:
    TRetRenderSelectorBase() = default;
    TRetRenderSelectorBase(const TRetRenderSelectorBase& rhs) = default;
    TRetRenderSelectorBase(const TProtoHwArgumentsRender& protoArgs);
    virtual ~TRetRenderSelectorBase() = default;
    void ToProto(TProtoHwArgumentsRender& protoArgs);

    const google::protobuf::Any& GetArguments() const {
        return ProtoRender_;
    }
    // Get a type of object stored in arguments
    const TString GetArgumentsClassName() const {
        TString res = ProtoRender_.type_url();
        // Valid objects must start with 'type.googleapis.com/' prefix
        if (res.StartsWith(PROTOBUF_PREFIX_NAME)) {
            res.erase(0, PROTOBUF_PREFIX_NAME.size());
        }
        return res;
    }
    const TString& GetScenePath() const {
        return ScenePath_;
    }
    bool Defined() const {
        return Defined_;
    }
    bool IsIrrelevant() const {
        return IsIrrelevant_;
    }

protected:
    bool IsIrrelevant_ = false;
    // Scene path for renderer. Can be:
    // "" for static/free functions
    // "scenario_name" for scenario members
    // "scenario_name/scene_name" for scene members
    TString ScenePath_;
    google::protobuf::Any ProtoRender_; // Protobuf for Render arguments
    bool Defined_ = false;

protected:
    // Closed implementation, called from TScenario/TScene
    TRetRenderSelectorBase(google::protobuf::Any&& renderArgs, const TString& scenePath, bool isIrrelevant)
        : IsIrrelevant_(isIrrelevant)
        , ScenePath_(scenePath)
        , ProtoRender_(std::move(renderArgs))
        , Defined_(true)
    {
    }
    TRetRenderSelectorBase(const TString& nlgName, const TString& phrase, const google::protobuf::Message& context, bool isIrrelevant);
};

//
// Intermediate render selector for TRetRenderSelector/TRetRenderSelectorFeatures
// Also contains DivRender data
//
class TRetRenderSelectorDiv: public TRetRenderSelectorBase {
public:
    const TVector<std::shared_ptr<NRenderer::TDivRenderData>>& GetDivRender() const {
        return DivRenderData_;
    }

protected:
    TRetRenderSelectorDiv(google::protobuf::Any&& renderArgs, const TString& scenePath)
        : TRetRenderSelectorBase(std::move(renderArgs), scenePath, /*isIrrelevant*/ false)
    {
    }
    TRetRenderSelectorDiv(const TString& nlgName, const TString& phrase, const google::protobuf::Message& context)
        : TRetRenderSelectorBase(nlgName, phrase, context, /*isIrrelevant*/ false)
    {
    }
    ~TRetRenderSelectorDiv() = default;
    void Add(NRenderer::TDivRenderData&& renderData);
private:
    TVector<std::shared_ptr<NRenderer::TDivRenderData>> DivRenderData_;
};

//
// Render selector for continue/apply functions (without RunFeatures)
//
class TRetRenderSelector: public TRetRenderSelectorDiv {
public:
    TRetRenderSelector(google::protobuf::Any&& renderArgs, const TString& scenePath)
        : TRetRenderSelectorDiv(std::move(renderArgs), scenePath)
    {
    }
    TRetRenderSelector(const TString& nlgName, const TString& phrase, const google::protobuf::Message& context)
        : TRetRenderSelectorDiv(nlgName, phrase, context)
    {
    }
    TRetRenderSelector& AddDivRender(NRenderer::TDivRenderData&& renderData) {
        Add(std::move(renderData));
        return *this;
    }
    // Very rarely case - need to return irrelevant answer from scene
    // May happen when scene has very complex logic and irrelevant case can not be handled in dispatcher
    TRetRenderSelector& MakeIrrelevantAnswerFromScene() {
        IsIrrelevant_ = true;
        return *this;
    }
};

//
// Render sslector for run function (with features)
//
class TRetRenderSelectorFeatures: public TRetRenderSelectorDiv {
public:
    TRetRenderSelectorFeatures(google::protobuf::Any&& renderArgs, const TString& scenePath, TRunFeatures&& features)
        : TRetRenderSelectorDiv(std::move(renderArgs), scenePath)
        , Features_(std::move(features))
    {
    }
    TRetRenderSelectorFeatures(const TString& nlgName, const TString& phrase, const google::protobuf::Message& context, TRunFeatures&& features)
        : TRetRenderSelectorDiv(nlgName, phrase, context)
        , Features_(std::move(features))
    {
    }
    const TRunFeatures& GetFeatures() const {
        return Features_;
    }
    TRetRenderSelectorFeatures& AddDivRender(NRenderer::TDivRenderData&& renderData) {
        Add(std::move(renderData));
        return *this;
    }
    // Very rarely case - need to return irrelevant answer from scene
    // May happen when scene has very complex logic and irrelevant case can not be handled in dispatcher
    TRetRenderSelectorFeatures& MakeIrrelevantAnswerFromScene() {
        IsIrrelevant_ = true;
        return *this;
    }
private:
    TRunFeatures Features_;
};

class TRetRenderSelectorIrrelevant: public TRetRenderSelectorBase {
public:
    TRetRenderSelectorIrrelevant(google::protobuf::Any&& renderArgs, const TString& scenePath, TRunFeatures&& features)
        : TRetRenderSelectorBase(std::move(renderArgs), scenePath, true)
        , Features_(std::move(features))
    {
    }
    TRetRenderSelectorIrrelevant(const TString& nlgName, const TString& phrase, const google::protobuf::Message& context, TRunFeatures&& features)
        : TRetRenderSelectorBase(nlgName, phrase, context, true)
        , Features_(std::move(features))
    {
    }
    const TRunFeatures& GetFeatures() const {
        return Features_;
    }
private:
    TRunFeatures Features_;
};

//
// Commit args
//
class TRetCommitSelector {
public:
    TRetCommitSelector(google::protobuf::Any&& commitArgs, TRetRenderSelectorFeatures&& render)
        : RenderSelector_(std::move(render))
        , CommitArgs_(std::move(commitArgs))
    {
    }
    const google::protobuf::Any& GetArguments() const {
        return CommitArgs_;
    }
    const NPrivate::TRetRenderSelectorFeatures& GetRenderSelector() const {
        return RenderSelector_;
    }

    TRetCommitSelector& AddDivRender(NRenderer::TDivRenderData&& renderData) {
        RenderSelector_.AddDivRender(std::move(renderData));
        return *this;
    }

private:
    NPrivate::TRetRenderSelectorFeatures RenderSelector_;
    google::protobuf::Any CommitArgs_; // Protobuf for commit arguments
};

//
// Continue
//
class TReturnValueContinueSelector {
public:
    TReturnValueContinueSelector(google::protobuf::Any&& args, TRunFeatures&& features)
        : ProtoContinue_(std::move(args))
        , Features_(std::move(features))
    {
    }
    const google::protobuf::Any& GetArguments() const {
        return ProtoContinue_;
    }
    const TRunFeatures& GetFeatures() const {
        return Features_;
    }
private:
    google::protobuf::Any ProtoContinue_; // Protobuf for continue arguments
    TRunFeatures Features_;
};

//
// Apply
//
class TReturnValueApplySelector {
public:
    TReturnValueApplySelector(google::protobuf::Any&& args, TRunFeatures&& features)
        : ProtoApply_(std::move(args))
        , Features_(std::move(features))
    {
    }
    const google::protobuf::Any& GetArguments() const {
        return ProtoApply_;
    }
    const TRunFeatures& GetFeatures() const {
        return Features_;
    }
private:
    google::protobuf::Any ProtoApply_; // Protobuf for apply arguments
    TRunFeatures Features_;
};

} // namespace NPrivate

//
// Return from render (default)
//
class TReturnValueSuccess {
};

//
// Additional class to force switch to old scenario flow
//
class TReturnValueDo {
};

//
// Generic types for function returns
//

// Return values from DispatchSetup, Scene::MainSetup
using TRetSetup = std::variant<TSetup,
                               TReturnValueDo,
                               TError>;
// Return values from Dispatch
using TRetScene = std::variant<NPrivate::TRetSceneSelector,
                               TReturnValueDo,
                               NPrivate::TRetRenderSelectorIrrelevant,
                               TError>;
// Return values from Scene::Main
using TRetMain = std::variant<NPrivate::TRetRenderSelector,
                              NPrivate::TRetRenderSelectorFeatures,
                              NPrivate::TReturnValueContinueSelector,
                              NPrivate::TReturnValueApplySelector,
                              NPrivate::TRetCommitSelector,
                              TReturnValueDo,
                              TError>;
// Return values from Scene::ContinueMain,  Scene::ContinueApply
using TRetContinue = std::variant<NPrivate::TRetRenderSelector,
                                  TReturnValueDo,
                                  TError>;
// return from: Render
using TRetResponse = std::variant<TReturnValueSuccess,
                                  TReturnValueDo,
                                  TError>;
// return from: Scene::Commit
using TRetCommit = std::variant<TReturnValueSuccess,
                                TReturnValueDo,
                                TError>;

} // namespace NAlice::NHollywoodFw
