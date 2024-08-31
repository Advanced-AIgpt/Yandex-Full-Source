#include "what_can_you_do_scene.h"

#include "memento.h"
#include "onboarding.h"
#include "skillrec_request_helper.h"

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/device_state/device_state.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/layout.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <dj/services/alisa_skills/server/proto/client/onboarding_response.pb.h>
#include <dj/services/alisa_skills/server/proto/data/data_types.pb.h>

namespace NAlice::NHollywoodFw::NOnboarding {

namespace {
    constexpr TStringBuf WHAT_CAN_YOU_DO_SCENE_NAME = "what_can_you_do";

    TString GetOnboardingScreen(const NAlice::TDeviceState& deviceState) {
        if (!deviceState.GetIsTvPluggedIn()) {
            if (NAlice::IsMusicPlaying(deviceState)) {
                return "music_playing";
            } else if (NAlice::IsRadioPlaying(deviceState)) {
                return "radio_player";
            }
            // TODO: add audio_player case
            return "main";
        } else {
            const auto screenId = NAlice::NVideoCommon::CurrentScreenId(deviceState);
            if (screenId == NVideoCommon::EScreenId::VideoPlayer) {
                // use special onboarding text when video_player is playing tv-stream content
                const auto& videoItem = deviceState.GetVideo().GetCurrentlyPlaying().GetRawItem();
                if (videoItem.GetType() == ToString(NVideoCommon::EContentType::TvStream)) {
                    const auto playerType = videoItem.GetTvStreamInfo().GetIsPersonal() ? "tv_player_personal" : "tv_player";
                    return playerType;
                }
            }
            return ToString(screenId);
        }
    }

    bool GetSource(const TSource& src, TStringBuf key, NDJ::NAS::TOnboardingResponse& out) {
        if (const auto rawResponse = src.GetRawHttpContent(key, /* throwOnFailure */ false)) {
            return out.ParseFromString(*rawResponse);
        }
        return false;
    }

    bool IsEmpty(const NDJ::NAS::TOnboardingResponse& resp) {
        return resp.ItemsSize() == 0;
    }

} // namespace

TWhatCanYouDoScene::TWhatCanYouDoScene(const TScenario* owner)
    : TScene(owner, WHAT_CAN_YOU_DO_SCENE_NAME)
{
    RegisterRenderer(&TWhatCanYouDoScene::Render);
}

TRetSetup TWhatCanYouDoScene::MainSetup(const TWhatCanYouDoSceneArgs&, const TRunRequest& request, const TStorage& storage) const {
    TSetup setup(request);
    if (!request.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_WHAT_CAN_YOU_DO_USE_SKILLREC)) {
        return setup;
    }
    const auto& meta = request.GetRequestMeta();
    TWhatCanYouDoRequest whatCanYouDoRequest{request, storage};
    NHollywood::THttpProxyRequestBuilder proxyRequest(whatCanYouDoRequest.GetPath(), meta, request.Debug().Logger(), TString{WHAT_CAN_YOU_DO_SCENE_NAME});
    proxyRequest.SetMethod(NAppHostHttp::THttpRequest::Post).SetBody(whatCanYouDoRequest.GetBody(), whatCanYouDoRequest.GetContentType());
    setup.Attach(proxyRequest.Build(), SKILLREC_REQUEST_KEY);
    return setup;
}

TRetMain TWhatCanYouDoScene::Main(const TWhatCanYouDoSceneArgs& args, const TRunRequest& runRequest, TStorage& storage, const TSource& src) const {
    TWhatCanYouDoRenderProto renderProto;
    renderProto.SetPhraseIndex(args.GetPhraseIndex());
    renderProto.SetIsTvPlugged(runRequest.Client().GetInterfaces().GetIsTvPlugged());
    TString screenMode = "main";
    if (runRequest.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_WHAT_CAN_YOU_DO_SWITCH_PHRASES)) {
        screenMode = GetOnboardingScreen(runRequest.GetRunRequest().GetBaseRequest().GetDeviceState());
    }

    renderProto.SetScreenMode(screenMode);

    NDJ::NAS::TOnboardingResponse response;
    if (runRequest.Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_WHAT_CAN_YOU_DO_USE_SKILLREC) &&
        GetSource(src, SKILLREC_RESPONSE_KEY, response) && !IsEmpty(response))
    {
        const auto& item = response.GetItems(0);
        renderProto.MutablePhrase()->CopyFrom(item.GetResult().GetDescription());
        UpdateTagStats(runRequest, storage, response.GetItems());
        AddLastViews(runRequest, storage, response.GetItems(), response.GetItemType());
    }

    return TReturnValueRender(&TWhatCanYouDoScene::Render, renderProto);
}

TRetResponse TWhatCanYouDoScene::Render(const TWhatCanYouDoRenderProto& renderProto, TRender& render) {
    render.CreateFromNlg(ToString(WHAT_CAN_YOU_DO_SCENE_NAME), "what_can_you_do_speakers", renderProto);

    NScenarios::TFrameAction actionNext;
    auto& nextPhraseFrame = *actionNext.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableOnboardingWhatCanYouDoSemanticFrame();
    nextPhraseFrame.MutablePhraseIndex()->SetUInt32Value(renderProto.GetPhraseIndex() + 1);
    actionNext.MutableNluHint()->SetFrameName("alice.proactivity.tell_me_more");

    NScenarios::TFrameAction actionDecline;
    actionDecline.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableDoNothingSemanticFrame();
    actionDecline.MutableNluHint()->SetFrameName("alice.proactivity.decline");

    auto& actions = *render.GetResponseBody().MutableFrameActions();
    actions["action_what_can_you_do_next"] = std::move(actionNext);
    actions["action_what_can_you_do_decline"] = std::move(actionDecline);

    if (render.GetRequest().Flags().IsExperimentEnabled(NAlice::NHollywood::EXP_HW_WHAT_CAN_YOU_DO_DONT_STOP_ON_DECLINE)) {
        NScenarios::TFrameAction actionStop;
        actionStop.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableDoNothingSemanticFrame();
        actionStop.MutableNluHint()->SetFrameName("personal_assistant.scenarios.player.pause");
        actions["action_what_can_you_do_stop"] = std::move(actionStop);
    }

    render.GetResponseBody().MutableLayout()->SetShouldListen(true);

    return TReturnValueSuccess();
}

}
