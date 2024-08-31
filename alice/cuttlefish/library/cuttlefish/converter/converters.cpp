#include "converters.h"
#include "utils.h"

#include <alice/cuttlefish/library/convert/builder.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>
#include <alice/cuttlefish/library/proto_converters/converter_handlers.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/events.traits.pb.h>
#include <alice/cuttlefish/library/protos/session.traits.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.traits.pb.h>

#include <alice/library/util/charchecker.h>

#include <util/charset/utf8.h>
#include <util/generic/map.h>
#include <util/string/ascii.h>
#include <util/string/strip.h>
#include <util/string/util.h>



namespace NAlice::NCuttlefish::NAppHostServices::NConverter {

namespace {

struct TVoiceQuality {
    static inline void Parse(const NJson::TJsonValue& src, NAliceProtocol::TVoiceOptions& dst) {
        NAliceProtocol::TVoiceOptions::EVoiceQuality val;
        if (NAliceProtocol::TVoiceOptions::EVoiceQuality_Parse(ToUpperUTF8(src.GetStringSafe()), &val)) {
            dst.SetQuality(val);
        }
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&&, const NAliceProtocol::TVoiceOptions&) { }
};



struct TExperimentsHandler {
    static inline void Parse(const NJson::TJsonValue& node, NAliceProtocol::TSynchronizeStateEvent& msg)
    {
        NAlice::NCuttlefish::NExpFlags::ParseExperiments(node, *msg.MutableExperiments());
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& writer, const NAliceProtocol::TSynchronizeStateEvent& msg)
    {
        auto map = writer.Map();
        for (const auto& it : msg.GetExperiments().GetStorage()) {
            map.Key(it.first);
            if (it.second.HasString()) {
                map.Value(it.second.GetString());
            } else if (it.second.HasBoolean()) {
                map.Value(it.second.GetBoolean());
            } else if (it.second.HasNumber()) {
                map.Value(it.second.GetNumber());
            } else if (it.second.HasInteger()) {
                map.Value(it.second.GetInteger());
            } else {
                map.Value("1");
            }
        }
    }
};


struct TUuidHandler {
    static inline void Parse(const NJson::TJsonValue& src, NAliceProtocol::TUserInfo& dst) {
        const TStringBuf uuid = src.GetStringSafe();
        if (!CheckUuid(uuid))
            throw TConvertException() << "Invalid uuid";
        dst.SetUuid(ToAsciiLower(uuid));
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& w, const NAliceProtocol::TUserInfo& src) {
        w.Value(src.GetUuid());
    }
};


template <typename FieldTraits>
struct TNormalizedDeviceField {
    static inline void Parse(const NJson::TJsonValue& src, NAliceProtocol::TDeviceInfo& dst) {
        TString str = ToLowerUTF8(src.GetStringSafe());
        ReplaceChars<' ', '_', '-', '_'>(str);
        FieldTraits::Set(dst, std::move(str));
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& w, const NAliceProtocol::TDeviceInfo& src) {
        w.Value(FieldTraits::Get(src));
    }
};


TConverter<NAliceProtocol::TSynchronizeStateEvent> CreateSynchronizeStateEventConverter()
{
    using NAlice::NCuttlefish::NConvert::TStrictConvert;
    using namespace NProtoTraits::NAliceProtocol;

    static const TConverter<NAliceProtocol::TDeviceInfo::TWifiNetwork> WifiNetworkConverter = [](){
        TConverter<NAliceProtocol::TDeviceInfo::TWifiNetwork> conv;
        auto b = conv.Build();
        b.SetValue<TDeviceInfo::TWifiNetwork::Mac>("mac");
        b.SetValue<TDeviceInfo::TWifiNetwork::SignalStrength>("signal_strength");
        return conv;
    }();

    TConverter<NAliceProtocol::TSynchronizeStateEvent> conv;
    {
        auto b = conv.Build();
        b.SetValue<TSynchronizeStateEvent::AppToken, TStrictConvert>("event/payload/auth_token");
        b.SetValue<TSynchronizeStateEvent::UserAgent, TStrictConvert>("event/payload/user_agent");
        b.ForEachInArray().Append<TSynchronizeStateEvent::UaasTests>("event/payload/uaas_tests");
        b.SetValue<TSynchronizeStateEvent::ICookie>("event/payload/icookie");
        b.SetValue<TSynchronizeStateEvent::ServiceName>("event/payload/service_name");
    }
    {
        auto b = conv.Build().Sub<TSynchronizeStateEvent::UserInfo>();
        b.Custom<TUuidHandler>("event/payload/uuid");
        b.SetValue<TUserInfo::Guid>("event/payload/guid");
        b.SetValue<TUserInfo::Yuid>("event/payload/yandexuid");
        b.Custom<NCuttlefish::NProtoConverters::TAuthTokenHandler>("event/payload/oauth_token");
        b.SetValue<TUserInfo::TvmServiceTicket, TStrictConvert>("event/payload/serviceticket");
        b.SetValue<TUserInfo::Cookie, TStrictConvert>("event/payload/cookie");
        b.SetValue<TUserInfo::CsrfToken, TStrictConvert>("event/payload/csrf_token");
        b.SetValue<TUserInfo::VinsApplicationUuid, TStrictConvert>("event/payload/vins/application/uuid");
    } {
        auto b = conv.Build().Sub<TSynchronizeStateEvent::UserOptions>();
        b.SetValue<TUserOptions::SaveToMds>("event/payload/save_to_mds");
        b.SetValue<TUserOptions::DisableLocalExperiments>("event/payload/disable_local_experiments");
        b.SetValue<TUserOptions::DisableUtteranceLogging>("event/payload/disable_utterance_logging");
        b.SetValue<TUserOptions::AcceptInvalidAuth>("event/payload/accept_invalid_auth");
    }{
        auto b = conv.Build().Sub<TSynchronizeStateEvent::DeviceInfo>();
        b.Custom<TNormalizedDeviceField<TDeviceInfo::DeviceManufacturer>>("event/payload/vins/application/device_manufacturer");
        b.Custom<TNormalizedDeviceField<TDeviceInfo::DeviceModel>>("event/payload/vins/application/device_model");
        b.SetValue<TDeviceInfo::DeviceModification>("event/payload/vins/application/device_revision");
        b.SetValue<TDeviceInfo::Device>("event/payload/device");
        b.SetValue<TDeviceInfo::DeviceId>("event/payload/vins/application/device_id");
        b.SetValue<TDeviceInfo::Platform>("event/payload/vins/application/platform");
        b.SetValue<TDeviceInfo::OsVersion>("event/payload/vins/application/os_version");
        b.SetValue<TDeviceInfo::DeviceColor>("event/payload/vins/application/device_color");
        b.SetValue<TDeviceInfo::NetworkType>("event/payload/network_type");
        b.ForEachInArray().AddNew<TDeviceInfo::WifiNetworks>().Parse<WifiNetworkConverter>("event/payload/wifi_networks");
        b.ForEachInArray().Append<TDeviceInfo::SupportedFeatures>("event/payload/supported_features");
    } {
        auto b = conv.Build().Sub<TSynchronizeStateEvent::ApplicationInfo>();
        b.Field<TApplicationInfo::Id>()
            .From("event/payload/app_id")
            .SpareFrom("event/payload/vins/application/app_id");
        b.Field<TApplicationInfo::Version>()
            .From("event/payload/app_version")
            .SpareFrom("event/payload/vins/application/app_version");
        b.SetValue<TApplicationInfo::SpeechkitVersion>("event/payload/speechkitVersion");
        b.SetValue<TApplicationInfo::Lang>("event/payload/vins/application/lang");
    } {
        auto b = conv.Build().Sub<TSynchronizeStateEvent::AudioOptions>();
        b.SetValue<TAudioOptions::Format>("event/payload/format");
    } {
        auto b = conv.Build().Sub<TSynchronizeStateEvent::VoiceOptions>();
        b.SetValue<TVoiceOptions::Volume>("event/payload/volume");
        b.SetValue<TVoiceOptions::Speed>("event/payload/speed");
        b.SetValue<TVoiceOptions::Lang>("event/payload/lang");
        b.SetValue<TVoiceOptions::Voice>("event/payload/voice");
        b.SetValue<TVoiceOptions::UnrestrictedEmotion>("event/payload/emotion");
        b.Custom<TVoiceQuality>("event/payload/quality");
    } {
        auto b = conv.Build().Sub<TSynchronizeStateEvent::BiometryOptions>();
        b.SetValue<TBiometryOptions::Classify>("event/payload/biometry_classify");
        b.SetValue<TBiometryOptions::Score>("event/payload/biometry_score");
        b.SetValue<TBiometryOptions::Group>("event/payload/biometry_group");
        b.SetValue<TBiometryOptions::SendScoreToMM>("event/payload/vins_scoring");
    } {
        conv.Build().Custom<TExperimentsHandler>("event/payload/request/experiments");
    }
    return conv;
}

static const TConverter<NAliceProtocol::TSynchronizeStateEvent> SynchronizeStateEventConverter = CreateSynchronizeStateEventConverter();


// ------------------------------------------------------------------------------------------------
struct TEventExceptionDefault {
    static inline void Parse(const NJson::TJsonValue&, NAliceProtocol::TEventException&) { }

