#pragma once

#include "fwd.h"

#include <alice/bass/libs/scheduler/scheduler.h>

#include <alice/megamind/library/config/protos/classification_config.pb.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/scenarios/registry/fwd.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/logger/fwd.h>
#include <alice/library/metrics/fwd.h>

#include <library/cpp/http/io/headers.h>
#include <library/cpp/logger/priority.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

class TLog;
class IScheduler;
class IThreadPool;
class TFullModel;

namespace NFactorSlices {
class TFactorDomain;
} // namespace NFactorSlices

namespace NGeobase {
class TLookup;
} // namespace NGeobase

namespace NCatboostCalcer {
class TCatboostCalcer;
} // namespace NCatboostCalcer

namespace NInfra {
class TLogger;
} // namespace NInfra

namespace NAlice {

namespace NNlg {
struct INlgRenderer;
} // namespace NNlg

class TConfig;
class TFormulasStorage;
class TRTLogClient;

using TPartialPreCalcer = NCatboostCalcer::TCatboostCalcer;

class IGlobalCtx : private NNonCopyable::TNonCopyable {
public:
    virtual ~IGlobalCtx() = default;

    virtual const TConfig& Config() const = 0;

    virtual const NMegamind::TClassificationConfig& ClassificationConfig() const = 0;

    virtual TRTLogger& BaseLogger() = 0;

    virtual TRTLogger RTLogger(TStringBuf token, bool session = false,
                               TMaybe<ELogPriority> logPriorityFromRequest = Nothing()) const = 0;

    virtual TRTLogClient& RTLogClient() const = 0;

    virtual NMetrics::ISensors& ServiceSensors() = 0;

    virtual const NFactorSlices::TFactorDomain& GetFactorDomain() const = 0;

    virtual const TFormulasStorage& GetFormulasStorage() const = 0;

    virtual TLog& MegamindAnalyticsLog() = 0;

    virtual TLog& MegamindProactivityLog() = 0;

    virtual const NNlg::INlgRenderer& GetNlgRenderer() const = 0;

    virtual const NGeobase::TLookup& GeobaseLookup() const = 0;

    virtual const TScenarioConfigRegistry& ScenarioConfigRegistry() const = 0;

    virtual const TCombinatorConfigRegistry& CombinatorConfigRegistry() const = 0;

    virtual const TString& RngSeedSalt() const = 0;

    virtual IScheduler& Scheduler() = 0;

    virtual const TString& ProxyHost() const = 0;

    virtual ui32 ProxyPort() const = 0;

    virtual const THttpHeaders& ProxyHeaders() const = 0;

    virtual IThreadPool& RequestThreads() = 0;

    virtual const TPartialPreCalcer& PartialPreClassificationCalcer() const = 0;
};

} // namespace NAlice
