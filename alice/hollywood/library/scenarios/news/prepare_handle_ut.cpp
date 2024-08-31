#include "alice/hollywood/library/framework/core/return_types.h"
#include "alice/hollywood/library/framework/core/scenario.h"
#include "alice/hollywood/library/framework/unittest/test_environment.h"
#include "alice/hollywood/library/framework/unittest/test_nodes.h"
#include "news_fast_data.h"
#include "prepare_handle.h"

#include <alice/hollywood/library/scenarios/news/nlg/register.h>

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood {

namespace {

const TString GET_NEWS_FRAME = "personal_assistant.scenarios.get_news";
const TString GET_FREE_NEWS_FRAME = "personal_assistant.scenarios.get_free_news";

const TString NEWS_IS_DEFAULT_REQUEST_SLOT = "is_default_request";
const TString NEWS_STATE_SLOT = "news";
const TString NEWS_MEMENTO_SLOT = "news_memento";
const TString NEWS_SMI_SLOT = "smi";
const TString NEWS_TOPIC_SLOT = "topic";
const TString NEWS_TOPIC_TYPE = "news_topic";
const TString NEWS_TOPIC_VALUE = "sport";
const TString NEWS_TOPIC_VALUE_SMI = "rbc";

const TString FREE_NEWS_TOPIC_TYPE = "string";
const TString FREE_NEWS_TOPIC_VALUE = "что творится в америке в связи с восстанием";

const TVector<TSmi> SMI_COLLECTION {
            {"РБК", "РБК", "rbc", "abcdef#rbc", "1", "rbc.ru", true, "rbk_logo.png"},
            {"Медуза", "Медузу", "meduza", "fedcba#medusa", "2", "meduza.ru", true, "meduza_logo.png"}};

static const TString EPOCH = "1587888637";
static const TString TIMEZONE = "UTC+3";
static const TString UUID = "094FC009-6BF1-4F18-86FD-DA31731187F6";

NScenarios::TScenarioRunRequest CreateRequest() {
    NScenarios::TScenarioRunRequest request;

    auto& clientInfoProto = *request.MutableBaseRequest()->MutableClientInfo();
    clientInfoProto.SetEpoch(EPOCH);
    clientInfoProto.SetTimezone(TIMEZONE);
    clientInfoProto.SetUuid(UUID);

    return request;
}

TSemanticFrame CreateGetNewsTopicFrame(TString type, TString value) {
    TSemanticFrame frame;
    frame.SetName(GET_NEWS_FRAME);

    auto& slot = *frame.AddSlots();
    slot.SetName(NEWS_TOPIC_SLOT);
    slot.MutableTypedValue()->SetType(type);
    slot.MutableTypedValue()->SetString(value);

    return frame;
}

TSemanticFrame CreateFreeNewsTopicFrame() {
    TSemanticFrame frame;
    frame.SetName(GET_FREE_NEWS_FRAME);

    auto& slot = *frame.AddSlots();
    slot.SetName(NEWS_TOPIC_SLOT);
    slot.MutableTypedValue()->SetType(FREE_NEWS_TOPIC_TYPE);
    slot.MutableTypedValue()->SetString(FREE_NEWS_TOPIC_VALUE);

    return frame;
}

void CheckBassRequest(const NAppHostHttp::THttpRequest& request) {
    const auto bassRequestRaw = NSc::TValue::FromJsonThrow(request.GetContent());
    const TRequestConstScheme bassRequest(&bassRequestRaw);

    UNIT_ASSERT_VALUES_EQUAL(GET_NEWS_FRAME, bassRequest.Form()->Name().Get());

    auto slots = bassRequest.Form()->Slots();
    UNIT_ASSERT_VALUES_EQUAL(4, slots.Size());
    auto slot = slots[0];
    UNIT_ASSERT_VALUES_EQUAL(NEWS_IS_DEFAULT_REQUEST_SLOT, slot.Name().Get());
    slot = slots[1];
    UNIT_ASSERT_VALUES_EQUAL(NEWS_STATE_SLOT, slot.Name().Get());
    slot = slots[2];
    UNIT_ASSERT_VALUES_EQUAL(NEWS_MEMENTO_SLOT, slot.Name().Get());
    slot = slots[3];
    UNIT_ASSERT_VALUES_EQUAL(NEWS_TOPIC_SLOT, slot.Name().Get());
    UNIT_ASSERT_VALUES_EQUAL(NEWS_TOPIC_VALUE, slot.Value().AsPrimitive<TStringBuf>().Get());
}

void CheckBassRequestSmi(const NAppHostHttp::THttpRequest& request) {
    const auto bassRequestRaw = NSc::TValue::FromJsonThrow(request.GetContent());
    const TRequestConstScheme bassRequest(&bassRequestRaw);

    UNIT_ASSERT_VALUES_EQUAL(GET_NEWS_FRAME, bassRequest.Form()->Name().Get());

    auto slots = bassRequest.Form()->Slots();
    UNIT_ASSERT_VALUES_EQUAL(5, slots.Size());

    bool hasState = false;
    bool hasMemento = false;
    bool hasTopic = false;
    bool hasSmi = false;
    bool hasIsDefaultRequest = false;

    NJson::TJsonValue smiSlot;

    for (int i = 0; i < 5; i++) {
        const auto& slot = slots[i];
        TStringBuf name = slot.Name().Get();
        if (name == NEWS_STATE_SLOT) {
            hasState = true;
        } else if (name == NEWS_MEMENTO_SLOT) {
            hasMemento = true;
        } else if (name == NEWS_TOPIC_SLOT) {
            hasTopic = true;
        } else if (name == NEWS_SMI_SLOT) {
            hasSmi = true;
            const TStringBuf value = slot.Value().AsPrimitive<TStringBuf>().Get();
            smiSlot = JsonFromString(value);
        } else if (name == NEWS_IS_DEFAULT_REQUEST_SLOT) {
            hasIsDefaultRequest = true;
        }
    }
    UNIT_ASSERT(hasState);
    UNIT_ASSERT(hasMemento);
    UNIT_ASSERT(hasTopic);
    UNIT_ASSERT(hasSmi);
    UNIT_ASSERT_VALUES_EQUAL(smiSlot["aid"], "1");
    UNIT_ASSERT(hasIsDefaultRequest);
}

} // namespace

Y_UNIT_TEST_SUITE(NewsPrepareHandleTests) {
    Y_UNIT_TEST(Smoke) {
        NHollywoodFw::TTestEnvironment testData("news", "ru-ru");
        testData.RunRequest = CreateRequest();
        *testData.RunRequest.MutableInput()->AddSemanticFrames() = CreateGetNewsTopicFrame(NEWS_TOPIC_TYPE, NEWS_TOPIC_VALUE);
        testData.AttachFastdata(std::shared_ptr<IFastData>(new TNewsFastData(SMI_COLLECTION)));

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper{testData.RunRequest, serviceCtx};

        TMockGlobalContext globalCtx;
        TContext ctx{globalCtx, TRTLogger::NullLogger(), /* scenarioResources= */ nullptr, /* scenarioNlg= */ nullptr};

        TRng rng{4};
        TRng rng2{4};
        TRng nlgRng{4};
        TCompiledNlgComponent nlgComponent = TCompiledNlgComponent(
            nlgRng, nullptr, NAlice::NHollywood::NLibrary::NScenarios::NNews::NNlg::RegisterAll);
        TNlgWrapper nlg = TNlgWrapper::Create(nlgComponent, wrapper, rng, ELanguage::LANG_RUS);
        const NJson::TJsonValue appHostParams;
        TRunResponseBuilder builder(&nlg);

        const auto bassRequest = NImpl::NewsPrepareDoImpl(ctx, nlg, builder, wrapper, testData.RequestMeta, appHostParams, rng2, testData.CreateRunRequest());
        UNIT_ASSERT(std::holds_alternative<THttpProxyRequest>(bassRequest));
        CheckBassRequest(std::get<THttpProxyRequest>(bassRequest).Request);
    }

    Y_UNIT_TEST(FreeNewsIrrelevant) {
        NHollywoodFw::TTestEnvironment testData("news", "ru-ru");
        testData.RunRequest = CreateRequest();
        *testData.RunRequest.MutableInput()->AddSemanticFrames() = CreateFreeNewsTopicFrame();
        testData.AttachFastdata(std::shared_ptr<IFastData>(new TNewsFastData(SMI_COLLECTION)));

        NScenarios::TRequestMeta meta;
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper{testData.RunRequest, serviceCtx};

        TMockGlobalContext globalCtx;
        TContext ctx{globalCtx, TRTLogger::NullLogger(), /* scenarioResources= */ nullptr, /* scenarioNlg= */ nullptr};

        TRng rng{4};
        TRng rng2{4};
        TRng nlgRng{4};
        TCompiledNlgComponent nlgComponent = TCompiledNlgComponent(
            nlgRng, nullptr, NAlice::NHollywood::NLibrary::NScenarios::NNews::NNlg::RegisterAll);
        TNlgWrapper nlg = TNlgWrapper::Create(nlgComponent, wrapper, rng, ELanguage::LANG_RUS);
        const NJson::TJsonValue appHostParams;
        TRunResponseBuilder builder(&nlg);

        const auto irrelResponse = NImpl::NewsPrepareDoImpl(ctx, nlg, builder, wrapper, testData.RequestMeta, appHostParams, rng2, testData.CreateRunRequest());
        UNIT_ASSERT(std::holds_alternative<NScenarios::TScenarioRunResponse>(irrelResponse));
        const auto& response = std::get<NScenarios::TScenarioRunResponse>(irrelResponse);

        UNIT_ASSERT(response.GetFeatures().GetIsIrrelevant());
        auto& card = response.GetResponseBody().GetLayout().GetCards(0);
        UNIT_ASSERT(card.GetText().size() > 0);
    }

    Y_UNIT_TEST(Smi) {
        NHollywoodFw::TTestEnvironment testData("news", "ru-ru");
        testData.RunRequest = CreateRequest();
        testData.AttachFastdata(std::shared_ptr<IFastData>(new TNewsFastData(SMI_COLLECTION)));
        *testData.RunRequest.MutableInput()->AddSemanticFrames() = CreateGetNewsTopicFrame(NEWS_TOPIC_TYPE, NEWS_TOPIC_VALUE_SMI);

        NScenarios::TRequestMeta meta;
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper{testData.RunRequest, serviceCtx};

        TMockGlobalContext globalCtx;
        TContext ctx{globalCtx, TRTLogger::NullLogger(), /* scenarioResources= */ nullptr, /* scenarioNlg= */ nullptr};

        TRng rng{4};
        TRng rng2{4};
        TRng nlgRng{4};
        TCompiledNlgComponent nlgComponent = TCompiledNlgComponent(
            nlgRng, nullptr, NAlice::NHollywood::NLibrary::NScenarios::NNews::NNlg::RegisterAll);
        TNlgWrapper nlg = TNlgWrapper::Create(nlgComponent, wrapper, rng, ELanguage::LANG_RUS);
        const NJson::TJsonValue appHostParams;
        TRunResponseBuilder builder(&nlg);

        const auto bassRequest = NImpl::NewsPrepareDoImpl(ctx, nlg, builder, wrapper, testData.RequestMeta, appHostParams, rng2, testData.CreateRunRequest());
        UNIT_ASSERT(std::holds_alternative<THttpProxyRequest>(bassRequest));
        CheckBassRequestSmi(std::get<THttpProxyRequest>(bassRequest).Request);
    }
}

} // namespace NAlice::NHollywood