    template <typename WriterT>
    static inline void Serialize(WriterT&& writer, const NAliceProtocol::TEventException&) {
        writer.Value(TStringBuf("Error"));
    }
};

TConverter<NAliceProtocol::TEventException> CreateEventExceptionConverter()
{
    using namespace NProtoTraits::NAliceProtocol;

    TConverter<NAliceProtocol::TEventException> conv;
    auto b = conv.Build();
    b.SetValue<TEventException::Text>("error/message");
    b.Custom<TEventExceptionDefault>("error/type");
    return conv;
}

static const TConverter<NAliceProtocol::TEventException> EventExceptionConverter = CreateEventExceptionConverter();


// ------------------------------------------------------------------------------------------------
struct TDirectivePayloadHandlers {
    template <typename JsonNodeT>
    static inline void Parse(JsonNodeT&, NAliceProtocol::TDirective&) {
        // don't know how to parse
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& writer, const NAliceProtocol::TDirective& msg)
    {
        if (msg.HasException()) {
            EventExceptionConverter.Serialize(msg.GetException(), std::forward<WriterT>(writer));
            return;
        }
        if (msg.HasInvalidAuth()) {
            writer.Map();  // currently this directive has empty body
            return;
        }
        if (msg.HasSyncStateResponse()) {
            const auto val = msg.GetSyncStateResponse();
            auto map = writer.Map();
            map.Insert("SessionId", val.GetSessionId());
            if (val.HasGuid()) {
                map.Insert("guid", val.GetGuid());
            }
        }
    }
};

TConverter<NAliceProtocol::TDirective> CreateDirectiveConverter()
{
    using namespace NProtoTraits::NAliceProtocol;

    TConverter<NAliceProtocol::TDirective> conv;
    auto b = conv.Build();

    b.Sub<TDirective::Header>().Parse<GetEventHeaderConverter()>("directive/header");
    b.Custom<TDirectivePayloadHandlers>("directive/payload");
    return conv;
}

static const TConverter<NAliceProtocol::TDirective> DirectiveConverter = CreateDirectiveConverter();

}  // anonymous namespace

// ------------------------------------------------------------------------------------------------

const TConverter<NAliceProtocol::TSynchronizeStateEvent>& GetSynchronizeStateEventConverter() {
    return SynchronizeStateEventConverter;
}

const TConverter<NAliceProtocol::TEventException>& GetEventExceptionConverter() {
    return EventExceptionConverter;
}

const TConverter<NAliceProtocol::TDirective>& GetDirectiveConverter() {
    return DirectiveConverter;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices::NConverter


