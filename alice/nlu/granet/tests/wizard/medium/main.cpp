#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <alice/nlu/granet/lib/granet.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetWizardMedium) {

    TFsPath GetGrammarPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/granet/main.grnt");
    }

    TFsPath GetBatchPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/test/granet/medium");
    }

    void Test(const TGranetDomain& domain) {
        TGrammar::TConstRef grammar = NCompiler::TCompiler().CompileFromPath(GetGrammarPath(domain), domain);
        const TString message = NBatch::EasyUnitTest(grammar, {.BatchDir = GetBatchPath(domain)});
        if (!message.empty()) {
            UNIT_FAIL(message);
        }
    }

    Y_UNIT_TEST(Russian) {
        Test({.Lang = LANG_RUS, .IsWizard = true});
    }
}
