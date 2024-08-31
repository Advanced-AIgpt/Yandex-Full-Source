#include "voice.h"

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/scenarios/voice/nlg/register.h>

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/memento/proto/user_configs.pb.h>

#include <alice/protos/data/contextual_data.pb.h>

#include <util/generic/fwd.h>
#include <util/string/join.h>

using namespace NAlice::NScenarios;
namespace NMemento = ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood {

namespace {

// Frames
constexpr TStringBuf WHISPER_OFF_FRAME = "alice.voice.whisper.turn_off";
constexpr TStringBuf WHISPER_ON_FRAME = "alice.voice.whisper.turn_on";
constexpr TStringBuf WHISPER_SAY_SOMETHING_FRAME = "alice.voice.whisper.say_something";
constexpr TStringBuf WHISPER_WHAT_IS_IT_FRAME = "alice.voice.whisper.what_is_it";

constexpr TStringBuf PROACTIVITY_AGREE_FRAME = "alice.proactivity.confirm";
constexpr TStringBuf PROACTIVITY_DECLINE_FRAME = "alice.proactivity.decline";

// NLG
constexpr TStringBuf WHISPER_NLG = "whisper";

constexpr TStringBuf IRRELEVANT_PHRASE = "irrelevant";
constexpr TStringBuf LOGIN_PROMPT_PHRASE = "login_prompt";
constexpr TStringBuf WHAT_IS_WHISPER_PHRASE = "what_is_whisper";
constexpr TStringBuf WHISPER_MODE_OFF_PHRASE = "whisper_mode_off";
constexpr TStringBuf WHISPER_MODE_ON_PHRASE = "whisper_mode_on";
constexpr TStringBuf WHISPER_SOMETHING_PHRASE = "whisper_something";

// Analytics
constexpr TStringBuf WHISPER_SAY_SOMETHING_INTENT = "whisper.say_something";
constexpr TStringBuf WHISPER_TURN_ON_INTENT = "whisper.turn_on";
constexpr TStringBuf WHISPER_TURN_OFF_INTENT = "whisper.turn_off";
constexpr TStringBuf WHISPER_WHAT_IS_IT_INTENT = "whisper.what_is_it";

// Action ids
constexpr TStringBuf WHISPER_TURN_ON_ACTION_ID = "whisper_turn_on_action";
constexpr TStringBuf DECLINE_ACTION_ID = "decline_action";


TWhisperInfo GetWhisperInfo(const TDataSource* whisperInfoSource) {
    return whisperInfoSource != nullptr
        ? whisperInfoSource->GetWhisperInfo()
        : Default<TWhisperInfo>();
}

class TVoice {
public:
    explicit TVoice(TScenarioHandleContext& ctx)
        : Logger(ctx.Ctx.Logger())
        , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM))
        , Request(RequestProto, ctx.ServiceCtx)
        , MementoWhisperConfig(Request.BaseRequestProto().GetMemento().GetUserConfigs().GetTtsWhisperConfig())
        , WhisperInfo(
            GetWhisperInfo(
                Request.GetDataSource(NAlice::EDataSourceType::WHISPER_INFO)
            )
        )
        , NlgData(Logger, Request)
        , NlgWrapper(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request, ctx.Rng, ctx.UserLang))
        , Builder(&NlgWrapper)
        , BodyBuilder(Builder.CreateResponseBodyBuilder())
    {
    }

    std::unique_ptr<TScenarioRunResponse> MakeResponse() && {
        Run();
        return std::move(Builder).BuildResponse();
    }

