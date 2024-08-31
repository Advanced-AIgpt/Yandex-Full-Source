#include "skillrec_request_helper.h"

#include "greetings_consts.h"

#include <alice/library/network/common.h>
#include <alice/library/onboarding/enums.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <library/cpp/protobuf/json/config.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/string_utils/quote/quote.h>


namespace NAlice::NHollywoodFw::NOnboarding {

    namespace {

        constexpr TStringBuf WHAT_CAN_YOU_DO_VOICE_CARD_NAME = "what_can_you_do_voice";

        // common
        [[nodiscard]] static TString QuoteString(TString str) {
            Quote(str);
            return str;
        }

        void InsertCgiParams(const TStringBuf requestId, const TStringBuf cardName, TCgiParameters& cgi ) {
            cgi.InsertUnescaped("card_name", cardName);
            cgi.InsertUnescaped("msid", requestId);
            cgi.InsertUnescaped("client", "hollywood");
        }
    }

    // class TGreetingsRequestOld
    TGreetingsRequestOld::TGreetingsRequestOld(const TRunRequest& request) {
        LOG_INFO(request.Debug().Logger()) << "TServiceRequest is created";
        SetupCgi(request);
        SetupProtoReq(request);
    };

    TString TGreetingsRequestOld::GetPath() const {
        return "/recommender?" + Cgi_.Print();
    };

    TStringBuf TGreetingsRequestOld::GetContentType() const {
        return NContentTypes::APPLICATION_JSON;
    };

    TString TGreetingsRequestOld::GetBody() const {
        NProtobufJson::TProto2JsonConfig cfg;
        cfg.FieldNameMode = NProtobufJson::TProto2JsonConfig::FieldNameSnakeCase;
        return NProtobufJson::Proto2Json(ProtoReq_, cfg);
    };

    void TGreetingsRequestOld::SetupCgi(const TRunRequest& request) {
        InsertCgiParams(request.System().RequestId(), GREETINGS_CARD_NAME, Cgi_);
    };

    void TGreetingsRequestOld::SetupProtoReq(const TRunRequest& request) {
        ProtoReq_.SetCardName(TString{GREETINGS_CARD_NAME});
        ProtoReq_.SetRngSeed(request.GetRequestMeta().GetRandomSeed());
        if (auto scaleFactor = request.GetRunRequest().GetBaseRequest().GetOptions().GetScreenScaleFactor()) {
            ProtoReq_.SetScreenScaleFactor(scaleFactor);
        }
        FillClientProperties(request.Client().GetClientInfo());
        FillFeatures(request.Client().GetInterfaces());
        FillFlags(request.Flags());
    };

    void TGreetingsRequestOld::FillClientProperties(const TClientInfo& info) {
        if (auto* userData = ProtoReq_.MutableUserData()) {
            userData->SetDeviceId(info.DeviceId);
            userData->SetClientId(QuoteString(info.ClientId));
            userData->SetUuid(info.Uuid);
        }
        ProtoReq_.SetLanguage(info.Lang);
        ProtoReq_.SetTimetz(QuoteString(TStringBuilder() << info.Epoch << '@' << info.Timezone));
    };

    void TGreetingsRequestOld::FillFeatures(const NScenarios::TInterfaces& interfaces) {
        using namespace NAlice::NOnboarding;
        if (interfaces.GetCanSetTimer()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::Timers));
        }
        if (interfaces.GetCanSetAlarm()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::Alarms));
        }
        if (interfaces.GetCanRecognizeImage()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::ImageRecognizer));
        }
        if (interfaces.GetCanRecognizeMusic()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::MusicRecognizer));
        }
        if (interfaces.GetCanRenderDivCards()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::DivCards));
        }
        if (interfaces.GetCanOpenWhocalls()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::WhoCalls));
        }
        if (interfaces.GetCanOpenKeyboard()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::Keyboard));
        }
        if (interfaces.GetHasNavigator()) {
            ProtoReq_.AddPlatformFeatures(ToString(ESkillRequirement::Navigator));
        }
    };

    void TGreetingsRequestOld::FillFlags(const TRequest::TFlags& flags) {
        flags.ForEach([this](const TString& key, const TMaybe<TString>&) {
            this->ProtoReq_.AddAliceExperiments(key);
            return true;
        });
    };

    // class TOnboardingRequest
    TOnboardingRequest::TOnboardingRequest(const TRunRequest& request, const TStorage& storage, const TStringBuf cardName) {
        SetupCgi(request, cardName);
        SetupProtoReq(request, storage);
    };

    TStringBuf TOnboardingRequest::GetContentType() const {
        return NContentTypes::APPLICATION_PROTOBUF;
    };

    TString TOnboardingRequest::GetBody() const {
        TString body;
        Y_ENSURE(ProtoReq_.SerializeToString(&body), "Request cannot be serialized");
        return body;
    };

    void TOnboardingRequest::SetupCgi(const TRunRequest& request, const TStringBuf cardName) {
        InsertCgiParams(request.System().RequestId(), cardName, Cgi_);
    };

    void TOnboardingRequest::SetupProtoReq(const TRunRequest& request, const TStorage& storage) {
        ProtoReq_.SetRngSeed(request.GetRequestMeta().GetRandomSeed());
        ProtoReq_.MutableOptions()->CopyFrom(request.GetRunRequest().GetBaseRequest().GetOptions());
        // client properties
        const TClientInfoProto info = request.GetRunRequest().GetBaseRequest().GetClientInfo();
        ProtoReq_.MutableClientInfo()->CopyFrom(info);
        // interfaces
        const NScenarios::TInterfaces interfaces = request.Client().GetInterfaces();
        ProtoReq_.MutableInterfaces()->CopyFrom(interfaces);
        // flags
        request.Flags().ForEach([this](const TString& key, const TMaybe<TString>&) {
            this->ProtoReq_.AddExperiments(key);
            return true;
        });
        ProtoReq_.MutableTagStatsStorage()->CopyFrom(storage.GetMementoUserConfig().GetProactivityTagStats());
    };

    // class TGreetingsRequestNew
    TGreetingsRequestNew::TGreetingsRequestNew(const TRunRequest& request, const TStorage& storage)
        : TOnboardingRequest(request, storage, GREETINGS_SCENE_NAME)
    {
        LOG_INFO(request.Debug().Logger()) << "TGreetingsRequestNew is created";
    };

    TString TGreetingsRequestNew::GetPath() const {
        return "/greetings?" + Cgi_.Print();
    };

    // class TWhatCanYouDoRequest
    TWhatCanYouDoRequest::TWhatCanYouDoRequest(const TRunRequest& request, const TStorage& storage)
        : TOnboardingRequest(request, storage, WHAT_CAN_YOU_DO_VOICE_CARD_NAME)
    {
        LOG_INFO(request.Debug().Logger()) << "TWhatCanYouDoRequest is created";
    };

    TString TWhatCanYouDoRequest::GetPath() const {
        return "/what_can_you_do?" + Cgi_.Print();
    };

} // namespace NAlice::NHollywoodFw::NOnboarding
