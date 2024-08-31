#include "request.h"

#include <alice/hollywood/library/request/ut/proto/test_states.pb.h>

#include <alice/library/scenarios/data_sources/data_sources.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood {

namespace {

Y_UNIT_TEST_SUITE(TRequestSuite) {
    Y_UNIT_TEST(TestStateLoad) {
        const TString info = "123";
        NAppHost::NService::TTestContext ctx;

        {
            NScenarios::TScenarioRunRequest requestProto;
            TScenarioRunRequestWrapper request(requestProto, ctx);
            const auto state = request.LoadState<TStateWithString>();

            UNIT_ASSERT(!state.GetInfo());
        }
        {
            TStateWithString initState;
            initState.SetInfo(info);

            NScenarios::TScenarioRunRequest requestProto;
            requestProto.MutableBaseRequest()->MutableState()->PackFrom(initState);

            TScenarioRunRequestWrapper request(requestProto, ctx);
            const auto state = request.LoadState<TStateWithString>();

            UNIT_ASSERT_VALUES_EQUAL(state.GetInfo(), info);
        }
        {
            TStateWithString initState;
            initState.SetInfo(info);

            NScenarios::TScenarioRunRequest requestProto;
            requestProto.MutableBaseRequest()->MutableState()->PackFrom(initState);

            TScenarioRunRequestWrapper request(requestProto, ctx);
            const auto state = request.LoadState<TStateWithInt>();

            UNIT_ASSERT(!state.GetValue());
        }
    }

    Y_UNIT_TEST(TestPersonalData) {
        NScenarios::TScenarioBaseRequest requestProto;
        auto& options = *requestProto.MutableOptions();
        options.SetRawPersonalData(R"-({
            "key1": "foo",
            "key2": {"key3": "bar"}
        })-");
        NAppHost::NService::TTestContext ctx;
        TScenarioBaseRequestWrapper request(requestProto, ctx);
        UNIT_ASSERT(request.HasPersonalData("key1"));
        UNIT_ASSERT(request.HasPersonalData("key2"));
        UNIT_ASSERT(!request.HasPersonalData("key3"));

        const auto* foo = request.GetPersonalDataString("key1");
        UNIT_ASSERT_STRINGS_EQUAL(*foo, "foo");

        const auto* key2Val = request.GetPersonalDataValue("key2");
        NJson::TJsonValue key3Bar;
        key3Bar["key3"] = "bar";
        UNIT_ASSERT_VALUES_EQUAL(*key2Val, key3Bar);

        UNIT_ASSERT_EXCEPTION(request.GetPersonalDataString("key2"), yexception);

        const auto* key3Str = request.GetPersonalDataString("key3");
        UNIT_ASSERT_EQUAL(key3Str, nullptr);

        const auto* key3Val = request.GetPersonalDataValue("key3");
        UNIT_ASSERT_EQUAL(key3Val, nullptr);
    }

    Y_UNIT_TEST(TestPersonalDataEmpty) {
        NScenarios::TScenarioBaseRequest requestProto;
        NAppHost::NService::TTestContext ctx;
        TScenarioBaseRequestWrapper request(requestProto, ctx);
        UNIT_ASSERT(!request.HasPersonalData("key"));

        const auto* keyStr = request.GetPersonalDataString("key");
        UNIT_ASSERT_EQUAL(keyStr, nullptr);

        const auto* keyVal = request.GetPersonalDataValue("key");
        UNIT_ASSERT_EQUAL(keyVal, nullptr);
    }

    Y_UNIT_TEST(TsatDataSources) {
        const TString appInfoValue = "123";

        {
            NScenarios::TDataSource dataSource;
            dataSource.MutableAppInfo()->SetValue(appInfoValue);

            NAppHost::NService::TTestContext ctx;
            ctx.AddProtobufItem(
                dataSource,
                NScenarios::GetDataSourceContextName(EDataSourceType::APP_INFO),
                NAppHost::EContextItemKind::Input
            );

            NScenarios::TScenarioRunRequest requestProto;
            TScenarioRunRequestWrapper request(requestProto, ctx);
            const auto* dataSourcePtr = request.GetDataSource(EDataSourceType::APP_INFO);
            UNIT_ASSERT(dataSourcePtr != nullptr);
            UNIT_ASSERT(dataSourcePtr->GetAppInfo().GetValue() == appInfoValue);
        }
    }

    Y_UNIT_TEST(UnpackAndGetRef) {
        TComplexState args;
        args.SetFoo(22.8);
        args.SetBar(13071999);
        args.MutableInnerState()->SetMock("ingBird");

        NScenarios::TScenarioApplyRequest applyReq;
        applyReq.MutableArguments()->PackFrom(args);

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioApplyRequestWrapper wrapper(applyReq, serviceCtx);

        const TComplexState& argsRef = wrapper.UnpackArgumentsAndGetRef<TComplexState>();
        UNIT_ASSERT_VALUES_EQUAL(argsRef.GetFoo(), 22.8);
        UNIT_ASSERT_VALUES_EQUAL(argsRef.GetBar(), 13071999);
        UNIT_ASSERT_VALUES_EQUAL(argsRef.GetInnerState().GetMock(), "ingBird");
    }
}

} // namespace

} // namespace NAlice::NHollywood