private:
    void Run() {
        LogInputSources();

        const auto puid = GetUid(Request);
        if (!puid) {
            LOG_INFO(Logger) << "Processing a user with no login";
        }

        auto& analyticsInfoBuilder = BodyBuilder.CreateAnalyticsInfoBuilder();

        if (Request.Input().FindSemanticFrame(WHISPER_ON_FRAME)) {
            LOG_INFO(Logger) << "Turning whisper mode on";
            analyticsInfoBuilder.SetIntentName(TString{WHISPER_TURN_ON_INTENT});

            if (!puid) {
                PromptLogin();
                return;
            }

            MementoWhisperConfig.SetEnabled(true);
            UpdateMementoWhisperConfig();

            if (WhisperInfo.GetIsAsrWhisper()) {
                SetWhisperModifier(NData::TContextualData::TWhisper::ForcedEnable);
            }
            AddRenderedPhrase(WHISPER_MODE_ON_PHRASE);

        } else if (Request.Input().FindSemanticFrame(WHISPER_OFF_FRAME)) {
            LOG_INFO(Logger) << "Turning whisper mode off";
            analyticsInfoBuilder.SetIntentName(TString{WHISPER_TURN_OFF_INTENT});

            if (!puid) {
                PromptLogin();
                return;
            }

            MementoWhisperConfig.SetEnabled(false);
            UpdateMementoWhisperConfig();

            SetWhisperModifier(NData::TContextualData::TWhisper::ForcedDisable);
            AddRenderedPhrase(WHISPER_MODE_OFF_PHRASE);

        } else if (Request.Input().FindSemanticFrame(WHISPER_WHAT_IS_IT_FRAME)) {
            LOG_INFO(Logger) << "Explaining the whisper mode";
            analyticsInfoBuilder.SetIntentName(TString{WHISPER_WHAT_IS_IT_INTENT});

            if (!MementoWhisperConfig.GetEnabled() && puid) {
                NlgData.Context["whisper_on"] = true;
                AddWhisperTurnOnFrameAction();
            }
            AddRenderedPhrase(WHAT_IS_WHISPER_PHRASE);

        } else if (Request.Input().FindSemanticFrame(WHISPER_SAY_SOMETHING_FRAME)) {
            LOG_INFO(Logger) << "Giving an example of whispering";
            AddRenderedPhrase(WHISPER_SOMETHING_PHRASE);
            analyticsInfoBuilder.SetIntentName(TString{WHISPER_SAY_SOMETHING_INTENT});

        } else {
            LOG_WARN(Logger) << "Valid semantic frame not found";
            Builder.SetIrrelevant();
            AddRenderedPhrase(IRRELEVANT_PHRASE);
        }
    }

    void LogInputSources() {
        TVector<TString> frameNames;
        for (const TSemanticFrame& frame : RequestProto.GetInput().GetSemanticFrames()) {
            frameNames.push_back(frame.GetName());
        }
        LOG_INFO(Logger) << "Frames in request: " << JoinSeq(", ", frameNames);
    }

    void PromptLogin() {
        BodyBuilder.TryAddAuthorizationDirective(
            Request.Interfaces().GetCanOpenYandexAuth()
        );
        AddRenderedPhrase(LOGIN_PROMPT_PHRASE);
    }

    void UpdateMementoWhisperConfig() {
        TMementoChangeUserObjectsDirective mementoDirective;
        AddUserConfigs(mementoDirective, NMemento::EConfigKey::CK_TTS_WHISPER, MementoWhisperConfig);
        *BodyBuilder.GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective() = std::move(mementoDirective);
    }

    void AddUserConfigs(TMementoChangeUserObjectsDirective& mementoDirective, NMemento::EConfigKey key, const NProtoBuf::Message& value) const {
        NMemento::TConfigKeyAnyPair mementoConfig;
        mementoConfig.SetKey(key);
        if (mementoConfig.MutableValue()->PackFrom(value)) {
            *mementoDirective.MutableUserObjects()->AddUserConfigs() = std::move(mementoConfig);
        } else {
            LOG_ERROR(Logger) << "PackFrom failed for user config";
        }
    }

    void SetWhisperModifier(const NData::TContextualData::TWhisper::EHint modifier) {
        NData::TContextualData data;
        data.MutableWhisper()->SetHint(modifier);
        BodyBuilder.AddContextualData(data);
    }

    void AddWhisperTurnOnFrameAction() {
        {
            TFrameAction agreeFrameAction;
            agreeFrameAction.MutableNluHint()->SetFrameName(TString{PROACTIVITY_AGREE_FRAME});
            agreeFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableWhisperTurnOnSemanticFrame();
            BodyBuilder.AddAction(TString{WHISPER_TURN_ON_ACTION_ID}, std::move(agreeFrameAction));
        }
        {
            TFrameAction declineFrameAction;
            declineFrameAction.MutableNluHint()->SetFrameName(TString{PROACTIVITY_DECLINE_FRAME});
            declineFrameAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableDoNothingSemanticFrame();
            BodyBuilder.AddAction(TString{DECLINE_ACTION_ID}, std::move(declineFrameAction));
        }

        BodyBuilder.SetShouldListen(true);
    }

    void AddRenderedPhrase(const TStringBuf phrase) {
        BodyBuilder.AddRenderedTextWithButtonsAndVoice(WHISPER_NLG, phrase, {}, NlgData);
    }

private:
    TRTLogger& Logger;
    const TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    NMemento::TTtsWhisperConfig MementoWhisperConfig;
    const TWhisperInfo WhisperInfo;
    TNlgData NlgData;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TResponseBodyBuilder& BodyBuilder;
};

} // namespace

void TVoiceRunHandle::Do(TScenarioHandleContext& ctx) const {
    ctx.ServiceCtx.AddProtobufItem(*TVoice{ctx}.MakeResponse(), RESPONSE_ITEM);
}

REGISTER_SCENARIO(
    "voice",
    AddHandle<TVoiceRunHandle>()
        .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NVoice::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood
