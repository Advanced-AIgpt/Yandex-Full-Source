#pragma once

#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/config/protos/config.pb.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/scenarios/config_registry/config_registry.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/library/logger/logger.h>
#include <alice/library/unittest/mock_sensors.h>

#include <catboost/libs/model/model.h>

#include <kernel/catboost/catboost_calcer.h>
#include <kernel/factor_slices/factor_domain.h>

#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/logger/log.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

namespace NTesting {

struct TGenericInitMockedGlobalCtx {
    TGenericInitMockedGlobalCtx();

    TConfig Config;
    TString RngSalt = "fake_rng_salt";
    TNoopSensors Sensors;
};

} // NTesting

class TMockGlobalContext : public IGlobalCtx {
public:
    enum class EInit {
        Noop,
        GenericInit
    };
public:
    TMockGlobalContext(EInit init = EInit::Noop) {
        if (init == EInit::GenericInit) {
            GenericInit();
        }

        ON_CALL(*this, ClassificationConfig())
            .WillByDefault(testing::ReturnRef(NMegamind::TClassificationConfig::default_instance()));
    }

    MOCK_METHOD(TConfig&, Config, (), (const, override));
    MOCK_METHOD(const NMegamind::TClassificationConfig&, ClassificationConfig, (), (const, override));
    MOCK_METHOD(TRTLogger&, BaseLogger, (), (override));
    MOCK_METHOD(TRTLogger, RTLogger, (TStringBuf, bool, TMaybe<ELogPriority>), (const, override));
    MOCK_METHOD(TRTLogClient&, RTLogClient, (), (const, override));
    MOCK_METHOD(NMetrics::ISensors&, ServiceSensors, (), (override));
    MOCK_METHOD(NFactorSlices::TFactorDomain&, GetFactorDomain, (), (const, override));
    MOCK_METHOD(TFormulasStorage&, GetFormulasStorage, (), (const, override));
    MOCK_METHOD(TLog&, MegamindAnalyticsLog, (), (override));
    MOCK_METHOD(TLog&, MegamindProactivityLog, (), (override));
    MOCK_METHOD(const NNlg::INlgRenderer&, GetNlgRenderer, (), (const, override));
    MOCK_METHOD(const NGeobase::TLookup&, GeobaseLookup, (), (const, override));
    MOCK_METHOD(TScenarioConfigRegistry&, ScenarioConfigRegistry, (), (const, override));
    MOCK_METHOD(TCombinatorConfigRegistry&, CombinatorConfigRegistry, (), (const, override));
    MOCK_METHOD(const TString&, RngSeedSalt, (), (const, override));
    MOCK_METHOD(IScheduler&, Scheduler, (), (override));
    MOCK_METHOD(const TString&, ProxyHost, (), (const, override));
    MOCK_METHOD(ui32, ProxyPort, (), (const, override));
    MOCK_METHOD(const THttpHeaders&, ProxyHeaders, (), (const, override));
    MOCK_METHOD(IThreadPool&, RequestThreads, (), (override));
    MOCK_METHOD(const TPartialPreCalcer&, PartialPreClassificationCalcer, (), (const, override));

public:
    const NTesting::TGenericInitMockedGlobalCtx& GenericInit();

private:
    TMaybe<NTesting::TGenericInitMockedGlobalCtx> GenericInit_;
};

} // namespace NAlice
