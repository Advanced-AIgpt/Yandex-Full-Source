#include "continue.h"

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/utils.h>

#include <kernel/factor_storage/factor_storage.h>
#include <kernel/formula_storage/formula_storage.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace ::testing;
using namespace NAlice;

namespace {

class TMockFormulaApplier : public NImpl::IFormulaApplier {
public:
    MOCK_METHOD(double, Predict, (const IContext&, const ::TFactorStorage&, const NAlice::TFormulasStorage&,
                                       TStringBuf, TStringBuf, EClientType, const TScenarioResponse&), (const, override));
};

Y_UNIT_TEST_SUITE(ContinueClassifier) {
    Y_UNIT_TEST(MockClassifier) {
        TFactorStorage factors = CreateStorage();

        auto& logger = TRTLogger::StderrLogger();
        StrictMock<TMockContext> ctx;
        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(logger));
        EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_EARLY_CONTINUE)).WillRepeatedly(Return(true));

        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        TClientInfo clientInfo{/* proto= */ TClientInfoProto{}};
        clientInfo.Name = "ru.yandex.quasar.app";
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TConfig config = GetRealConfig();
        const auto classificationConfig = GetRealClassificationConfig();
        const auto& scenarioClassificationConfigs = classificationConfig.GetScenarioClassificationConfigs();
        const TFormulasDescription formulasDescription{scenarioClassificationConfigs, config.GetFormulasPath(),
                                                       logger};

        ::TFormulasStorage rawFormulasStorage;
        NAlice::TFormulasStorage formulasStorage{rawFormulasStorage, formulasDescription};

        const TString musicScenario{HOLLYWOOD_MUSIC_SCENARIO};
        TScenarioResponse scenarioResponse{musicScenario, /* scenarioSemanticFrames= */ {},
                                           /* acceptsAnyUtterance= */ false};
    }
}

} // namespace
