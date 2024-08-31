#include "repeat.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/player_features/player_features.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/library/device_state/device_state.h>
#include <alice/library/logger/logger.h>

#include <util/string/cast.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

    constexpr TStringBuf FRAME = "personal_assistant.scenarios.repeat";
    constexpr TStringBuf NLG_TEMPLATE = "repeat";
    constexpr TStringBuf NOTHING_RESTORED_PHRASE = "nothing_restored";

    ui64 GetClientTimeOfTheLastPhrase(TRTLogger& logger, const TScenarioRunRequestWrapper& request) {
        ui64 clientTimeMs = 0;
        if (const auto* dataSource = request.GetDataSource(EDataSourceType::DIALOG_HISTORY)) {
            const auto& dialogHistory = dataSource->GetDialogHistory();
            if (!dialogHistory.GetDialogTurns().empty()) {
                const auto& recentTurn = dialogHistory.GetDialogTurns(dialogHistory.GetDialogTurns().size() - 1);
                clientTimeMs = recentTurn.GetClientTimeMs();
                LOG_INFO(logger) << "Found DialogHistory turn record";
            }
        }
        LOG_INFO(logger) << "ClientTimeMs from DialogHistory is " << clientTimeMs;
        return clientTimeMs;
    }

} // namespace

void TRepeatRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto frame = request.Input().CreateRequestFrame(FRAME);

    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder{&nlgWrapper};

    TMaybe<TLayout> layout;
    TMaybe<google::protobuf::Map<TString, NScenarios::TFrameAction>> actions;
    if (const auto* dataSource = request.GetDataSource(EDataSourceType::RESPONSE_HISTORY)) {
        const auto& responseHistory = dataSource->GetResponseHistory();
        if (responseHistory.HasPrevResponse() && responseHistory.GetPrevResponse().HasLayout()) {
            layout = responseHistory.GetPrevResponse().GetLayout();
            actions = responseHistory.GetPrevResponse().GetActions();
        }
    }

    const auto hasPrevResponse = layout.Defined();

    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);
    auto& analyticsInfo = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfo.SetIntentName(TString{FRAME});
    analyticsInfo.SetProductScenarioName("repeat");

    if (!hasPrevResponse) {
        LOG_INFO(ctx.Ctx.Logger()) << "Not found prev response";

        TNlgData nlgData{ctx.Ctx.Logger(), request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_TEMPLATE, NOTHING_RESTORED_PHRASE, /* buttons = */ {},
                                                       nlgData);

        builder.SetIrrelevant();
    } else {
        const auto& deviceState = request.BaseRequestProto().GetDeviceState();
        bool isPlayingSomething = NAlice::IsVideoPlaying(deviceState) || NAlice::IsMusicPlaying(deviceState) ||
                                  NAlice::IsRadioPlaying(deviceState) || NAlice::IsAudioPlaying(deviceState) ||
                                  NAlice::IsBluetoothPlaying(deviceState);
        if (isPlayingSomething) {
            LOG_INFO(ctx.Ctx.Logger()) << "Irrelevant by player";

            builder.SetIrrelevant();
        } else {
            auto clientTimeMs = GetClientTimeOfTheLastPhrase(ctx.Ctx.Logger(), request);
            auto playerFeatures = CalcPlayerFeatures(ctx.Ctx.Logger(), request, TInstant::MilliSeconds(clientTimeMs));
            builder.AddPlayerFeatures(std::move(playerFeatures));
        }
    }

    auto response = std::move(builder).BuildResponse();
    if (hasPrevResponse) {
        *response->MutableResponseBody()->MutableLayout() = std::move(*layout);
        *response->MutableResponseBody()->MutableFrameActions() = std::move(*actions);
    }

    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("repeat",
                  AddHandle<TRepeatRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NRepeat::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
