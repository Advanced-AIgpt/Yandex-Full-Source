//
// HOLLYWOOD FRAMEWORK
// ScenarioEntiry declarations
//

#pragma once

#include "run_features.h"
#include "scene_base.h"

#include <google/protobuf/empty.pb.h>

#include <exception>

namespace NAlice::NHollywoodFw {

template <class Proto>
class TScene: public TSceneBase {
public:
    virtual TRetSetup MainSetup(const Proto&, const TRunRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("RunSetup()");
    }
    virtual TRetMain Main(const Proto&, const TRunRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Run()");
    }
    virtual TRetSetup ContinueSetup(const Proto&, const TContinueRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("ContinueSetup()");
    }
    virtual TRetContinue Continue(const Proto&, const TContinueRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Continue()");
    }
    virtual TRetSetup CommitSetup(const Proto&, const TCommitRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("CommitSetup()");
    }
    virtual TRetCommit Commit(const Proto&, const TCommitRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Commit()");
    }
    virtual TRetSetup ApplySetup(const Proto&, const TApplyRequest&, const TStorage&) const {
        return ErrorHandlerNotFound("ApplySetup()");
    }
    virtual TRetContinue Apply(const Proto&, const TApplyRequest&, TStorage&, const TSource&) const {
        return ErrorHandlerNotFound("Apply()");
    }

protected:
    TScene(const TScenario* owner, TStringBuf sceneName)
        : TSceneBase(owner, sceneName)
    {
    }

    /*
        Return values from scene functions
    */

    /*
        Return a render as a non-static member of scenario or scene function
        - with features (run stage only)
        - without features (run, continue, apply stages)
    */
    template <class Object, class RenderProto>
    NPrivate::TRetRenderSelector TReturnValueRender(
        TRetResponse (Object::*)(const RenderProto&, TRender& render) const,
        const RenderProto& renderArgs) const
    {
        static_assert(std::is_base_of<TScenario, Object>::value || std::is_base_of<TScene, Object>::value,
                      "The render object should be a member of TScenario or TScene");
        google::protobuf::Any renderArgsPacked;
        renderArgsPacked.PackFrom(renderArgs);

        if constexpr (std::is_base_of<TScenario, Object>::value) {
            return NPrivate::TRetRenderSelector(std::move(renderArgsPacked), BaseOwner_->GetName());
        } else {
            //
            // The next line checks that the called render belongs the same Scene class
            // You CAN NOT call a render function if it belong another scene object
            //
            const Object* checkPtr = static_cast<const Object*>(this);
            Y_UNUSED(checkPtr);
            return NPrivate::TRetRenderSelector(std::move(renderArgsPacked),
                                                Join("/", BaseOwner_->GetName(), GetSceneName()));
        }
        // never return
    }
    template <class Object, class RenderProto>
    NPrivate::TRetRenderSelectorFeatures TReturnValueRender(
        TRetResponse (Object::*)(const RenderProto&, TRender& render) const,
        const RenderProto& renderArgs,
        TRunFeatures&& features) const
    {
        static_assert(std::is_base_of<TScenario, Object>::value || std::is_base_of<TScene, Object>::value,
                      "The render object should be a member of TScenario or TScene");
        google::protobuf::Any renderArgsPacked;
        renderArgsPacked.PackFrom(renderArgs);

        if constexpr (std::is_base_of<TScenario, Object>::value) {
            return NPrivate::TRetRenderSelectorFeatures(std::move(renderArgsPacked), BaseOwner_->GetName(), std::move(features));
        } else {
            //
            // The next line checks that the called render belongs the same Scene class
            // You CAN NOT call a render function if it belong another scene object
            //
            const Object* checkPtr = static_cast<const Object*>(this);
            Y_UNUSED(checkPtr);
            return NPrivate::TRetRenderSelectorFeatures(std::move(renderArgsPacked),
                                                Join("/", BaseOwner_->GetName(), GetSceneName()),
                                                std::move(features));
        }
        // never return
    }

