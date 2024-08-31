#pragma once

#include "talkien.h"

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>
#include <alice/megamind/library/scenarios/interface/scenario.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/protos/proactivity/proactivity.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/stream/fwd.h>

namespace NAlice {

class IHttpResponse;

class TScenariosErrors {
public:
    void Add(const TString& scenarioName, TStringBuf stage, const TError& error) {
        Errors[scenarioName][stage] = error;
    }

    bool Empty() const {
        return Errors.empty();
    }

    void ToHttpResponse(IHttpResponse& response) const;

    friend void operator<<(IOutputStream& os, const TScenariosErrors& e) {
        os << "TScenariosErrors [" << e.ErrorsToString() << "]";
    }

    template <typename TFn>
    void ForEachError(TFn&& fn) const {
        for (const auto& [scenarioName, scenarioError] : Errors) {
            for (const auto& [stage, error] : scenarioError) {
                fn(scenarioName, stage, error);
            }
        }
    }

    TString ErrorsToString() const;

    static TString ErrorToString(const TString& scenarioName, const TString& stage, const TError& error);

private:
    THashMap<TString, THashMap<TString, TError>> Errors;
};

struct TWalkerResponse {
    TWalkerResponse() = default;

    explicit TWalkerResponse(TVector<TScenarioResponse>&& scenarios)
        : Scenarios(std::move(scenarios)) {
    }

    explicit TWalkerResponse(const TDirectiveListResponse& action)
        : Action(action)
        , ApplyResult(EApplyResult::Called) {
    }

    explicit TWalkerResponse(const TError& error)
        : Error(error) {
    }

    bool HasResponses() const {
        return !Scenarios.empty();
    }

    template <typename TResponse>
    void AddScenarioResponse(TResponse&& response) {
        // NOTE (a-sidorin@): we protect the vector from concurrent writes. The reads should occur
        // in the main worker thread only.
        with_lock (ScenariosWriteLock) {
            Scenarios.emplace_back(std::forward<TResponse>(response));
        }
    }

    void AddScenarioError(const TString& scenarioName, TStringBuf stage, const TError& error) {
        // NOTE (a-sidorin@): we protect the vector from concurrent writes. The reads should occur
        // in the main worker thread only.
        with_lock (ErrorsWriteLock) {
            Errors.Add(scenarioName, stage, error);
        }
    }

    const TScenariosErrors& GetScenarioErrors() const {
        return Errors;
    }

    template <typename TVisitor>
    void Visit(TVisitor&& visitor) const {
        if (Action.Defined()) {
            visitor(*Action);
            return;
        }
        if (!Scenarios.empty()) {
            visitor(Scenarios[0]);
            return;
        }
        if (Error) {
            visitor(*Error);
            return;
        }
        if (!Errors.Empty()) {
            visitor(Errors);
            return;
        }
        visitor(TError{TError::EType::Logic} << "Empty response scenarios set!");
    }

    TMaybe<TDirectiveListResponse> Action;

    TAdaptiveLock ScenariosWriteLock;
    TVector<TScenarioResponse> Scenarios; // Guarded by ScenariosLock.

    TAdaptiveLock ErrorsWriteLock;
    TScenariosErrors Errors; // Guarded by ErrorsLock.

    TMaybe<TError> Error;
    TMaybe<EApplyResult> ApplyResult;
    NMegamind::TMegamindAnalyticsInfoBuilder AnalyticsInfoBuilder;
    TQualityStorage QualityStorage;
    NMegamind::TProactivityLogStorage ProactivityLogStorage;
};

} // namespace NAlice
