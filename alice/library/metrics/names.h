#pragma once

#include "aggregate_labels_builder.h"

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/monlib/metrics/labels.h>
#include <library/cpp/monlib/metrics/metric_type.h>

namespace NAlice::NSignal {

inline constexpr TStringBuf RESPONSE_HTTP_CODE_LABEL_NAME = "response_http_code";

inline const NMonitoring::TLabels POST_CLASSIFY_AMBIGUOUS_WIN = {{"name", "ambiguous_wins_per_second"}};
inline constexpr TStringBuf WATCHDOG_REQUEST_KILL_SOON = "request.watchdog_kill_soon";

// AppHost related sensors.
inline const NMonitoring::TLabels APPHOST_REQUEST_EXCEPTION = {{"name", "apphost_request.failed_with_exception_requests_per_second"}};
inline const NMonitoring::TLabels APPHOST_REQUEST_INCOMING = {{"name", "apphost_request.incoming_requests_per_second"}};
inline const NMonitoring::TLabels APPHOST_REQUEST_SUCCESS = {{"name", "apphost_request.succeeded_requests_per_second"}};
inline const NMonitoring::TLabels APPHOST_REQUEST_TIME = {{"name", "apphost_request.time_milliseconds"}};
inline const NMonitoring::TLabels APPHOST_NEXTINPUT_IN_PROGRESS = {{"name", "apphost.next_input.in_progress"}};
inline constexpr TStringBuf APPHOST_ASYNC_THREADPOOL_ONGOING = "apphost.async_threadpool.ongoing";
inline constexpr TStringBuf APPHOST_GET_ITEM_FAILURE = "apphost.get_item.failure";
inline constexpr TStringBuf APPHOST_NODE_EXECUTION_TIME = "node.execution.time_milliseconds";

// Http related sensors
inline const NMonitoring::TLabels HTTP_REQUEST_EXCEPTION = {{"name", "http_request.failed_with_exception_requests_per_second"}};
inline const NMonitoring::TLabels HTTP_REQUEST_INCOMING = {{"name", "http_request.incoming_requests_per_second"}};
inline const NMonitoring::TLabels HTTP_REQUEST_RESPONSE_ERROR = {{"name", "http_request.response_errors_per_second"}};
inline const NMonitoring::TLabels HTTP_REQUEST_SUCCESS = {{"name", "http_request.succeeded_requests_per_second"}};
inline const NMonitoring::TLabels HTTP_REQUEST_TIME = {{"name", "http_request.time_milliseconds"}};
inline const NMonitoring::TLabels HTTP_REQUEST_WAIT_IN_QUEUE = {{"name", "http_request.wait_in_queue_time_milliseconds"}};
inline const NMonitoring::TLabels HTTP_SERVER_FLUSH_ERROR = {{"name", "http_server.flush_errors_per_second"}};
inline const NMonitoring::TLabels HTTP_SERVER_MAX_CONN = {{"name", "http_server.max_conns_per_second"}};
inline const NMonitoring::TLabels HTTP_SERVER_ON_WAIT = {{"name", "http_server.on_waits_per_second"}};
inline const NMonitoring::TLabels HTTP_SERVER_REQUEST_FAIL = {{"name", "http_server.fail_requests_per_second"}};
inline const NMonitoring::TLabels HTTP_SERVER_STARTED = {{"name", "http_server.on_listen_starts_per_second"}};
inline const NMonitoring::TLabels HTTP_HANDLER_NOT_FOUND_ERROR = {{"name", "handler.errors.not_found_per_second"}};

// stack engine related sensors
inline const TString STACK_ENGINE_EMPTY = "empty";
inline const TString STACK_ENGINE_ERROR_TYPE = "error_type";
inline const TString STACK_ENGINE_ERRORS_PER_SECOND = "errors_per_second";
inline const TString STACK_ENGINE_RECOVERED = "recovered";
inline const TString STACK_ENGINE_RECOVERY_TYPE = "recovery_type";
inline const TString STACK_ENGINE_RECOVERIES_PER_SECOND = "recoveries_per_second";
inline const TString STACK_ENGINE_INVALID_SESSION = "invalid_session";
inline const TString STACK_ENGINE_INVALID_WARMUP = "invalid_warmup";
inline const TString STACK_ENGINE_PREFIX = "stack_engine";
inline const TString STACK_DELIM = ".";
inline const TString STACK_REQUESTS_PER_SECOND = "requests_per_second";
inline const TString STACK_SIZE = "stack_size";
inline const TString STACK_ENGINE_INVALID_EFFECT = "invalid_effect";

inline const TString STACK_RECOVERED_FROM_MEMENTO = "from_memento";
inline const TString STACK_RECOVERED_FROM_CALLBACK = "from_callback";

inline const NMonitoring::TLabels HANDLER_RUN = {{"name", "handler.run_time_milliseconds"}};
inline const NMonitoring::TLabels HANDLER_APPLY = {{"name", "handler.apply_time_milliseconds"}};

inline const NMonitoring::TLabels WALKER_STAGE_POSTCLASSIFICATION = {{"name", "walker_stage.postclassification_time_milliseconds"}};
inline const NMonitoring::TLabels WALKER_STAGE_PRECLASSIFICATION = {{"name", "walker_stage.preclassification_time_milliseconds"}};

inline constexpr TStringBuf SCENARIO_BASED_TIMER_APPLY_STAGE = "scenario_stage.apply_time_milliseconds";
inline constexpr TStringBuf SCENARIO_BASED_TIMER_FINISH_STAGE = "scenario_stage.finish_time_milliseconds";
inline constexpr TStringBuf SCENARIO_BASED_TIMER_START_STAGE = "scenario_stage.start_time_milliseconds";

inline constexpr TStringBuf SCENARIO_WARMUP_PER_SECOND = "scenario.warmup_per_second";

inline constexpr TStringBuf TEST_IDS_ERRORS_PER_SECOND = "test_ids.errors_per_second";

inline const NMonitoring::TLabels LOGGER_VINS_LIKE_RECORDS_COUNT = {{"name", "logger_vins_like.records_count_per_second"}};
inline const NMonitoring::TLabels LOGGER_VINS_LIKE_RECORDS_LENGTH = {{"name", "logger_vins_like.records_bytes_per_second"}};
inline const NMonitoring::TLabels LOGGER_RECORDS_COUNT = {{"name", "logger.records_count_per_second"}};
inline const NMonitoring::TLabels LOGGER_RECORDS_LENGTH = {{"name", "logger.records_bytes_per_second"}};
inline const NMonitoring::TLabels INCORRECT_STATE_ON_EXTERNAL_BUTTON_ERRORS = {{"name", "incorrect_state_on_external_button_errors_per_second"}};
inline const NMonitoring::TLabels EMPTY_SESSIONS = {{"name", "empty_sessions_per_second"}};
inline const NMonitoring::TLabels SCENARIOS_WITH_SEARCH = {{"name", "scenario.with_search_per_second"}};

inline const TString CLIENT_TYPE = "client_type";
inline const TString ERROR_TYPE = "error_type";
inline const TString EVENT = "event";
inline const TString HOST = "host";
inline const TString HTTP_CODE = "http_code";
inline const TString NAME = "name";
inline const TString NLG_TEMPLATE_ID = "nlg_template_id";
inline const TString SCENARIO_NAME = "scenario_name";
inline const TString COMBINATOR_NAME = "combinator_name";
inline const TString SLOWEST_FAST_SCENARIO_COUNT = "fast_scenario.slowest_per_second";
inline const TString SLOWEST_FAST_SCENARIO_DIFF = "fast_scenario.slowest_diff_time_milliseconds";
inline const TString SLOWEST_HEAVY_SCENARIO_COUNT = "scenario.slowest_per_second";
inline const TString SLOWEST_HEAVY_SCENARIO_DIFF = "scenario.slowest_diff_time_milliseconds";
inline const TString TEST_ID = "test_id";
inline const TString CLIENT_TYPE_SMART_SPEAKER = "smart_speaker";
inline const TString CLIENT_TYPE_NON_SMART_SPEAKER = "not_smart_speaker";
inline const TString PROCESS_ID = "process_id";
inline const TString STATUS = "status";
inline constexpr TStringBuf INTENT = "intent";
inline constexpr TStringBuf UDP_SENSOR = "udp_sensor";
inline constexpr TStringBuf IS_WINNER_SCENARIO = "is_winner_scenario";
inline constexpr TStringBuf IS_IRRELEVANT = "irrelevant";
inline constexpr TStringBuf ENTITY_NAME = "entity_name";

inline const TAggregateLabelsBuilder ANY_NLG_TEMPLATE_ID_BUILDER = {"any_", {{"nlg_template_id"}}};

enum class EPartialPreStatus {
    Skip      /* "skip" */,
    Filtered  /* "filtered" */,
    Left      /* "left" */,
};

struct TWalkerStageApplyAndFinalizeLabels {
    NMonitoring::TLabels ApplyAndFinalize;
    NMonitoring::TLabels Apply;
    NMonitoring::TLabels Finalize;
};

class TProtocolScenarioHttpLabelsGenerator final {
public:
    explicit TProtocolScenarioHttpLabelsGenerator(const TString& scenarioName, bool isSmartSpeaker, const NScenarios::TScenarioBaseRequest_ERequestSourceType& requestSourceType);
    NMonitoring::TLabels RequestSize(bool hedged, NMonitoring::EMetricType sensorKind) const;
    NMonitoring::TLabels ResponseSize(bool hedged) const;
    NMonitoring::TLabels RequestType(bool hedged) const;
    NMonitoring::TLabels ResponseType(bool hedged) const;
    NMonitoring::TLabels ResponseHttpCode(const TString& code, bool hedged) const;
    NMonitoring::TLabels ResponseStatus(const TString& result, bool hedged, bool getNext) const;
    NMonitoring::TLabels ResponseTime(bool hedged, bool getNext) const;
    bool IsRequestSourceTypeGetNext() const;

private:
    NMonitoring::TLabels AddCommonLabels(NMonitoring::TLabels&& labels) const;
    static TStringBuf GetHedgedLabel(bool hedged);
    static TStringBuf GetGetNextLabel(bool getNext);
    struct TScenarioInfo {
        explicit TScenarioInfo(const TString& scenarioName);
        TString Name;
        TString RequestType;
    };
    const TScenarioInfo ScenarioInfo;
    bool IsSmartSpeaker;
    const NScenarios::TScenarioBaseRequest_ERequestSourceType RequestSourceType;
};

class TPrestartingLabelsGenerator final {
public:
    TPrestartingLabelsGenerator() = default;
    TPrestartingLabelsGenerator(const TString& client);
    NMonitoring::TLabels RequestIncoming() const;
    NMonitoring::TLabels RequestSuccess() const;
    NMonitoring::TLabels RequestException() const;
    NMonitoring::TLabels RequestTime() const;

private:
    NMonitoring::TLabels AddCommonLabels(NMonitoring::TLabels&& labels) const;
    bool IsSmartSpeaker = false;
};

class TProtocolScenarioLabelsGenerator final {
public:
    explicit TProtocolScenarioLabelsGenerator(const TString& scenarioName);
    NMonitoring::TLabels Version() const;
    NMonitoring::TLabels RequestResponseType(TStringBuf requestType, TStringBuf responseType) const;
    NMonitoring::TLabels ArgumentsSize(TStringBuf type) const;
    NMonitoring::TLabels StateSize() const;

private:
    NMonitoring::TLabels AddCommonLabels(NMonitoring::TLabels&& labels) const;
    TString ScenarioName;
};

NMonitoring::TLabels LabelsForInvokedScenario(const TString& scenarioName);
NMonitoring::TLabels LabelsForWinningScenario(const TString& scenarioName);
NMonitoring::TLabels LabelsForWinningCombinator(const TString& combinatorName);
NMonitoring::TLabels LabelsForIrrelevantCombinator(const TString& combinatorName);
NMonitoring::TLabels LabelsForWinningScenarioInCombinator(const TString& combinatorName,
                                                          const TString& scenarioName);

TString GetClientType(const TString& client);

NMonitoring::TLabels LabelsForStackErrorsPerScenario(const TString& scenarioName, const TString& errorType);
NMonitoring::TLabels LabelsForStackRecoveriesPerScenario(const TString& scenarioName, const TString& type);

NMonitoring::TLabels LabelsForScenarioInStack(const TString& scenarioName, const TString& sensorsName);

NMonitoring::TLabels LabelsForPartialPreClassificationReqs(bool isSmartSpeaker, EPartialPreStatus status);
NMonitoring::TLabels LabelsForPartialPreClassificationErrors(bool isSmartSpeaker);

NMonitoring::TLabels LabelsForPostrollSource(const TString& actionType, const TString& source);
NMonitoring::TLabels LabelsForPostrollItemInfo(const TString& actionType, const TString& item_info);

NMonitoring::TLabels LabelsForDeviceStateEntitiesLimitExceeded(const TString& clientName, const TString& entityName);

NMonitoring::TLabels LabelsForActivatedConditionalAction(const TString& productScenario, const TString& purpose);

} // namespace NAlice::NSignal