    /*
        Return a render as a static member of scenario/scene/free function
        - without features (run, continue, apply stages)
    */
    template <class RenderProto>
    NPrivate::TRetRenderSelector TReturnValueRender(
        TRetResponse (*)(const RenderProto&, TRender& render),
        const RenderProto& renderArgs) const
    {
        google::protobuf::Any renderArgsPacked;
        renderArgsPacked.PackFrom(renderArgs);
        return NPrivate::TRetRenderSelector(std::move(renderArgsPacked), "");
    }
    template <class RenderProto>
    NPrivate::TRetRenderSelectorFeatures TReturnValueRender(
        TRetResponse (*)(const RenderProto&, TRender& render),
        const RenderProto& renderArgs,
        TRunFeatures&& features) const
    {
        google::protobuf::Any renderArgsPacked;
        renderArgsPacked.PackFrom(renderArgs);
        return NPrivate::TRetRenderSelectorFeatures(std::move(renderArgsPacked), "", std::move(features));
    }
    NPrivate::TRetRenderSelector TReturnValueRender(const TString& nlgName,
                                                    const TString& phrase,
                                                    const google::protobuf::Message& context) const {
        return NPrivate::TRetRenderSelector(nlgName, phrase, context);
    }
    NPrivate::TRetRenderSelectorFeatures TReturnValueRender(const TString& nlgName,
                                                            const TString& phrase,
                                                            const google::protobuf::Message& context,
                                                            TRunFeatures&& features) const {
        return NPrivate::TRetRenderSelectorFeatures(nlgName, phrase, context, std::move(features));
    }

    /*
        Return continue, apply arguments
            - with optional features (run stage only)
    */
    template <class TArguments = google::protobuf::Empty>
    NPrivate::TReturnValueContinueSelector TReturnValueContinue(const TArguments& continueArgs = {},
                                                                TRunFeatures&& features = TRunFeatures()) const {
        google::protobuf::Any args;
        args.PackFrom(continueArgs);
        return NPrivate::TReturnValueContinueSelector(std::move(args), std::move(features));
    }
    template <class TArguments>
    NPrivate::TReturnValueApplySelector TReturnValueApply(const TArguments& applyArgs,
                                                          TRunFeatures&& features = TRunFeatures()) const {
        google::protobuf::Any args;
        args.PackFrom(applyArgs);
        return NPrivate::TReturnValueApplySelector(std::move(args), std::move(features));
    }
    NPrivate::TReturnValueApplySelector TReturnValueApplyUnpacked(google::protobuf::Any applyArgs,
                                                          TRunFeatures&& features = TRunFeatures()) const {
        return NPrivate::TReturnValueApplySelector(std::move(applyArgs), std::move(features));
    }

    /*
        Return commit arguments
            - render as a non-static member of scenario or scene function
            - OR render as a static member of scenario/scene/free function
            - with optional features (run stage only)
    */
    template <class Object, class RenderProto, class ProtoCommit>
    NPrivate::TRetCommitSelector TReturnValueCommit(
        TRetResponse (Object::*fn)(const RenderProto&, TRender& render) const,
        const RenderProto& renderArgs,
        const ProtoCommit& commitArgs,
        TRunFeatures&& features = TRunFeatures()) const
    {
        NPrivate::TRetRenderSelectorFeatures render = TReturnValueRender(fn, renderArgs, std::move(features));
        google::protobuf::Any commitProto;
        commitProto.PackFrom(commitArgs);
        return NPrivate::TRetCommitSelector(std::move(commitProto), std::move(render));
    }
    template <class RenderProto, class ProtoCommit>
    NPrivate::TRetCommitSelector TReturnValueCommit(
        TRetResponse (*fn)(const RenderProto&, TRender& render),
        const RenderProto& renderArgs,
        const ProtoCommit& commitArgs,
        TRunFeatures&& features = TRunFeatures()) const
    {
        NPrivate::TRetRenderSelectorFeatures render = TReturnValueRender(fn, renderArgs, std::move(features));
        google::protobuf::Any commitProto;
        commitProto.PackFrom(commitArgs);
        return NPrivate::TRetCommitSelector(std::move(commitProto), std::move(render));
    }

private:
    TError ErrorHandlerNotFound(TStringBuf node) const {
        TError err(TError::EErrorDefinition::HandlerNotFound);
        err.Details() << "Handler for node '" << node << "' not found.";
        return err;
    }
    // Owner of this scene
    const TScenario* Owner_;
    // Stage of function
    EStageDeclaration Stage_;
};

} // namespace NAlice::NHollywoodFw
