#include "equalizer.h"

#include <alice/hollywood/library/environment_state/endpoint.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/endpoint/capability.pb.h>


using namespace NAlice::NScenarios;


namespace NAlice::NHollywood {

namespace {


class TEqualizerScenarioEngine {
public:
    TEqualizerScenarioEngine(TScenarioHandleContext& ctx)
        : Logger(ctx.Ctx.Logger())
        , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request(RequestProto, ctx.ServiceCtx)
        , NlgData(Logger, Request)
        , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
        , Builder(&NlgWrapper)
        , BodyBuilder(Builder.CreateResponseBodyBuilder())
        , AnalyticsInfoBuilder(BodyBuilder.CreateAnalyticsInfoBuilder())
        , PushUrl(PushUrlPrefix + FindDeviceId())
        , SupportsEqualizer_(ComputeWhetherSupportsEqualizer())
    {
    }

    std::unique_ptr<TScenarioRunResponse> MakeResponse() && {
        Run();
        return std::move(Builder).BuildResponse();
    }

private:
    void Run() {
        if (!FindValidSemanticFrame()) {
            LOG_INFO(Logger) << "Valid semantic frame not found";
            MakeIrrelevantResponse();
        } else if (!SupportsEqualizer_ && CanShowDevicesList() && HasSmartSpeakers()) {
            AddDirectiveToOpenDevicesListScreen();
            RenderReplyPhrase();
        } else if (!SupportsEqualizer_) {
            LOG_INFO(Logger) << "Surface doesn't support equalizer";
            MakeIrrelevantResponse();
        } else {
            AddPushWithLinkToQuasarSettings();
            RenderReplyPhrase();
        }
        MakeAnalyticsInfo();
    }

    bool CanShowDevicesList() const {
        return Request.Interfaces().GetCanOpenLinkYellowskin();
    }

    bool HasSmartSpeakers() {
        const auto* ioTUserInfoPtr = Request.GetDataSource(EDataSourceType::IOT_USER_INFO);
        if (!ioTUserInfoPtr) {
            return false;
        }

        const auto& ioTUserInfo = ioTUserInfoPtr->GetIoTUserInfo();
        return AnyOf(ioTUserInfo.GetDevices(), [](const auto& device) {
            return !device.GetQuasarInfo().GetDeviceId().empty();
        });
    }

    void AddDirectiveToOpenDevicesListScreen() {
        TDirective directive;
        TOpenUriDirective& openUriDirective = *directive.MutableOpenUriDirective();
        openUriDirective.SetUri(ToString(AllDevicesScreenLink));
        BodyBuilder.AddDirective(std::move(directive));
    }

    bool ComputeWhetherSupportsEqualizer() const {
        if (Request.Interfaces().GetHasEqualizer()) {
            return true;
        }

        const auto* envState = GetEnvironmentStateProto(Request);
        if (!envState) {
            return false;
        }

        const auto* endpoint = NHollywood::FindEndpoint(*envState, Request.ClientInfo().DeviceId);
        if (!endpoint) {
            return false;
        }

        TEqualizerCapability capability;
        return NHollywood::ParseTypedCapability(capability, *endpoint);
    }

    bool FindValidSemanticFrame() {
        return AnyOf(ValidFramesNames, [&](const TStringBuf frameName) {
            if (FindFrame(frameName)) {
                FrameName = frameName;
                return true;
            }
            return false;
        });
    }

    void AddPushWithLinkToQuasarSettings() {
        TPushDirectiveBuilder(PushTitle, PushText, PushUrl, PushTag)
            .SetPushId(PushTag)
            .SetThrottlePolicy(PushPolicy)
            .SetAnalyticsAction(FrameName, FrameName, ToString(SendingPushDescription))
            .BuildTo(BodyBuilder);
    }

    void MakeIrrelevantResponse() {
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(NlgTemplateName, NlgRenderReplyErrorName, {}, NlgData);
        Builder.SetIrrelevant();
    }

    void RenderReplyPhrase() {
        NlgData.Context["frame_name"] = FrameName;
        NlgData.Context["current_device_supports_equalizer"] = SupportsEqualizer_;
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(NlgTemplateName, NlgRenderReplyPhraseName, {}, NlgData);
    }

