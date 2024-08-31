#include <alice/cuttlefish/library/cuttlefish/megamind/request/request_builder.h>

#include <alice/cuttlefish/library/cuttlefish/common/datasync.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/library/json/json.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/guest/guest_data.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include "hinter.h"

using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

static TSourceMetrics DEFAULT_METRICS("");
static const TLogContext DEFAULT_LOG_CONTEXT(MakeIntrusive<TSelfFlushLogFrame>(nullptr), nullptr);

class TMockSpeakerService : public IActiveSpeakerService {
public:
    TMockSpeakerService()
        : ActiveSpeakerContext(nullptr)
    {}

    TMockSpeakerService(TAtomicSharedPtr<TSpeakerContext> activeSpeakerContext)
        : ActiveSpeakerContext(activeSpeakerContext)
    {}

    TAtomicSharedPtr<TSpeakerContext> GetActiveSpeaker() const override {
        return ActiveSpeakerContext;
    }

private:
    TAtomicSharedPtr<TSpeakerContext> ActiveSpeakerContext;
};

const ERequestPhase DEFAULT_REQUEST_PHASE = ERequestPhase::RUN;
const TMaybe<NJson::TJsonValue> DEFAULT_APP_HOST_PARAMS;
const NAliceProtocol::TSessionContext DEFAULT_SESSION_CONTEXT;
const NAliceProtocol::TRequestContext DEFAULT_REQUEST_CONTEXT;
const NAliceProtocol::TContextLoadResponse DEFAULT_CONTEXT_LOAD_RESPONSE;
const NAlice::TSpeechKitRequestProto DEFAULT_REQUEST;
const NAlice::TLoggerOptions DEFAULT_ALICE_LOGGER_OPTIONS;
const TMockSpeakerService MOCK_SPEAKER_SERVICE;

} // namespace

Y_UNIT_TEST_SUITE(BalancingHints) {
    Y_UNIT_TEST(Hinter) {
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("sas", "prod").GetBalancingHint("badmode"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("sas", "badctype").GetBalancingHint("prod"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("badgeo", "prod").GetBalancingHint("prod"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("vla", "prod").GetBalancingHint("prod"), "vla");
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("man", "prod").GetBalancingHint("pre_prod"), "man");
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("sas", "prestable").GetBalancingHint("prod"), Nothing());
        UNIT_ASSERT_VALUES_EQUAL(TBalancingHintHolder("sas", "prestable").GetBalancingHint("pre_prod"), "sas-pre");
        TString oldGeo = GetEnv("UNIPROXY_CUSTOM_GEO");
        TString oldCtype = GetEnv("a_ctype");
        SetEnv("UNIPROXY_CUSTOM_GEO", "sas");
        SetEnv("a_ctype", "prod");
        TStringBuilder builder;
        TBalancingHintHolder::AddBalancingHint("prod", builder);
        UNIT_ASSERT_VALUES_EQUAL(builder, "X-Yandex-Balancing-Hint: sas\r\n");
        SetEnv("UNIPROXY_CUSTOM_GEO", oldGeo);
        SetEnv("a_ctype", oldCtype);
    }
}

