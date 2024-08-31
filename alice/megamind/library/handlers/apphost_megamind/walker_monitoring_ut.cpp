#include "walker_monitoring.h"
#include "ut_helper.h"

#include <alice/library/metrics/names.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/scenarios_text_response.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/iterator/cartesian_product.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace ::testing;
using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

auto FillTextResponse(const TVector<TString>& texts, const TString& scenarioName) {
    NMegamindAppHost::TScenarioTextResponse textResponse;
    textResponse.SetScenarioName(scenarioName);
    for (const auto& text : texts) {
        textResponse.AddText(text);
    }
    return textResponse;
}


auto FillScenarioResponse(const TVector<TString>& texts, bool useTextWithButtons = false) {
    NScenarios::TScenarioRunResponse response;
    auto& responseBody = *response.MutableResponseBody();
    auto& cards = *responseBody.MutableLayout()->MutableCards();
    for (const auto& text : texts) {
        auto& card = *cards.Add();
        if (useTextWithButtons) {
            auto& textWithButtons = *card.MutableTextWithButtons();
            textWithButtons.SetText(text);
        } else {
            card.SetText(text);
        }
    }
    return response;
}

auto FillScenarioHttpResponse(const TVector<TString>& texts, bool useTextWithButtons = false) {
    auto scenarioResponse = FillScenarioResponse(texts, useTextWithButtons);
    NAppHostHttp::THttpResponse response;
    response.SetContent(scenarioResponse.SerializeAsString());
    response.SetStatusCode(200);
    NAppHostHttp::THeader header;
    header.SetName(NNetwork::HEADER_CONTENT_TYPE);
    header.SetValue(TString{NContentTypes::APPLICATION_PROTOBUF});
    *response.AddHeaders() = header;
    return response;
}

void FillContext(
    NAppHost::IServiceContext& ctx,
    const TVector<TString>& texts,
    const TString& scenarioName,
    bool useHttp,
    bool textWithButtons)
{
    ctx.DeleteItems();
    if (useHttp) {
        auto name = TString::Join(SCENARIO_ITEM_PREFIX, scenarioName, SCENARIO_HTTP_ITEM_SUFFIX);
        ctx.AddProtobufItem(FillScenarioHttpResponse(texts, textWithButtons), name, NAppHost::EContextItemKind::Input);
        return;
    }
    auto name = TString::Join(SCENARIO_ITEM_PREFIX, scenarioName, SCENARIO_PURE_ITEM_SUFFIX);
    ctx.AddProtobufItem(FillScenarioResponse(texts, textWithButtons), name, NAppHost::EContextItemKind::Input);
}

Y_UNIT_TEST_SUITE(AppHostMegamindWalkerMonitoring) {
    Y_UNIT_TEST_F(NoErrors, TAppHostWalkerTestFixture) {
        TVector<TString> texts = {"cute cats"};
        TString scenarioName = {};

        auto checkSensors = [&]() {
            TMockSensors sensors;
            EXPECT_CALL(GlobalCtx, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));

            EXPECT_CALL(sensors, IncRate(_)).Times(0);

            TAppHostWalkerMonitoringNodeHandler handler{GlobalCtx};
            handler.RunSync(AhCtx.TestCtx());
        };
        {
            AhCtx.TestCtx().AddProtobufItem(FillTextResponse(texts, scenarioName), AH_ITEM_SCENARIOS_RESPONSE_MONITORING,
                                        NAppHost::EContextItemKind::Input);
            checkSensors();
        }

        TVector<bool> cases{false, true};
        for (auto [textWithButtons, useHttp] : CartesianProduct(cases, cases)) {
            FillContext(AhCtx.TestCtx(), texts, scenarioName, useHttp, textWithButtons);
            checkSensors();
        }
    }

    Y_UNIT_TEST_F(NlgErrorsOnly, TAppHostWalkerTestFixture) {
        TVector<TString> texts = {"Прошу прощения, что-то сломалось."};
        TString scenarioName = "cats";

        auto checkSensors = [&]() {
            TMockSensors sensors;
            EXPECT_CALL(GlobalCtx, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));

            NMonitoring::TLabels expectedLabels = {
                {NSignal::SCENARIO_NAME, scenarioName},
                {"error_type", "NLG_ERRORS"}
            };
            EXPECT_CALL(sensors, IncRate(std::move(expectedLabels))).Times(1);

            NMonitoring::TLabels expectedAllLabels = {
                {NSignal::SCENARIO_NAME, scenarioName},
                {"error_type", "NLG_ALL"}
            };
            EXPECT_CALL(sensors, IncRate(std::move(expectedAllLabels))).Times(1);

            TAppHostWalkerMonitoringNodeHandler handler{GlobalCtx};
            handler.RunSync(AhCtx.TestCtx());
        };

        {
            AhCtx.TestCtx().AddProtobufItem(FillTextResponse(texts, scenarioName), AH_ITEM_SCENARIOS_RESPONSE_MONITORING,
                                        NAppHost::EContextItemKind::Input);
            checkSensors();
        }

        TVector<bool> cases{false, true};
        for (auto [textWithButtons, useHttp] : CartesianProduct(cases, cases)) {
            FillContext(AhCtx.TestCtx(), texts, scenarioName, useHttp, textWithButtons);
            checkSensors();
        }
    }

    Y_UNIT_TEST_F(NlgNotFoundAndSorry, TAppHostWalkerTestFixture) {
        TVector<TString> texts = {"к сожалению не получилось ничего найти", "увы не получилось"};
        TString scenarioName = "cats";
        auto checkSensors = [&]() {
            TMockSensors sensors;
            EXPECT_CALL(GlobalCtx, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));

            NMonitoring::TLabels expectedNotFoundLabels = {
                {NSignal::SCENARIO_NAME, scenarioName},
                {"error_type", "NLG_NOT_FOUND"}
            };
            EXPECT_CALL(sensors, IncRate(std::move(expectedNotFoundLabels))).Times(1);

            NMonitoring::TLabels expectedSorryLabels = {
                {NSignal::SCENARIO_NAME, scenarioName},
                {"error_type", "NLG_SORRY"}
            };
            EXPECT_CALL(sensors, IncRate(std::move(expectedSorryLabels))).Times(1);

            NMonitoring::TLabels expectedAllLabels = {
                {NSignal::SCENARIO_NAME, scenarioName},
                {"error_type", "NLG_ALL"}
            };
            EXPECT_CALL(sensors, IncRate(std::move(expectedAllLabels))).Times(2);

            TAppHostWalkerMonitoringNodeHandler handler{GlobalCtx};
            handler.RunSync(AhCtx.TestCtx());
        };
        {
            AhCtx.TestCtx().AddProtobufItem(FillTextResponse(texts, scenarioName), AH_ITEM_SCENARIOS_RESPONSE_MONITORING,
                                    NAppHost::EContextItemKind::Input);
            checkSensors();
        }

        TVector<bool> cases{false, true};
        for (auto [textWithButtons, useHttp] : CartesianProduct(cases, cases)) {
            FillContext(AhCtx.TestCtx(), texts, scenarioName, useHttp, textWithButtons);
            checkSensors();
        }
    }
}

} // namespace
