//
// HOLLYWOOD FRAMEWORK
// Internal class : default system renderers
//

#include "default_renders.h"

#include "return_types.h"

#include <alice/hollywood/library/framework/proto/default_render.pb.h>

#include <alice/library/json/json.h>

#include <library/cpp/json/json_reader.h>

namespace NAlice::NHollywoodFw::NPrivate {

/*
    Irrelevant renderer - default framework function
*/
TRetResponse RenderIrrelevant(const TProtoRenderIrrelevantNlg& protoNlg, TRender& render) {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Irrelevant render, phrase: " << protoNlg.GetPhrase();
    if (!protoNlg.GetNlgName().Empty() && !protoNlg.GetPhrase().Empty()) {
        NJson::TJsonValue context;
        NJson::ReadJsonFastTree(protoNlg.GetContext(), &context);
        render.CreateFromNlg(protoNlg.GetNlgName(), protoNlg.GetPhrase(), context);
    }
    return TReturnValueSuccess();
}

/*
    Defaut NLG renderer - default framework function
*/
TRetResponse RenderDefaultNlg(const TProtoRenderDefaultNlg& protoNlg, TRender& render) {
    NJson::TJsonValue context;
    if (!NJson::ReadJsonFastTree(protoNlg.GetContext(), &context)) {
        LOG_ERROR(render.GetRequest().Debug().Logger()) << "Can not extract Json tree from context: '" << protoNlg.GetContext() << '\'';
    }
    render.CreateFromNlg(protoNlg.GetNlgName(), protoNlg.GetPhrase(), context);
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NPrivate