Y_UNIT_TEST_SUITE(RequestBuilderVinsUrl) {
    Y_UNIT_TEST(DefaultRunAndApply) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://vins.alice.yandex.net/speechkit/app/pa/"},
            {ERequestPhase::APPLY, "full://vins.alice.yandex.net/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                DEFAULT_REQUEST_CONTEXT,
                DEFAULT_CONTEXT_LOAD_RESPONSE,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(Srcrwr) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS"] = "http://abc.com/speechkit/app/pa/"; // imitates "?srcrwr=VINS:http://abc.com/speechkit/app/pa/"

        TMegamindRequestBuilder builder(
            DEFAULT_REQUEST_PHASE,
            config,
            appHostParams,
            DEFAULT_SESSION_CONTEXT,
            DEFAULT_REQUEST_CONTEXT,
            DEFAULT_CONTEXT_LOAD_RESPONSE,
            MOCK_SPEAKER_SERVICE,
            DEFAULT_ALICE_LOGGER_OPTIONS,
            DEFAULT_LOG_CONTEXT
        );
        TRTLogActivation rtLogChild;
        NJson::TJsonValue sessionLog;
        UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, "full://abc.com/speechkit/app/pa/");
    }

    Y_UNIT_TEST(QuasarViaPayloadRunAndApply) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://vins.alice.yandex.net/speechkit/quasar");

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://vins.alice.yandex.net/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://vins.alice.yandex.net/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                DEFAULT_CONTEXT_LOAD_RESPONSE,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(UaasVinsUrlWithQueryRunAndApply) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/?a=b");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://apphost.vins.alice.yandex.net/speechkit/app/pa/?a=b"},
            {ERequestPhase::APPLY, "full://apphost.vins.alice.yandex.net/speechkit/apply/?a=b"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(UaasVinsUrlStupidPathRunAndApply) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetString("1");

        NAliceProtocol::TRequestContext reqCtx;

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://apphost.vins.alice.yandex.net/apphost/speechkit/app/pa/"},
            {ERequestPhase::APPLY, "full://apphost.vins.alice.yandex.net/apphost/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(VinsUrlStupidPathRunAndApply) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NAliceProtocol::TRequestContext reqCtx;

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://apphost.vins.alice.yandex.net/apphost/speechkit/app/pa/"},
            {ERequestPhase::APPLY, "full://apphost.vins.alice.yandex.net/apphost/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(VinsUrlAndUaasUrl1) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NAliceProtocol::TRequestContext reqCtx;
        for (const auto& vinsUrl : {"http://vins.alice.yandex.net/speechkit/quasar", "http://vins-int.voicetech.yandex.net/speechkit/quasar"}) {
            reqCtx.SetVinsUrl(vinsUrl);

            for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
                {ERequestPhase::RUN, "full://apphost.vins.alice.yandex.net/apphost/speechkit/quasar"},
                {ERequestPhase::APPLY, "full://apphost.vins.alice.yandex.net/apphost/speechkit/apply/"},
            }) {
                TMegamindRequestBuilder builder(
                    phase,
                    config,
                    DEFAULT_APP_HOST_PARAMS,
                    DEFAULT_SESSION_CONTEXT,
                    reqCtx,
                    contextLoadResponse,
                    MOCK_SPEAKER_SERVICE,
                    DEFAULT_ALICE_LOGGER_OPTIONS,
                    DEFAULT_LOG_CONTEXT
                );
                TRTLogActivation rtLogChild;
                NJson::TJsonValue sessionLog;
                UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
            }
        }
    }

    Y_UNIT_TEST(VinsUrlAndUaasUrl2) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://megamind-rc.alice.yandex.net/speechkit/quasar");

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://megamind-rc.alice.yandex.net/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://megamind-rc.alice.yandex.net/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(VinsUrlAndUaasUrl3) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://foobar.alice.yandex.net/speechkit/quasar");

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://foobar.alice.yandex.net/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://foobar.alice.yandex.net/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(SimpleCustomHost) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://custom_host/speechkit/app/pa/"},
            {ERequestPhase::APPLY, "full://custom_host/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                DEFAULT_REQUEST_CONTEXT,
                DEFAULT_CONTEXT_LOAD_RESPONSE,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndPayloadUrl) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://vins.alice.yandex.net/speechkit/quasar");

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://custom_host/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://custom_host/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                DEFAULT_CONTEXT_LOAD_RESPONSE,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl1) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/?a=b");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://apphost.vins.alice.yandex.net/speechkit/app/pa/?a=b"},
            {ERequestPhase::APPLY, "full://apphost.vins.alice.yandex.net/speechkit/apply/?a=b"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl2) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://vins.alice.yandex.net/?a=b");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://custom_host/speechkit/app/pa/?a=b"},
            {ERequestPhase::APPLY, "full://custom_host/speechkit/apply/?a=b"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl3) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://apphost.vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://apphost.vins.alice.yandex.net/apphost/speechkit/app/pa/"},
            {ERequestPhase::APPLY, "full://apphost.vins.alice.yandex.net/apphost/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl4) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://custom_host/apphost/speechkit/app/pa/"},
            {ERequestPhase::APPLY, "full://custom_host/apphost/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl5) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://vins.alice.yandex.net/speechkit/quasar");

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://custom_host/apphost/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://custom_host/apphost/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl6) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://vins-int.voicetech.yandex.net/speechkit/quasar");

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://custom_host/apphost/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://custom_host/apphost/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl7) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://megamind-rc.alice.yandex.net/speechkit/quasar");

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://megamind-rc.alice.yandex.net/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://megamind-rc.alice.yandex.net/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            UNIT_ASSERT_VALUES_EQUAL(builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false).Addr, url);
        }
    }

    Y_UNIT_TEST(CustomHostAndUaasUrl8) {
        NAliceCuttlefishConfig::TConfig config = GetDefaultCuttlefishConfig();

        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.SetVinsUrl("http://foobar.alice.yandex.net/speechkit/quasar");

        NAliceProtocol::TContextLoadResponse contextLoadResponse;
        const TString flag = "UaasVinsUrl_" + Base64Encode("http://vins.alice.yandex.net/apphost");
        (*contextLoadResponse.MutableFlagsInfo()->MutableVoiceFlags()->MutableStorage())[flag].SetInteger(1);

        NJson::TJsonValue appHostParams;
        appHostParams["srcrwr"]["VINS_HOST"] = "custom_host"; // imitates "?srcrwr=VINS_HOST:custom_host"

        NAlice::TLoggerOptions aliceLoggerOptions;
        aliceLoggerOptions.SetSetraceLogLevel(NAlice::ELogLevel::ELL_INFO);

        for (const auto& [phase, url] : TVector<std::pair<ERequestPhase, TStringBuf>>{
            {ERequestPhase::RUN, "full://foobar.alice.yandex.net/speechkit/quasar"},
            {ERequestPhase::APPLY, "full://foobar.alice.yandex.net/speechkit/apply/"},
        }) {
            TMegamindRequestBuilder builder(
                phase,
                config,
                appHostParams,
                DEFAULT_SESSION_CONTEXT,
                reqCtx,
                contextLoadResponse,
                MOCK_SPEAKER_SERVICE,
                aliceLoggerOptions,
                DEFAULT_LOG_CONTEXT
            );
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            const NNeh::TMessage req = builder.Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false);
            UNIT_ASSERT_VALUES_EQUAL(req.Addr, url);
            UNIT_ASSERT_VALUES_UNEQUAL(req.Data.find("X-Alice-Logger-Options: CBQ=\r\n"), TString::npos);
        }
    }
    Y_UNIT_TEST(ContactsJsonParser) {
        TString json = R"___({
  "contacts": {
    "data": {
      "contacts": [
        {
          "_id": 1,
          "account_name": "+78005555555",
          "account_type": "com.viber.voip",
          "contact_id": 123,
          "display_name": "Jose Martinez",
          "first_name": "Jose",
          "last_time_contacted": 0,
          "lookup_key": "key",
          "lookup_key_index": 0,
          "middle_name": "",
          "second_name": "Martinez",
          "times_contacted": 0
        }
      ],
      "is_known_uuid": true,
      "phones": [
        {
          "_id": 1,
          "account_type": "com.google",
          "lookup_key": "key",
          "lookup_key_index": 0,
          "phone": "8 800 555-55-55",
          "type": "mobile"
        }
      ],
      "lookup_key_map_serialized": "SDRzSUFBQUFBQUFBQS1OaTVXTE9UcTBFQUhmVUJVUUhBQUFB"
    },
    "status": "ok"
  }
})___";
        NAppHostHttp::THttpResponse http;
        http.SetStatusCode(200);
        http.SetContent(json);
        NAlice::TSpeechKitRequestProto::TContacts cts;
        UNIT_ASSERT_NO_EXCEPTION(ParseContactsResponseJson(http, cts, DEFAULT_LOG_CONTEXT));
    }
}

