#include "walker_monitoring.h"

#include <alice/library/metrics/names.h>

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/apphost_request/protos/scenarios_text_response.pb.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/generic/xrange.h>

namespace NAlice::NMegamind {

namespace {

// https://a.yandex-team.ru/arc/trunk/arcadia/quality/ab_testing/cofe/projects/alice/mappings.py?rev=r7961478#L184-222
const re2::RE2 NLG_ERRORS = {
    "Прошу прощения, что-то сломалось.|"
    "Произошла какая-то ошибка.|"
    "Извините, что-то пошло не так.|"
    "Даже идеальные помощники иногда ломаются.|"
    "Мне кажется, меня уронили.|"
    "О, кажется, мы с вами нашли во мне ошибку. Простите.|"
    "Мы меня сломали, но я обязательно починюсь.|"
    "что-то пошло не так"
};

const re2::RE2 NLG_NOT_FOUND = {
    "не нашла|"
    "не нашлось|"
    "не удалось найти|"
    "не смогла найти|"
    "ничего нет|"
    "у меня нет|"
    "не получилось ничего найти"
};

const re2::RE2 NLG_SORRY = {
    "увы|"
    "извините|"
    "к сожалению|"
    "боюсь|"
    "простите|"
    "прошу прощения"
};

const re2::RE2 NLG_CANT = {
    "не могу|"
    "не знаю|"
    "не умею|"
    "не поняла|"
    "не удалось"
};

const re2::RE2 NLG_SKILLS = {
    "Извините.*не отвечает"
};

} // namespace

// TAppHostWalkerMonitoringNodeHandler ----------------------------------------------
TAppHostWalkerMonitoringNodeHandler::TAppHostWalkerMonitoringNodeHandler(IGlobalCtx& globalCtx)
    : TAppHostNodeHandler(globalCtx, /* useAppHostStreaming= */ false)
{
}

TMaybe<TString> TAppHostWalkerMonitoringNodeHandler::TextHasError(const TString& text) {
    if (RE2::PartialMatch(text, NLG_ERRORS)) {
        return "NLG_ERRORS";
    }
    if (RE2::PartialMatch(text, NLG_NOT_FOUND)) {
        return "NLG_NOT_FOUND";
    }
    if (RE2::PartialMatch(text, NLG_SORRY)) {
        return "NLG_SORRY";
    }
    if (RE2::PartialMatch(text, NLG_CANT)) {
        return "NLG_CANT";
    }
    if (RE2::PartialMatch(text, NLG_SKILLS)) {
        return "NLG_SKILLS";
    }
    return Nothing();
}

void SendErrorSensor(NMetrics::ISensors& sensors, const TMaybe<TString>& errorType, const TString& scenarioName) {
    if (!errorType.Defined()) {
        return;
    }
    sensors.IncRate({
        {NSignal::SCENARIO_NAME, scenarioName},
        {"error_type", errorType.GetRef()}
    });
    sensors.IncRate({
        {NSignal::SCENARIO_NAME, scenarioName},
        {"error_type", "NLG_ALL"}
    });
}

void PushScenariosTexts(
    const NScenarios::TScenarioResponseBody& scenarioResponseBody,
    const TString& scenarioName,
    NMetrics::ISensors& sensors)
{
    NMegamindAppHost::TScenarioTextResponse textResponse;
    for (const auto& card : scenarioResponseBody.GetLayout().GetCards()) {
        switch (card.GetCardCase()) {
            case NScenarios::TLayout_TCard::CardCase::kText:
                SendErrorSensor(sensors, TAppHostWalkerMonitoringNodeHandler::TextHasError(card.GetText()), scenarioName);
                break;
            case NScenarios::TLayout_TCard::CardCase::kTextWithButtons:
                SendErrorSensor(sensors, TAppHostWalkerMonitoringNodeHandler::TextHasError(card.GetTextWithButtons().GetText()), scenarioName);
                break;
            default:
                break;
        }
    }
}

TStatus TAppHostWalkerMonitoringNodeHandler::Execute(IAppHostCtx& ahCtx) const {
    for (auto& itemName : ahCtx.ItemProxyAdapter().GetItemNamesFromCache()) {
        TStringBuf scenarioName = itemName;
        if (!scenarioName.SkipPrefix(SCENARIO_ITEM_PREFIX)) {
            continue;
        }
        TMaybe<TErrorOr<NScenarios::TScenarioRunResponse>> response;
        if (scenarioName.ChopSuffix(SCENARIO_HTTP_ITEM_SUFFIX)) {
            auto httpResponse = ahCtx.ItemProxyAdapter().GetFromContext<NAppHostHttp::THttpResponse>(itemName);
            response = ParseScenarioResponse<NScenarios::TScenarioRunResponse>(httpResponse, /* method= */ "run");
        } else if (scenarioName.ChopSuffix(SCENARIO_PURE_ITEM_SUFFIX)) {
            response = ahCtx.ItemProxyAdapter().GetFromContext<NScenarios::TScenarioRunResponse>(itemName);
        }
        if (response.Defined() && response->Error()) {
            LOG_ERR(ahCtx.Log()) << "Cannot get " << scenarioName
                                 << " scenario response from context in monitoring node: " << *response->Error();
            continue;
        }
        PushScenariosTexts(response->Value().GetResponseBody(), ToString(scenarioName), ahCtx.GlobalCtx().ServiceSensors());
    }
    TVector<NMegamindAppHost::TScenarioTextResponse> responses;
    auto callback = [&responses](const NMegamindAppHost::TScenarioTextResponse& proto) {
        responses.push_back(proto);
    };
    ahCtx.ItemProxyAdapter().ForEachCached<NMegamindAppHost::TScenarioTextResponse>(AH_ITEM_SCENARIOS_RESPONSE_MONITORING, callback);
    for (const auto& textResponse : responses) {
        for (const auto& text : textResponse.GetText()) {
            SendErrorSensor(ahCtx.GlobalCtx().ServiceSensors(), TextHasError(text), textResponse.GetScenarioName());
        }
    }
    return Success();
}

} // namespace NAlice::NMegamind