    void MakeAnalyticsInfo() {
        AnalyticsInfoBuilder.SetIntentName(FrameName);
        AnalyticsInfoBuilder.SetProductScenarioName(ToString(ProductScenarioName));
    }

    bool FindFrame(const TStringBuf name) const {
        return Request.Input().FindSemanticFrame(name).IsValid();
    }

    TString FindDeviceId() const {
        const auto* iotUserInfoPtr = Request.GetDataSource(EDataSourceType::IOT_USER_INFO);
        if (!iotUserInfoPtr) {
            return "";
        }

        const auto& ioTUserInfo = iotUserInfoPtr->GetIoTUserInfo();
        const auto deviceQuasarDeviceId = Request.ClientInfo().DeviceId;

        const auto* device = FindIfPtr(ioTUserInfo.GetDevices(), [&deviceQuasarDeviceId](const auto& device) {
            return device.GetQuasarInfo().GetDeviceId() == deviceQuasarDeviceId;
        });
        return device ? ToString(device->GetId()) : TString();
    }

private:
    TRTLogger& Logger;
    const TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    TNlgData NlgData;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TResponseBodyBuilder& BodyBuilder;
    IAnalyticsInfoBuilder& AnalyticsInfoBuilder;
    TString FrameName;
    TString PushUrl;
    const bool SupportsEqualizer_;

    static constexpr TStringBuf EqualizerEnableFrameName = "alice.equalizer.enable";
    static constexpr TStringBuf EqualizerDisableFrameName = "alice.equalizer.disable";
    static constexpr TStringBuf EqualizerWhichPresetIsSetFrameName = "alice.equalizer.which_preset_is_set";
    static constexpr TStringBuf EqualizerAddMoreBassFrameName = "alice.equalizer.more_bass";
    static constexpr TStringBuf EqualizerMakeLessBassFrameName = "alice.equalizer.less_bass";
    static constexpr TStringBuf EqualizerHowToSetFrameName = "alice.equalizer.how_to_set";
    static constexpr TStringBuf ValidFramesNames[] = {
        EqualizerEnableFrameName, EqualizerDisableFrameName, EqualizerWhichPresetIsSetFrameName,
        EqualizerAddMoreBassFrameName, EqualizerMakeLessBassFrameName, EqualizerHowToSetFrameName
    };
    static constexpr TStringBuf NlgTemplateName = "equalizer";
    static constexpr TStringBuf NlgRenderReplyPhraseName = "render_reply";
    static constexpr TStringBuf NlgRenderReplyErrorName = "render_unsupported";

    static constexpr TStringBuf ProductScenarioName = "equalizer";
    static constexpr TStringBuf SendingPushDescription = "В приложение Яндекса отправляется пуш, по нажатию "
                                                         "на который пользователь может попасть в настройки "
                                                         "эквалайзера для данной колонки";
    static constexpr TStringBuf AllDevicesScreenLink = "ya-search-app-open://?uri=yellowskin://?url=https%3A%2F%2Fyandex.ru%2Fquasar";

    static const TString PushTitle;
    static const TString PushText;
    static const TString PushUrlPrefix;
    static const TString PushTag;
    static const TString PushPolicy;
};

const TString TEqualizerScenarioEngine::PushTitle = "Настройка звука";
const TString TEqualizerScenarioEngine::PushText = "Нажмите, чтобы настроить звук колонки";
const TString TEqualizerScenarioEngine::PushUrlPrefix = "ya-search-app-open://?uri=yellowskin://?url=https%3A%2F%2Fyandex.ru%2Fquasar%2Fexternal%2Fopen-equalizer%3FdeviceId%3D";
const TString TEqualizerScenarioEngine::PushTag = "alice.equalizer";
const TString TEqualizerScenarioEngine::PushPolicy = "unlimited_policy";

}  // namespace

void TEqualizerRunHandle::Do(TScenarioHandleContext& ctx) const {
    ctx.ServiceCtx.AddProtobufItem(*TEqualizerScenarioEngine{ctx}.MakeResponse(), RESPONSE_ITEM);
}

REGISTER_SCENARIO("equalizer",
                  AddHandle<TEqualizerRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NEqualizer::NNlg::RegisterAll));

} // namespace NAlice::NHollywood