Y_UNIT_TEST_SUITE(GuestContext) {
    static const TVector<ERequestPhase> PHASES = { ERequestPhase::RUN, ERequestPhase::APPLY };
    static const TString PERS_ID = "TEST_PERS_ID";
    static const TString OAUTH_TOKEN = "TEST_OAUTH_TOKEN";
    static const TString GUEST_YUID = "TEST_GUEST_YUID";

    static const TString DATASYNC_HTTP_RESPONSE_CONTENT = NAlice::JsonToString(TDatasyncClient::MakeVinsContextsResponseContent(
        R"({"items":[{"last_used":"2016-04-07T23:22:48.010000+00:00","address_id":"home","tags":[],"title":"Home","modified":"2016-04-07T23:22:48.010000+00:00","longitude":12.345,"created":"2016-04-07T23:22:48.010000+00:00","mined_attributes":[],"address_line_short":"Bakerstreet,221b","latitude":23.456,"address_line":"London,Bakerstreet,221b"},{"last_used":"2016-04-07T23:22:48.011000+00:00","address_id":"work","tags":[],"title":"Work","modified":"2016-04-07T23:22:48.011000+00:00","longitude":34.567,"created":"2016-04-07T23:22:48.011000+00:00","mined_attributes":[],"address_line_short":"Tolstogostreet,16","latitude":45.678,"address_line":"Moscow,Tolstogostreet,16"}],"total":2,"limit":20,"offset":0})",
        R"({"items":[{"id":"AutomotivePromoCounters","value":{"auto_music_promo_2020":3}},{"id":"alice_proactivity","value":"enabled"},{"id":"gender","value":"male"},{"id":"guest_uid","value":"1234567"},{"id":"morning_show","value":{"last_push_timestamp":1633374073,"pushes_sent":2}},{"id":"proactivity_history","value":{}},{"id":"user_name","value":"Swarley"},{"id":"video_rater_proactivity_history","value":{"LastShowTime":"1593005794"}},{"id":"yandexstation_123456_location","value":{"location":"Moscow"}}]})",
        R"({"items":[{"do_not_use_user_logs":true}]})"
    ));

    static const TString CORRUPTED_DATASYNC_HTTP_RESPONSE_CONTENT = NAlice::JsonToString(TDatasyncClient::MakeVinsContextsResponseContent(
            R"({"items1":[{"last_used":"2016-04-07T23:22:48.010000+00:00","address_id":"home","tags":[],"title":"Home","modified":"2016-04-07T23:22:48.010000+00:00","longitude":12.345,"created":"2016-04-07T23:22:48.010000+00:00","mined_attributes":[],"address_line_short":"Bakerstreet,221b","latitude":23.456,"address_line":"London,Bakerstreet,221b"},{"last_used":"2016-04-07T23:22:48.011000+00:00","address_id":"work","tags":[],"title":"Work","modified":"2016-04-07T23:22:48.011000+00:00","longitude":34.567,"created":"2016-04-07T23:22:48.011000+00:00","mined_attributes":[],"address_line_short":"Tolstogostreet,16","latitude":45.678,"address_line":"Moscow,Tolstogostreet,16"}],"total":2,"limit":20,"offset":0})",
            R"({"items":2}")",
            R"({"3":})"
    ));

    static const TString BLACKBOX_HTTP_RESPONSE_CONTENT = R"(
        {
           "dbfields" : {
              "userinfo.lastname.uid" : "TEST_LAST_NAME",
              "userinfo.firstname.uid" : "TEST_FIRST_NAME"
           },
           "uid" : {
              "value" : "TEST_GUEST_YUID"
           },
           "user_ticket" : "mock_user_ticket",
           "phones" : [
              {
                 "id" : "111",
                 "attributes" : {
                    "108" : "1",
                    "102" : "TEST_PHONE",
                    "107" : "default_phone"
                 }
              }
           ],
           "attributes" : {
              "1015" : "1",
              "8": "1"
           },
           "address-list" : [
              {
                 "address" : "TEST_EMAIL"
              }
           ],
           "aliases" : {
              "13" : "root"
           },
           "billing_features" : {
              "basic-kinopoisk" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-plus" : {
                 "in_trial": true,
                 "region": 225
              },
              "basic-music" : {
                 "in_trial": true,
                 "region": 225
              }
           }
        })";

    static const TString CORRUPTED_BLACKBOX_HTTP_RESPONSE_CONTENT = R"(
        {
           "I don't know, how JSON looks like"
        })";

    const static TString RAW_PERSONAL_DATA = R"({"/v1/personality/profile/alisa/kv/AutomotivePromoCounters":{"auto_music_promo_2020":3},"/v1/personality/profile/alisa/kv/alice_proactivity":"enabled","/v1/personality/profile/alisa/kv/gender":"male","/v1/personality/profile/alisa/kv/guest_uid":"1234567","/v1/personality/profile/alisa/kv/morning_show":{"last_push_timestamp":1633374073,"pushes_sent":2},"/v1/personality/profile/alisa/kv/proactivity_history":{},"/v1/personality/profile/alisa/kv/user_name":"Swarley","/v1/personality/profile/alisa/kv/video_rater_proactivity_history":{"LastShowTime":"1593005794"},"/v1/personality/profile/alisa/kv/yandexstation_123456_location":{"location":"Moscow"},"/v2/personality/profile/addresses/home":{"address_id":"home","address_line":"London,Bakerstreet,221b","address_line_short":"Bakerstreet,221b","created":"2016-04-07T23:22:48.010000+00:00","last_used":"2016-04-07T23:22:48.010000+00:00","latitude":23.456,"longitude":12.345,"mined_attributes":[],"modified":"2016-04-07T23:22:48.010000+00:00","tags":[],"title":"Home"},"/v2/personality/profile/addresses/work":{"address_id":"work","address_line":"Moscow,Tolstogostreet,16","address_line_short":"Tolstogostreet,16","created":"2016-04-07T23:22:48.011000+00:00","last_used":"2016-04-07T23:22:48.011000+00:00","latitude":45.678,"longitude":34.567,"mined_attributes":[],"modified":"2016-04-07T23:22:48.011000+00:00","tags":[],"title":"Work"}})";

    class TTestRequestBuilder : public TMegamindRequestBuilder {
    public:

        TTestRequestBuilder(
            const NAliceCuttlefishConfig::TConfig& config,
            ERequestPhase phase,
            const IActiveSpeakerService& speakerService
        )
            : TMegamindRequestBuilder(
                phase,
                config,
                DEFAULT_APP_HOST_PARAMS,
                DEFAULT_SESSION_CONTEXT,
                DEFAULT_REQUEST_CONTEXT,
                DEFAULT_CONTEXT_LOAD_RESPONSE,
                speakerService,
                DEFAULT_ALICE_LOGGER_OPTIONS,
                DEFAULT_LOG_CONTEXT)
        {
        }

        NAlice::TSpeechKitRequestProto GetResultSpeechKitRequest() {
            TRTLogActivation rtLogChild;
            NJson::TJsonValue sessionLog;
            Build(DEFAULT_REQUEST, rtLogChild, sessionLog, /* isFinal */ false);

            return NAlice::JsonToProto<NAlice::TSpeechKitRequestProto>(sessionLog["Body"]);
        }
    };

    class TTestFixture {
    public:
        TTestFixture(ERequestPhase phase, TAtomicSharedPtr<TSpeakerContext> speakerContext)
            : Config(GetDefaultCuttlefishConfig())
            , SpeakerService(speakerContext)
            , Builder(Config, phase, SpeakerService)
        {}

    public:
        NAliceCuttlefishConfig::TConfig Config;
        TMockSpeakerService SpeakerService;
        TTestRequestBuilder Builder;
    };

    Y_UNIT_TEST(WhenEmpty) {
        for (const auto phase : PHASES) {
            TTestFixture fixture(phase, nullptr);

            auto result = fixture.Builder.GetResultSpeechKitRequest();
Y_UNUSED(result);
            UNIT_ASSERT(!result.GetRequest().GetAdditionalOptions().HasGuestUserOptions());
            UNIT_ASSERT(!result.HasGuestUserData());
        }
    }

    Y_UNIT_TEST(WhenHasOnlyGuestOptions) {
        for (const auto phase : PHASES) {
            auto speakerContext = MakeAtomicShared<TSpeakerContext>();
            speakerContext->GuestUserOptions.SetPersId("PERS_ID");
            speakerContext->GuestUserOptions.SetOAuthToken("OAUTH_TIOKEN");
            speakerContext->GuestUserOptions.SetYandexUID("YANDEX_UID");
            speakerContext->GuestUserOptions.SetStatus(::NAlice::TGuestOptions::Match);
            speakerContext->GuestUserOptions.SetGuestOrigin(::NAlice::TGuestOptions::VoiceBiometry);

            TTestFixture fixture(phase, speakerContext);

            NAlice::TGuestOptions result = fixture.Builder
                .GetResultSpeechKitRequest()
                .GetRequest()
                .GetAdditionalOptions()
                .GetGuestUserOptions();

            UNIT_ASSERT_STRINGS_EQUAL(result.GetPersId(), "PERS_ID");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetOAuthToken(), "OAUTH_TIOKEN");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetYandexUID(), "YANDEX_UID");
            UNIT_ASSERT_EQUAL(result.GetStatus(), ::NAlice::TGuestOptions::Match);
            UNIT_ASSERT_EQUAL(result.GetGuestOrigin(), ::NAlice::TGuestOptions::VoiceBiometry);
        }
    }

    Y_UNIT_TEST(WhenHasOnlyGuestData) {
        for (const auto phase : PHASES) {
            auto speakerContext = MakeAtomicShared<TSpeakerContext>();
            speakerContext->GuestUserData.SetRawPersonalData("hello");
            speakerContext->GuestUserData.MutableUserInfo()->SetUid("uid_123");
            speakerContext->GuestUserData.MutableUserInfo()->SetEmail("root@yandex.ru");
            speakerContext->GuestUserData.MutableUserInfo()->SetFirstName("firstname");
            speakerContext->GuestUserData.MutableUserInfo()->SetLastName("lastname");
            speakerContext->GuestUserData.MutableUserInfo()->SetPhone("main_phone");
            speakerContext->GuestUserData.MutableUserInfo()->SetHasYandexPlus(true);
            speakerContext->GuestUserData.MutableUserInfo()->SetIsStaff(true);
            speakerContext->GuestUserData.MutableUserInfo()->SetIsBetaTester(true);
            speakerContext->GuestUserData.MutableUserInfo()->SetHasMusicSubscription(true);
            speakerContext->GuestUserData.MutableUserInfo()->SetMusicSubscriptionRegionId(225);

            TTestFixture fixture(phase, speakerContext);

            NAlice::TGuestData result = fixture.Builder
                .GetResultSpeechKitRequest()
                .GetGuestUserData();

            UNIT_ASSERT_STRINGS_EQUAL(result.GetRawPersonalData(), "hello");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetUserInfo().GetUid(), "uid_123");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetUserInfo().GetEmail(), "root@yandex.ru");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetUserInfo().GetFirstName(), "firstname");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetUserInfo().GetLastName(), "lastname");
            UNIT_ASSERT_STRINGS_EQUAL(result.GetUserInfo().GetPhone(), "main_phone");
            UNIT_ASSERT_VALUES_EQUAL(result.GetUserInfo().GetHasYandexPlus(), true);
            UNIT_ASSERT_VALUES_EQUAL(result.GetUserInfo().GetIsStaff(), true);
            UNIT_ASSERT_VALUES_EQUAL(result.GetUserInfo().GetIsBetaTester(), true);
            UNIT_ASSERT_VALUES_EQUAL(result.GetUserInfo().GetHasMusicSubscription(), true);
            UNIT_ASSERT_VALUES_EQUAL(result.GetUserInfo().GetMusicSubscriptionRegionId(), 225);
        }
    }
}
