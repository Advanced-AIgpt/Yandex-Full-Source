#pragma once

#include <alice/library/metrics/sensors.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

class TRequestCtx;

namespace NMegamind {

inline constexpr TStringBuf TS_STAGE_START_REQUEST = "request.start";
inline constexpr TStringBuf TS_STAGE_WEBSEARCH_AFTER_RESPONSE = "websearch.after_response";
inline constexpr TStringBuf TS_STAGE_WEBSEARCH_REQUESTED = "websearch.requested";
inline constexpr TStringBuf TS_STAGE_WALKER_AFTER_PRECLASSIFY = "request.walker.after_preclassify";
inline constexpr TStringBuf TS_STAGE_WALKER_AFTER_POSTCLASSIFY = "request.walker.after_postclassify";
inline constexpr TStringBuf TS_STAGE_WALKER_AFTER_PREPARE_REQUEST = "request.walker.after_prepare_request";
inline constexpr TStringBuf TS_STAGE_WALKER_AFTER_SCENARIOS = "request.walker.after_scenarios";
inline constexpr TStringBuf TS_STAGE_WALKER_BEFORE_PRECLASSIFY = "request.walker.before_preclassify";
inline constexpr TStringBuf TS_STAGE_WALKER_BEFORE_PREPARE_REQUEST = "request.walker.before_prepare_request";
inline constexpr TStringBuf TS_STAGE_WALKER_AFTER_PROCESS_CONTINUE = "request.walker.after_process_continue";
inline constexpr TStringBuf TS_STAGE_WALKER_AFTER_FINALIZE = "request.walker.after_finalize";

class TStageTimers {
public:
    TStageTimers() = default;
    virtual ~TStageTimers() = default;

    /** Register timestamp and push as diff signal with startStageName.
     *  return false if startStageName has not been found.
     */
    bool RegisterAndSignal(TRequestCtx& ctx, TStringBuf stageName, TStringBuf startStageName, NMetrics::ISensors& sensors);
    TInstant Register(TStringBuf name);
    const TInstant* Find(TStringBuf name) const;

protected:
    /** Add signal into storage
     */
    void Add(const TString& name, TInstant at) {
        Storage_.emplace(name, at);
    }

    /** Additional actions after register is done.
     *  (i.e. for apphost is to send to context)
     */
    virtual void Upload(TStringBuf /* name */, TInstant /* at */) {
    }

private:
    THashMap<TString, TInstant> Storage_;
};

} // namespace NMegamind
} // namespace NAlice
