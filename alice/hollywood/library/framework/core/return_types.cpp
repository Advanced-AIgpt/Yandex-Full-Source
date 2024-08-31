//
// HOLLYWOOD FRAMEWORK
// Internal class : scenario factory
//

#include "return_types.h"

#include "default_renders.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/library/json/json.h>

#include <alice/protos/api/renderer/api.pb.h>

namespace NAlice::NHollywoodFw {

namespace NPrivate {

TRetSceneSelector::TRetSceneSelector(const TProtoHwArguments& proto)
    : SceneName_(proto.GetSceneName())
    , SelectedIntent_(proto.GetSemanticFrameName())
    , Defined_(true)
{
    ProtoScene_.CopyFrom(proto.GetArgs());
}

void TRetSceneSelector::ToProto(TProtoHwArguments& protoArgs) {
    protoArgs.MutableArgs()->CopyFrom(ProtoScene_);
    protoArgs.SetSceneName(SceneName_);
    protoArgs.SetSemanticFrameName(SelectedIntent_);
}

/*
    Render using default NLG renderer with given nlgName, phrase and context protobuf
    @param nlgName - name of scenario NLG file to find a phrase
    @param phrase - phrase to render
    @context - extra parameters passed to NLG renderer
*/
TRetRenderSelectorBase::TRetRenderSelectorBase(const TString& nlgName, const TString& phrase, const google::protobuf::Message& context, bool isIrrelevant)
    : IsIrrelevant_(isIrrelevant)
    , ScenePath_("")
    , Defined_(true)
{
    if (isIrrelevant) {
        TProtoRenderIrrelevantNlg irrelevantNlgProto;
        irrelevantNlgProto.SetNlgName(nlgName);
        irrelevantNlgProto.SetPhrase(phrase);
        irrelevantNlgProto.SetContext(JsonStringFromProto(context));
        ProtoRender_.PackFrom(irrelevantNlgProto);
    } else {
        TProtoRenderDefaultNlg defaultNlgProto;
        defaultNlgProto.SetNlgName(nlgName);
        defaultNlgProto.SetPhrase(phrase);
        defaultNlgProto.SetContext(JsonStringFromProto(context));
        ProtoRender_.PackFrom( defaultNlgProto);
    }
}

TRetRenderSelectorBase::TRetRenderSelectorBase(const TProtoHwArgumentsRender& proto)
    : IsIrrelevant_(proto.GetIrrelevantFlag())
    , ScenePath_(proto.GetScenePath())
    , Defined_(true)
{
    ProtoRender_.CopyFrom(proto.GetArgs());
}

void TRetRenderSelectorBase::ToProto(TProtoHwArgumentsRender& protoArgs) {
    protoArgs.MutableArgs()->CopyFrom(ProtoRender_);
    protoArgs.SetScenePath(ScenePath_);
    protoArgs.SetIrrelevantFlag(IsIrrelevant_);
}

/*
    Ad Div render data to rendering
*/
void TRetRenderSelectorDiv::Add(NRenderer::TDivRenderData&& renderData) {
    DivRenderData_.push_back(std::make_shared<NRenderer::TDivRenderData>(renderData));
}

} // namespace NPrivate

} // namespace NAlice::NHollywoodFw
