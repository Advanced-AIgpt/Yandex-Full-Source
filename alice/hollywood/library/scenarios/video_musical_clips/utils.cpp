#include "musical_clips_defs.h"
#include "utils.h"

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMusicalClips {

void AddIrrelevantResponse(NAlice::NHollywood::TScenarioHandleContext& ctx) {
    NAlice::NHollywood::TRunResponseBuilder responseBuilder;
    responseBuilder.SetIrrelevant();
    responseBuilder.CreateResponseBodyBuilder();
    ctx.ServiceCtx.AddProtobufItem(*std::move(responseBuilder).BuildResponse(), NAlice::NHollywood::RESPONSE_ITEM);
}

NJson::TJsonValue GetCurrentlyPlaying(const TScenarioRunRequestWrapper& request) {
    if (const auto* dataSource = request.GetDataSource(NAlice::EDataSourceType::VIDEO_CURRENTLY_PLAYING)) {
        const auto& currentlyPlaying = dataSource->GetVideoCurrentlyPlaying();
        return NAlice::JsonFromProto(currentlyPlaying.GetCurrentlyPlaying());
    }
    return NJson::TJsonValue();
}

bool CheckScenarioAppropriateSlots(TScenarioHandleContext& ctx, const TFrame* frame) {
    if (const auto fromPtr = frame->FindSlot(TStringBuf("clip_genre"))) {
        LOG_INFO(ctx.Ctx.Logger()) << "CheckScenarioAppropriateSlots, clip_genre found.";
        return false;
    }

    if (const auto toPtr = frame->FindSlot(TStringBuf("clip_related_object"))) {
        LOG_INFO(ctx.Ctx.Logger()) << "CheckScenarioAppropriateSlots, clip_related_object found.";
        return false;
    }

    if (const auto toPtr = frame->FindSlot(TStringBuf("content_provider"))) {
        LOG_INFO(ctx.Ctx.Logger()) << "CheckScenarioAppropriateSlots, content_provider found.";
        return false;
    }

    return true;
}

} // namespace NAlice::NHollywood::NMusicalClips
