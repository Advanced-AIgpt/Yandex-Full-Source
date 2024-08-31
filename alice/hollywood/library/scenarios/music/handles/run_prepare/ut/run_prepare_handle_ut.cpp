#include <alice/hollywood/library/scenarios/music/handles/run_prepare/impl.h>
#include "common.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/music/music_resources.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/nlg/register.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/library/unittest/mock_sensors.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace testing;

namespace NAlice::NHollywood::NMusic {

namespace {

const TString SEARCH_TEXT_KEY = "search_text";
const TString SEARCH_TEXT_VALUE = "hello";

static const TString EPOCH = "1574930264";
static const TString TIMEZONE = "UTC+3";
static const TString UUID = "34D94983-65BD-49F2-B34E-BD123A7A9830";

NScenarios::TScenarioRunRequest CreateRequest() {
    NScenarios::TScenarioRunRequest request;

    auto& clientInfoProto = *request.MutableBaseRequest()->MutableClientInfo();
    clientInfoProto.SetEpoch(EPOCH);
    clientInfoProto.SetTimezone(TIMEZONE);
    clientInfoProto.SetUuid(UUID);

    return request;
}

TSemanticFrame CreateMusicPlayFrame(const TString& frameName) {
    TSemanticFrame frame;
    frame.SetName(frameName);

    auto& slot = *frame.AddSlots();
    slot.SetName(SEARCH_TEXT_KEY);
    slot.MutableTypedValue()->SetType(SEARCH_TEXT_KEY);
    slot.MutableTypedValue()->SetString(SEARCH_TEXT_VALUE);

    return frame;
}

void CheckBassRequest(const NAppHostHttp::THttpRequest& request) {
    const auto bassRequestRaw = NSc::TValue::FromJsonThrow(request.GetContent());
    const TRequestConstScheme bassRequest(&bassRequestRaw);

    UNIT_ASSERT_VALUES_EQUAL(MUSIC_PLAY_FRAME, bassRequest.Form()->Name().Get());

    auto slots = bassRequest.Form()->Slots();
    UNIT_ASSERT_VALUES_EQUAL(1, slots.Size());

    auto slot = slots[0];
    UNIT_ASSERT_VALUES_EQUAL(SEARCH_TEXT_KEY, slot.Name().Get());
    UNIT_ASSERT_VALUES_EQUAL(SEARCH_TEXT_VALUE, slot.Value().AsPrimitive<TStringBuf>().Get());
}

} // namespace

Y_UNIT_TEST_SUITE(MusicPrepareHandleTests) {
    Y_UNIT_TEST(Smoke) {
        NScenarios::TScenarioRunRequest request = CreateRequest();
        *request.MutableInput()->AddSemanticFrames() = CreateMusicPlayFrame(MUSIC_PLAY_FRAME);

        NScenarios::TRequestMeta meta;
        NAppHost::NService::TTestContext serviceCtx;
        serviceCtx.AddProtobufItem(request, REQUEST_ITEM);
        TScenarioRunRequestWrapper wrapper{request, serviceCtx};

        TMockSensors sensors;
        TMockGlobalContext globalCtx;
        EXPECT_CALL(globalCtx, Sensors()).WillRepeatedly(testing::ReturnRef(sensors));
        EXPECT_CALL(sensors, IncRate(_)).WillRepeatedly(Return());

        TRng rng{4};
        TRng nlgRng{4};
        TCompiledNlgComponent nlgComponent = TCompiledNlgComponent(
            nlgRng, nullptr, NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll);
        TNlgWrapper nlg = TNlgWrapper::Create(nlgComponent, wrapper, rng, ELanguage::LANG_RUS);
        const NJson::TJsonValue appHostParams;

        TMusicResources musicResources;
        TContext ctx{globalCtx, TRTLogger::NullLogger(), &musicResources, &nlgComponent};

        TScenarioHandleContext scenarioHandleCtx{
            .ServiceCtx = serviceCtx,
            .RequestMeta = meta,
            .Ctx = ctx,
            .Rng = rng,
            .Lang = ELanguage::LANG_RUS,
            .UserLang = ELanguage::LANG_RUS,
            .AppHostParams = appHostParams,
        };
        const auto bassRequest = NImpl::TRunPrepareHandleImpl{scenarioHandleCtx}.Do();
        UNIT_ASSERT(std::holds_alternative<THttpProxyRequest>(bassRequest));
        CheckBassRequest(std::get<THttpProxyRequest>(bassRequest).Request);
    }

    Y_UNIT_TEST(EarlyOut) {
        NScenarios::TScenarioRunRequest request = CreateRequest();

        NScenarios::TRequestMeta meta;
        NAppHost::NService::TTestContext serviceCtx;
        serviceCtx.AddProtobufItem(request, REQUEST_ITEM);
        TScenarioRunRequestWrapper wrapper{request, serviceCtx};

        TFakeRng rng{TFakeRng::TIntegerTag{}, []() -> ui64 { return 0; }};
        TRng nlgRng{4};
        TCompiledNlgComponent nlgComponent = TCompiledNlgComponent(
            nlgRng, nullptr, NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll);
        TNlgWrapper nlg = TNlgWrapper::Create(nlgComponent, wrapper, rng, ELanguage::LANG_RUS);
        const NJson::TJsonValue appHostParams;

        TMusicResources musicResources;
        TMockGlobalContext globalCtx;
        TContext ctx{globalCtx, TRTLogger::NullLogger(), &musicResources, &nlgComponent};

        TScenarioHandleContext scenarioHandleCtx{
            .ServiceCtx = serviceCtx,
            .RequestMeta = meta,
            .Ctx = ctx,
            .Rng = rng,
            .Lang = ELanguage::LANG_RUS,
            .UserLang = ELanguage::LANG_RUS,
            .AppHostParams = appHostParams,
        };
        const auto response = NImpl::TRunPrepareHandleImpl{scenarioHandleCtx}.Do();
        UNIT_ASSERT(std::holds_alternative<NScenarios::TScenarioRunResponse>(response));
        UNIT_ASSERT_VALUES_EQUAL(
            "К сожалению, у меня нет такой музыки.",
            std::get<NScenarios::TScenarioRunResponse>(response).GetResponseBody().GetLayout().GetOutputSpeech());
    }
}

} // namespace NAlice::NHollywood::NMusic
