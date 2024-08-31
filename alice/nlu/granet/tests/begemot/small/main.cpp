#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/compiler/source_text_collection.h>
#include <alice/nlu/granet/lib/granet.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetAliceSmall) {

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

    void CheckFreshRestrictions(const TGranetDomain& domain) {
        TGrammar::TConstRef grammar = NCompiler::TCompiler().CompileFromPath(GetGrammarPath(domain), domain);
        const TString message = NBatch::CheckFreshRestrictions(grammar);
        if (!message.empty()) {
            UNIT_FAIL(message);
        }
    }

    Y_UNIT_TEST(Russian) {
        TestPlain({.Lang = LANG_RUS});
    }
    Y_UNIT_TEST(Russian_SourceTextCollection) {
        TestSourceTextCollection({.Lang = LANG_RUS});
    }
    Y_UNIT_TEST(Russian_CheckFreshRestrictions) {
        CheckFreshRestrictions({.Lang = LANG_RUS});
    }

    Y_UNIT_TEST(Turkish) {
        TestPlain({.Lang = LANG_TUR});
    }
    Y_UNIT_TEST(Turkish_SourceTextCollection) {
        TestSourceTextCollection({.Lang = LANG_TUR});
    }
    Y_UNIT_TEST(Turkish_CheckFreshRestrictions) {
        CheckFreshRestrictions({.Lang = LANG_TUR});
    }

    Y_UNIT_TEST(Kazakh) {
        TestPlain({.Lang = LANG_KAZ});
    }
    Y_UNIT_TEST(Kazakh_SourceTextCollection) {
        TestSourceTextCollection({.Lang = LANG_KAZ});
    }
    Y_UNIT_TEST(Kazakh_CheckFreshRestrictions) {
        CheckFreshRestrictions({.Lang = LANG_KAZ});
    }

    Y_UNIT_TEST(PaskillsRussian) {
        TestPlain({.Lang = LANG_RUS, .IsPASkills = true});
    }
    Y_UNIT_TEST(PaskillsRussian_SourceTextCollection) {
        TestSourceTextCollection({.Lang = LANG_RUS, .IsPASkills = true});
    }
    Y_UNIT_TEST(PaskillsRussian_CheckFreshRestrictions) {
        CheckFreshRestrictions({.Lang = LANG_RUS, .IsPASkills = true});
    }

    Y_UNIT_TEST(SnezhanaRussian) {
        TestPlain({.Lang = LANG_RUS, .IsSnezhana = true});
    }
    Y_UNIT_TEST(SnezhanaRussian_SourceTextCollection) {
        TestSourceTextCollection({.Lang = LANG_RUS, .IsSnezhana = true});
    }
    Y_UNIT_TEST(SnezhanaRussian_CheckFreshRestrictions) {
        CheckFreshRestrictions({.Lang = LANG_RUS, .IsSnezhana = true});
    }

    Y_UNIT_TEST(Arabic) {
        TestPlain({.Lang = LANG_ARA});
    }
    Y_UNIT_TEST(Arabic_SourceTextCollection) {
        TestSourceTextCollection({.Lang = LANG_ARA});
    }
    Y_UNIT_TEST(Arabic_CheckFreshRestrictions) {
        CheckFreshRestrictions({.Lang = LANG_ARA});
    }
}
