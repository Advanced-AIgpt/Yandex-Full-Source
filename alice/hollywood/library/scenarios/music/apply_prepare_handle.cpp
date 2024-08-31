#include "apply_prepare_handle.h"

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_request_init.h>
#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

void TApplyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    LOG_INFO(logger) << "TMusicArguments are "
                     << SerializeProtoText(applyArgs, /* singleLineMode= */ true, /* expandAny= */ true);

    Y_ENSURE(request.Interfaces().GetHasAudioClient());
    Y_ENSURE(request.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) ||
             request.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE));
    AddMusicContentRequest(ctx, request);
}

} // namespace NAlice::NHollywood::NMusic
