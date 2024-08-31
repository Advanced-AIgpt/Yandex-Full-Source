//
// HOLLYWOOD FRAMEWORK
// Internal class : default system renderers
//
#pragma once

#include "render.h"
#include "return_types.h"

#include <alice/hollywood/library/framework/proto/default_render.pb.h>

namespace NAlice::NHollywoodFw::NPrivate {

extern TRetResponse RenderIrrelevant(const TProtoRenderIrrelevantNlg&, TRender&);
extern TRetResponse RenderDefaultNlg(const TProtoRenderDefaultNlg&, TRender&);

} // namespace NAlice::NHollywoodFw::NPrivate
