#include "names.h"

#include <alice/library/client/client_info.h>

#include <util/string/split.h>

namespace NAlice::NSignal {

namespace {

constexpr TStringBuf DELIM = ".";
constexpr TStringBuf HTTP = "http";
constexpr TStringBuf PROTOCOL = "protocol";
constexpr TStringBuf SCENARIO_COMMON_PREFIX = "scenario";

constexpr TStringBuf RETRY_LABEL = "retry_";
constexpr TStringBuf GET_NEXT_LABEL = "get_next_";
constexpr TStringBuf EMPTY_STRING = "";

const TString COMMON_HTTP = TStringBuilder{} << SCENARIO_COMMON_PREFIX << DELIM << HTTP << DELIM;
const TString COMMON_PROTOCOL = TStringBuilder{} << SCENARIO_COMMON_PREFIX << DELIM << PROTOCOL << DELIM;
const TString CLIENT_TYPE_OTHER = "other";

const TString POSTROLL_ACTION_TYPE = "action_type";
const TString POSTROLL_ITEM_INFO = "item_info";
const TString POSTROLL_SOURCE = "source";

const TString PRODUCT_SCENARIO = "product_scenario";
const TString PURPOSE = "purpose";

TString GetClientTypeSmart(bool isSmartSpeaker) {
    return isSmartSpeaker ? CLIENT_TYPE_SMART_SPEAKER : CLIENT_TYPE_NON_SMART_SPEAKER;
}

} // namespace

TStringBuf TProtocolScenarioHttpLabelsGenerator::GetHedgedLabel(const bool hedged) {
    return hedged ? RETRY_LABEL : EMPTY_STRING;
}

TStringBuf TProtocolScenarioHttpLabelsGenerator::GetGetNextLabel(const bool getNext) {
    return getNext ? GET_NEXT_LABEL : EMPTY_STRING;
}

bool TProtocolScenarioHttpLabelsGenerator::IsRequestSourceTypeGetNext() const {
    return RequestSourceType == NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext;
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::AddCommonLabels(NMonitoring::TLabels&& labels) const {
    labels.Add("scenario_name", ScenarioInfo.Name);
    labels.Add("request_type", ScenarioInfo.RequestType);
    labels.Add("client_type", GetClientTypeSmart(IsSmartSpeaker));
    return labels;
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::RequestSize(bool hedged,
                                                                       NMonitoring::EMetricType sensorKind) const {
    auto value = TStringBuilder{} << COMMON_HTTP << GetHedgedLabel(hedged) << "request_size_bytes";
    if (sensorKind ==  NMonitoring::EMetricType::RATE) {
        value << "_per_second";
    }
    return AddCommonLabels({{"name", value}});
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::ResponseSize(const bool hedged) const {
    return AddCommonLabels(
        {{"name", TStringBuilder{} << COMMON_HTTP << GetHedgedLabel(hedged) << "response_size_bytes"}});
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::RequestType(const bool hedged) const {
    return AddCommonLabels(
        {{"name", TStringBuilder{} << COMMON_HTTP << GetHedgedLabel(hedged) << "request_types_per_second"}});
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::ResponseType(const bool hedged) const {
    return AddCommonLabels(
        {{"name", TStringBuilder{} << COMMON_HTTP << GetHedgedLabel(hedged) << "response_types_per_second"}});
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::ResponseHttpCode(const TString& code,
                                                                           const bool hedged) const {
    return AddCommonLabels(
        {{"http_code", code},
         {"name", TStringBuilder{} << COMMON_HTTP << GetHedgedLabel(hedged) << "response_http_codes_per_second"}});
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::ResponseStatus(const TString& result,
                                                                         const bool hedged,
                                                                         const bool getNext) const {
    return AddCommonLabels(
        {{"response_status", result},
         {"name", TStringBuilder{} << COMMON_HTTP << GetGetNextLabel(getNext) << GetHedgedLabel(hedged) << "response_statuses_per_second"}});
}

NMonitoring::TLabels TProtocolScenarioHttpLabelsGenerator::ResponseTime(const bool hedged, const bool getNext) const {
    return AddCommonLabels(
        {{"name", TStringBuilder{} << COMMON_HTTP << GetGetNextLabel(getNext) << GetHedgedLabel(hedged) << "response_time_milliseconds"}});
}

TProtocolScenarioHttpLabelsGenerator::TProtocolScenarioHttpLabelsGenerator(const TString& scenarioName,
                                                                           bool isSmartSpeaker,
                                                                           const NScenarios::TScenarioBaseRequest_ERequestSourceType& requestSourceType)
    : ScenarioInfo(scenarioName)
    , IsSmartSpeaker(isSmartSpeaker)
    , RequestSourceType(requestSourceType) {
}

TProtocolScenarioHttpLabelsGenerator::TScenarioInfo::TScenarioInfo(const TString& scenarioName) {
    TVector<TString> splittedName = StringSplitter(scenarioName).Split('-');
    Name = splittedName[0];
    RequestType = splittedName[1];
}

TPrestartingLabelsGenerator::TPrestartingLabelsGenerator(const TString& client)
    : IsSmartSpeaker(NAlice::IsSmartSpeaker(client)) {
}

NMonitoring::TLabels TPrestartingLabelsGenerator::RequestIncoming() const {
    auto labels = HTTP_REQUEST_INCOMING;
    return AddCommonLabels(std::move(labels));
}

NMonitoring::TLabels TPrestartingLabelsGenerator::RequestSuccess() const {
    auto labels = HTTP_REQUEST_SUCCESS;
    return AddCommonLabels(std::move(labels));
}

NMonitoring::TLabels TPrestartingLabelsGenerator::RequestException() const {
    auto labels = HTTP_REQUEST_EXCEPTION;
    return AddCommonLabels(std::move(labels));
}

NMonitoring::TLabels TPrestartingLabelsGenerator::RequestTime() const {
    auto labels = HTTP_REQUEST_TIME;
    return AddCommonLabels(std::move(labels));
}


NMonitoring::TLabels TPrestartingLabelsGenerator::AddCommonLabels(NMonitoring::TLabels&& labels) const {
    labels.Add(CLIENT_TYPE, GetClientTypeSmart(IsSmartSpeaker));
    return labels;
}

TProtocolScenarioLabelsGenerator::TProtocolScenarioLabelsGenerator(const TString& scenarioName)
    : ScenarioName(scenarioName) {
}

NMonitoring::TLabels TProtocolScenarioLabelsGenerator::AddCommonLabels(NMonitoring::TLabels&& labels) const {
    labels.Add("scenario_name", ScenarioName);
    return labels;
}

NMonitoring::TLabels TProtocolScenarioLabelsGenerator::Version() const {
    return AddCommonLabels({{"name", TStringBuilder{} << COMMON_PROTOCOL << "version"}});
}

NMonitoring::TLabels TProtocolScenarioLabelsGenerator::RequestResponseType(TStringBuf requestType,
                                                                          TStringBuf responseType) const {
    return AddCommonLabels({{"response_type", responseType},
                            {"request_type", requestType},
                            {"name", TStringBuilder{} << COMMON_PROTOCOL << "responses_per_second"}});
}

NMonitoring::TLabels TProtocolScenarioLabelsGenerator::ArgumentsSize(const TStringBuf type) const {
    return AddCommonLabels(
        {{"arguments_type", type}, {"name", TStringBuilder{} << COMMON_PROTOCOL << "arguments_size_bytes"}});
}

NMonitoring::TLabels TProtocolScenarioLabelsGenerator::StateSize() const {
    return AddCommonLabels({{"name", TStringBuilder{} << COMMON_PROTOCOL << "state_size_bytes"}});
}

NMonitoring::TLabels LabelsForInvokedScenario(const TString& scenarioName) {
    return {
        {SCENARIO_NAME, scenarioName},
        {NAME, "invokes_per_second"}
    };
}

NMonitoring::TLabels LabelsForWinningScenario(const TString& scenarioName) {
    return {
        {SCENARIO_NAME, scenarioName},
        {NAME, "wins_per_second"}
    };
}

NMonitoring::TLabels LabelsForWinningCombinator(const TString& combinatorName) {
    return {
        {COMBINATOR_NAME, combinatorName},
        {NAME, "combinator_wins_per_second"}
    };
}

NMonitoring::TLabels LabelsForIrrelevantCombinator(const TString& combinatorName) {
    return {
        {COMBINATOR_NAME, combinatorName},
        {NAME, "combinator_irrelevant_per_second"}
    };
}

NMonitoring::TLabels LabelsForWinningScenarioInCombinator(const TString& combinatorName,
                                                          const TString& scenarioName) {
    return {
        {COMBINATOR_NAME, combinatorName},
        {SCENARIO_NAME, scenarioName},
        {NAME, "scenario_in_combinator_wins_per_second"}
    };
}

TString GetClientType(const TString& client) {
    if (NAlice::IsSmartSpeaker(client)) {
        return CLIENT_TYPE_SMART_SPEAKER;
    }
    return CLIENT_TYPE_OTHER;
}

NMonitoring::TLabels LabelsForStackErrorsPerScenario(const TString& scenarioName, const TString& errorType) {
    NMonitoring::TLabels labels = LabelsForScenarioInStack(scenarioName, STACK_ENGINE_ERRORS_PER_SECOND);
    labels.Add(STACK_ENGINE_ERROR_TYPE, errorType);
    return labels;
}

NMonitoring::TLabels LabelsForStackRecoveriesPerScenario(const TString& scenarioName, const TString& type) {
    NMonitoring::TLabels labels = LabelsForScenarioInStack(scenarioName, STACK_ENGINE_RECOVERIES_PER_SECOND);
    labels.Add(STACK_ENGINE_RECOVERY_TYPE, type);
    return labels;
}

NMonitoring::TLabels LabelsForScenarioInStack(const TString& scenarioName, const TString& sensorsName) {
    return {{SCENARIO_NAME, scenarioName},
            {NAME, TStringBuilder{} << STACK_ENGINE_PREFIX << STACK_DELIM << sensorsName}};
}

NMonitoring::TLabels LabelsForPartialPreClassificationReqs(bool isSmartSpeaker, EPartialPreStatus status) {
    return {
        {CLIENT_TYPE, GetClientTypeSmart(isSmartSpeaker)},
        {NAME, "partial_preclassifier.preclassifications_per_second"},
        {STATUS, ToString(status)},
    };
}

NMonitoring::TLabels LabelsForPartialPreClassificationErrors(bool isSmartSpeaker) {
    return {
        {CLIENT_TYPE, GetClientTypeSmart(isSmartSpeaker)},
        {NAME, "partial_preclassifier.errors"},
    };
}

NMonitoring::TLabels LabelsForPostrollSource(const TString& actionType, const TString& source) {
    return {
        {POSTROLL_ACTION_TYPE, actionType},
        {POSTROLL_SOURCE, source},
        {NAME, "postroll_action_source"},
    };
}

NMonitoring::TLabels LabelsForPostrollItemInfo(const TString& actionType, const TString& item_info) {
    return {
        {POSTROLL_ACTION_TYPE, actionType},
        {POSTROLL_ITEM_INFO, item_info},
        {NAME, "postroll_action_item_info"},
    };
}

NMonitoring::TLabels LabelsForDeviceStateEntitiesLimitExceeded(const TString& clientName, const TString& entityName) {
    return {
        {CLIENT_TYPE, clientName},
        {NAME, "device_state_entities_limit_exceeded"},
        {ENTITY_NAME, entityName},
    };
}

NMonitoring::TLabels LabelsForActivatedConditionalAction(const TString& productScenario, const TString& purpose) {
    return {
        {NAME, "activated_conditional_action"},
        {PRODUCT_SCENARIO, productScenario},
        {PURPOSE, purpose}
    };
}

} // namespace NAlice::NSignal
