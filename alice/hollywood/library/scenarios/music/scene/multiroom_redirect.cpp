#include "multiroom_redirect.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>

#include <alice/hollywood/library/frame_redirect/frame_redirect.h>
#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <util/generic/xrange.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

NAlice::TSemanticFrame ConstructSemanticFrame(const NHollywood::TFrame& frame, const google::protobuf::Message& tsfProto) {
    auto frameProto = frame.ToProto();
    auto& tsfField = *frameProto.MutableTypedSemanticFrame();

    const auto* descriptor = tsfField.GetDescriptor();
    const auto* reflection = tsfField.GetReflection();

    for (const int i : xrange(descriptor->field_count())) {
        const auto* fieldDescriptor = descriptor->field(i);
        if (fieldDescriptor->message_type() == tsfProto.GetDescriptor()) {
            reflection->MutableMessage(&tsfField, fieldDescriptor)->CopyFrom(tsfProto);
        }
    }

    return frameProto;
}

} // namespace

TMaybe<TMusicScenarioSceneArgsMultiroomRedirect> TryCreateMultiroomRedirectSceneArgs(
    const TScenarioRequestData& requestData,
    const google::protobuf::Message& tsfProto,
    const bool addPlayerFeature,
    const bool onlyToMasterDevice)
{
    const auto& req = *static_cast<const TRunRequest*>(&requestData.Request);

    if (!req.Client().GetInterfaces().GetMultiroomAudioClient()) {
        return Nothing();
    }

    NHollywood::TFrameRedirectTypeFlags flags;
    if (req.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_MULTIROOM_REDIRECT)) {
        flags |= NHollywood::EFrameRedirectType::Server;
    }
    if (req.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_MULTIROOM_CLIENT_REDIRECT)) {
        flags |= NHollywood::EFrameRedirectType::Client;
    }
    if (!flags) {
        return Nothing();
    }

    NHollywood::TMultiroom multiroom{req};

    const auto& frameName = tsfProto.GetDescriptor()->options().GetExtension(SemanticFrameName);
    const auto& frame = *req.Input().FindSemanticFrame(frameName).Get();
    const auto frameProto = ConstructSemanticFrame(frame, tsfProto);

    if (!frameProto.HasTypedSemanticFrame()) {
        LOG_ERROR(req.Debug().Logger()) << "Frame has no TypedSemanticFrame! Cancel redirect.";
        return Nothing();
    }

    // create lambda to avoid copy-paste
    const auto createRedirectResponse = [&](TStringBuf targetDeviceId, bool stopPlaying)
    {
        LOG_INFO(req.Debug().Logger()) << "Redirect to device " << targetDeviceId;

        TMusicScenarioSceneArgsMultiroomRedirect sceneArgs;
        if (stopPlaying) {
            sceneArgs.MutableClearQueueDirective();
        }

        TStringBuf uid;
        if (const auto* ds = req.GetDataSource(NAlice::EDataSourceType::BLACK_BOX)) {
            uid = ds->GetUserInfo().GetUid();
        }

        auto frameRedirectInfo = NHollywood::TFrameRedirectBuilder{frameProto, uid, targetDeviceId, flags}
            .SetProductScenarioName(NAlice::NProductScenarios::MUSIC)
            .SetAddPlayerFeatures(addPlayerFeature)
            .BuildResponse();
        if (frameRedirectInfo.Directive) {
            *sceneArgs.MutableMultiroomSemanticFrameDirective() = std::move(*frameRedirectInfo.Directive);
        }
        if (frameRedirectInfo.ServerDirective) {
            *sceneArgs.MutablePushTypedSemanticFrameDirective() = std::move(*frameRedirectInfo.ServerDirective);
        }
        if (frameRedirectInfo.PlayerFeatures) {
            *sceneArgs.MutablePlayerFeatures() = std::move(*frameRedirectInfo.PlayerFeatures);
        }
        return sceneArgs;
    };

    // check if it is an alarm request
    if (frame.FindSlot(NAlice::NMusic::SLOT_ALARM_ID)) {
        LOG_INFO(req.Debug().Logger()) << "It is an alarm request, don't redirect frame";
        return Nothing();
    }

    // check if we asked about a peer in other room (or about single station)
    if (NHollywood::FrameHasSomeLocationSlot(frame) && !onlyToMasterDevice) {
        const auto locationInfo = NHollywood::MakeLocationInfo(frame);

        if (multiroom.IsDeviceInLocation(locationInfo)) {
            // we already on the needed location, process the request on this device
            // (it will stop the current MR session)
            return Nothing();
        }

        const TMaybe<TStringBuf> peerDeviceId = multiroom.FindVisiblePeerFromLocationInfo(locationInfo);

        if (peerDeviceId) {
            // we are NOT on the needed location, redirect the device and maybe stop the current multiroom
            const bool stopPlayingCurrentMr = multiroom.LocationIntersectsWithPlayingDevices(locationInfo);
            return createRedirectResponse(*peerDeviceId, stopPlayingCurrentMr);
        } else {
            // there are no devices from the needed location in `visible peers`, ignore the request...
            LOG_INFO(req.Debug().Logger()) << "User asked for missing location " << locationInfo.ShortUtf8DebugString().Quote();
        }
    }

    // check if the latest player is the old player despite the flags
    if (!NHollywood::NMusic::IsAudioPlayerVsMusicTheLatest(requestData.GetProto<TDeviceState>())) {
        LOG_INFO(req.Debug().Logger()) << "Music player is more recent than audio player, don't redirect frame";
        return Nothing();
    }

    if (!NHollywood::NMusic::IsAudioPlayerPlaying(requestData.GetProto<TDeviceState>())) {
        // allow to redirect only "Continue" frame
        if (frame.Name() != NAlice::NMusic::PLAYER_CONTINUE) {
            LOG_INFO(req.Debug().Logger()) << "The music player is not playing, don't redirect frame";
            return Nothing();
        }
    }

    // check if multiroom is not active and frame doesn't contain activation slots
    if (!multiroom.IsActiveWithFrame(frame)) {
        return Nothing();
    }

    // try get master device id (fails if device is not slave)
    TStringBuf masterDeviceId;
    if (!multiroom.IsDeviceSlave(masterDeviceId)) {
        return Nothing();
    }

    // check if the request is onyourwave-ish
    if (NHollywood::NMusic::IsOnYourWaveRequest(frame)) {
        LOG_INFO(req.Debug().Logger()) << "The request seem to be onyourwave-ish, don't redirect (corner case)";
        return Nothing();
    }

    // build frame redirect response
    return createRedirectResponse(masterDeviceId, /* stopPlaying = */ false);
}

TMusicScenarioSceneMultiroomRedirect::TMusicScenarioSceneMultiroomRedirect(const TScenario* owner)
    : TScene{owner, "multiroom_redirect"}
{
}

TRetMain TMusicScenarioSceneMultiroomRedirect::Main(const TMusicScenarioSceneArgsMultiroomRedirect& sceneArgs,
                                                    const TRunRequest&,
                                                    TStorage&,
                                                    const TSource&) const
{
    TRunFeatures runFeatures;
    TMusicScenarioRenderArgsCommon renderArgs;

    if (sceneArgs.HasPlayerFeatures()) {
        const auto& playerFeatures = sceneArgs.GetPlayerFeatures();
        runFeatures.SetPlayerFeatures(playerFeatures.GetRestorePlayer(), playerFeatures.GetSecondsSincePause());
    }

    if (sceneArgs.HasMultiroomSemanticFrameDirective()) {
        *renderArgs.AddDirectiveList()->MutableMultiroomSemanticFrameDirective() =
            sceneArgs.GetMultiroomSemanticFrameDirective();
    }

    if (sceneArgs.HasPushTypedSemanticFrameDirective()) {
        *renderArgs.AddServerDirectiveList()->MutablePushTypedSemanticFrameDirective() =
            sceneArgs.GetPushTypedSemanticFrameDirective();
    }

    if (sceneArgs.HasClearQueueDirective()) {
        *renderArgs.AddDirectiveList()->MutableClearQueueDirective() =
            sceneArgs.GetClearQueueDirective();
    }

    return TReturnValueRender(&CommonRender, renderArgs, std::move(runFeatures));
}

} // namespace NAlice::NHollywoodFw::NMusic
