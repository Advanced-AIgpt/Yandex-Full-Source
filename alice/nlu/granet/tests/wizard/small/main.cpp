#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/compiler/source_text_collection.h>
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetWizardSmall) {

    TFsPath GetGrammarPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/granet/main.grnt");
    }

    TFsPath GetBatchPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/test/granet/small");
    }

    void Test(const TGranetDomain& domain, const TGrammar::TConstRef& grammar) {
        const TString message = NBatch::EasyUnitTest(grammar, {.BatchDir = GetBatchPath(domain)});
        if (!message.empty()) {
            UNIT_FAIL(message);
        }
    }

    void TestPlain(const TGranetDomain& domain) {
        Test(domain, NCompiler::TCompiler().CompileFromPath(GetGrammarPath(domain), domain));
    }

    void TestSourceTextCollection(const TGranetDomain& domain) {
        const NCompiler::TSourceTextCollection sources1 = NCompiler::TCompiler().CollectSourceTexts(GetGrammarPath(domain), domain);
        NCompiler::TSourceTextCollection sources2;
        sources2.FromCompressedBase64(sources1.ToCompressedBase64());
        Test(domain, NCompiler::TCompiler().CompileFromSourceTextCollection(sources2));
    }

    void Test(const TGranetDomain& domain) {
        TestPlain(domain);
        TestSourceTextCollection(domain);
    }

    Y_UNIT_TEST(Russian) {
        Test({.Lang = LANG_RUS, .IsWizard = true});
    }
}
