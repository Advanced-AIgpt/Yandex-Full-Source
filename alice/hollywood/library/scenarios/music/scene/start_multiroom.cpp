#include "start_multiroom.h"

#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

#include <alice/hollywood/library/frame_redirect/frame_redirect.h>
#include <alice/hollywood/library/multiroom/multiroom.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

namespace {

class TStartMultiroomHelper {
public:
    TStartMultiroomHelper(const TMusicScenarioSceneArgsStartMultiroom& sceneArgs,
                          const TRunRequest& request,
                          TStorage& storage,
                          const TSource& source)
        : SceneArgs_{sceneArgs}
        , Request_{request}
        , Storage_{storage}
        , Source_{source}

        , RequestData_{.Request = Request_, .Storage = Storage_, .Source = &Source_}
        , State_{RequestData_}

        , RenderData_{}

        , DeviceState_{Request_.Client().TryGetMessage<TDeviceState>().GetOrElse(TDeviceState{})}
        , Multiroom_{Request_}
    {
    }

    TCommonRenderData ConstructRenderData() {
        LOG_INFO(Request_.Debug().Logger()) << "Got start multiroom request: " << SceneArgs_;

        RenderData_.FillRunFeatures(RequestData_);

        if (!IsPlayingSomething() || !State_.MusicQueue.HasCurrentItem()) {
            auto& nlgData = *RenderData_.RenderArgs.MutableNlgData();
            nlgData.SetTemplate("multiroom");
            nlgData.SetPhrase("nothing_is_playing");
            return std::move(RenderData_);
        }

        // find a deviceId from `locationInfo`
        const auto& locationInfo = SceneArgs_.GetLocationInfo();
        TMaybe<TStringBuf> deviceIdFromLocationInfo;
        if (Multiroom_.IsDeviceInLocation(locationInfo)) {
            // in this case we will send `multiroom_semantic_frame` to the same device
            deviceIdFromLocationInfo = Request_.Client().GetClientInfo().DeviceId;
        } else {
            deviceIdFromLocationInfo = Multiroom_.FindVisiblePeerFromLocationInfo(locationInfo);
        }

        if (!deviceIdFromLocationInfo.Defined()) {
            auto& nlgData = *RenderData_.RenderArgs.MutableNlgData();
            nlgData.SetTemplate("multiroom");
            nlgData.SetPhrase("dont_know_place");
            return std::move(RenderData_);
        }

        RenderData_.RenderArgs.AddDirectiveList()->MutableClearQueueDirective();

        NAlice::TSemanticFrame frame;
        *frame.MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame() = ConstructFrameFromMusicQueue();

        auto frameRedirectData = NHollywood::TFrameRedirectBuilder{frame, GetUid(), *deviceIdFromLocationInfo, NHollywood::EFrameRedirectType::Client}
            .SetProductScenarioName(NAlice::NProductScenarios::MUSIC)
            .BuildResponse();
        Y_ENSURE(frameRedirectData.Directive.Defined());

        *RenderData_.RenderArgs.AddDirectiveList()->MutableMultiroomSemanticFrameDirective()
            = std::move(*frameRedirectData.Directive);

        return std::move(RenderData_);
    }

private:
    bool IsPlayingSomething() {
        return DeviceState_.GetAudioPlayer().GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing;
    }

    double GetOffsetSec() {
        return DeviceState_.GetAudioPlayer().GetOffsetMs() / 1000.0;
    }

    TMusicPlaySemanticFrame ConstructFrameFromMusicQueue() {
        const auto& currentItem = State_.MusicQueue.CurrentItem();
        const auto& originContentId = currentItem.GetOriginContentId();

        TMusicPlaySemanticFrame frame;

        switch (originContentId.GetType()) {
        case NHollywood::NMusic::TContentId_EContentType_Album:
            frame.MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Album);
            frame.MutableObjectId()->SetStringValue(originContentId.GetId());
            frame.MutableStartFromTrackId()->SetStringValue(currentItem.GetTrackId());
            break;
        case NHollywood::NMusic::TContentId_EContentType_Artist:
            frame.MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Artist);
            frame.MutableObjectId()->SetStringValue(originContentId.GetId());
            frame.MutableStartFromTrackId()->SetStringValue(currentItem.GetTrackId());
            break;
        case NHollywood::NMusic::TContentId_EContentType_Playlist:
            frame.MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Playlist);
            frame.MutableObjectId()->SetStringValue(originContentId.GetId());
            frame.MutableStartFromTrackId()->SetStringValue(currentItem.GetTrackId());
            break;
        default:
            // for Track, Radio, ...
            // TODO(sparkle): custom answer for FmRadio, Generative?
            frame.MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot_EValue_Track);
            frame.MutableObjectId()->SetStringValue(currentItem.GetTrackId());
            break;
        }

        frame.MutableOffsetSec()->SetDoubleValue(GetOffsetSec());
        *frame.MutableLocation() = NHollywood::ConstructLocationSlotFromLocationInfo(SceneArgs_.GetLocationInfo());

        return frame;
    }

    TStringBuf GetUid() {
        TStringBuf uid;
        if (const auto* ds = Request_.GetDataSource(NAlice::EDataSourceType::BLACK_BOX)) {
            uid = ds->GetUserInfo().GetUid();
        }
        return uid;
    }

private:
    // source objects
    const TMusicScenarioSceneArgsStartMultiroom& SceneArgs_;
    const TRunRequest& Request_;
    TStorage& Storage_;
    const TSource& Source_;

    // helper structs
    TScenarioRequestData RequestData_;
    TScenarioStateData State_;

    // render data
    TCommonRenderData RenderData_;

    // other structs
    const TDeviceState DeviceState_;
    NHollywood::TMultiroom Multiroom_;
};

} // namespace

bool CanProcessStartMultiroom(const TRunRequest& request) {
    return request.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_THIN_CLIENT_MULTIROOM);
}

TMusicScenarioSceneArgsStartMultiroom BuildStartMultiroomSceneArgs(const TStartMultiroomSemanticFrame& frame) {
    TMusicScenarioSceneArgsStartMultiroom sceneArgs;
    *sceneArgs.MutableLocationInfo() = NHollywood::MakeLocationInfo(frame);
    return sceneArgs;
}

TMusicScenarioSceneStartMultiroom::TMusicScenarioSceneStartMultiroom(const TScenario* owner)
    : TScene{owner, "start_multiroom"}
{
}

TRetMain TMusicScenarioSceneStartMultiroom::Main(const TMusicScenarioSceneArgsStartMultiroom& sceneArgs,
                                                 const TRunRequest& request,
                                                 TStorage& storage,
                                                 const TSource& source) const
{
    TCommonRenderData renderData = TStartMultiroomHelper{sceneArgs, request, storage, source}.ConstructRenderData();
    return TReturnValueRender(&CommonRender, renderData.RenderArgs, std::move(renderData.RunFeatures));
}

} // namespace NAlice::NHollywoodFw::NMusic
