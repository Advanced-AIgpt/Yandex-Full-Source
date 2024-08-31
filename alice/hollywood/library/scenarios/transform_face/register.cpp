#include "transform_face_run.h"
#include "transform_face_continue.h"
#include "transform_face_render.h"

#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/transform_face/nlg/register.h>

namespace NAlice::NHollywood {

REGISTER_SCENARIO(
    "transform_face",
    AddHandle<TTransformFaceRunHandle>()
        .AddHandle<TTransformFaceContinueHandle>()
        .AddHandle<TTransformFaceRenderHandle>()
        .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NTransformFace::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood
