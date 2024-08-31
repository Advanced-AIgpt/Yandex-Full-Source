#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <alice/nlu/granet/lib/granet.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetAliceMedium) {

    TFsPath GetGrammarPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/granet/main.grnt");
    }

    TFsPath GetBatchPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/test/granet/medium");
    }

    void FailIfNotOk(const NBatch::TBatchProcessor& tester) {
        const TString message = tester.GetResultUtMessage();
        if (!message.empty()) {
            UNIT_FAIL(message);
        }
    }

    void CheckUnitTestLimits(const TGranetDomain& domain) {
        NBatch::TBatchProcessor tester({.BatchDir = GetBatchPath(domain)});
        tester.CheckUnitTestLimits();
        FailIfNotOk(tester);
    }

    void CheckFixlist(const TGranetDomain& domain) {
        TGrammar::TConstRef grammar = NCompiler::TCompiler().CompileFromPath(GetGrammarPath(domain), domain);
        NBatch::TBatchProcessor tester({.BatchDir = GetBatchPath(domain)});
        tester.CheckFixlist(grammar);
        FailIfNotOk(tester);
    }

    void Test(const TGranetDomain& domain, TStringBuf namePrefix) {
        TGrammar::TConstRef grammar = NCompiler::TCompiler().CompileFromPath(GetGrammarPath(domain), domain);
        NBatch::TBatchProcessor tester({.BatchDir = GetBatchPath(domain), .FilterByNamePrefixIgnoreCase = TString(namePrefix)});
        tester.Test(grammar);
        FailIfNotOk(tester);
    }

    Y_UNIT_TEST(Russian_CheckUnitTestLimits) {
        CheckUnitTestLimits({.Lang = LANG_RUS});
    }

    Y_UNIT_TEST(Russian_CheckFixlist) {
        CheckFixlist({.Lang = LANG_RUS});
    }

    Y_UNIT_TEST(Russian_PartA) { Test({.Lang = LANG_RUS}, "A"); }
    Y_UNIT_TEST(Russian_PartB) { Test({.Lang = LANG_RUS}, "B"); }
    Y_UNIT_TEST(Russian_PartC) { Test({.Lang = LANG_RUS}, "C"); }
    Y_UNIT_TEST(Russian_PartD) { Test({.Lang = LANG_RUS}, "D"); }
    Y_UNIT_TEST(Russian_PartE) { Test({.Lang = LANG_RUS}, "E"); }
    Y_UNIT_TEST(Russian_PartF) { Test({.Lang = LANG_RUS}, "F"); }
    Y_UNIT_TEST(Russian_PartG) { Test({.Lang = LANG_RUS}, "G"); }
    Y_UNIT_TEST(Russian_PartH) { Test({.Lang = LANG_RUS}, "H"); }
    Y_UNIT_TEST(Russian_PartI) { Test({.Lang = LANG_RUS}, "I"); }
    Y_UNIT_TEST(Russian_PartJ) { Test({.Lang = LANG_RUS}, "J"); }
    Y_UNIT_TEST(Russian_PartK) { Test({.Lang = LANG_RUS}, "K"); }
    Y_UNIT_TEST(Russian_PartL) { Test({.Lang = LANG_RUS}, "L"); }
    Y_UNIT_TEST(Russian_PartM) { Test({.Lang = LANG_RUS}, "M"); }
    Y_UNIT_TEST(Russian_PartN) { Test({.Lang = LANG_RUS}, "N"); }
    Y_UNIT_TEST(Russian_PartO) { Test({.Lang = LANG_RUS}, "O"); }
    Y_UNIT_TEST(Russian_PartP) { Test({.Lang = LANG_RUS}, "P"); }
    Y_UNIT_TEST(Russian_PartQ) { Test({.Lang = LANG_RUS}, "Q"); }
    Y_UNIT_TEST(Russian_PartR) { Test({.Lang = LANG_RUS}, "R"); }
    Y_UNIT_TEST(Russian_PartS) { Test({.Lang = LANG_RUS}, "S"); }
    Y_UNIT_TEST(Russian_PartT) { Test({.Lang = LANG_RUS}, "T"); }
    Y_UNIT_TEST(Russian_PartU) { Test({.Lang = LANG_RUS}, "U"); }
    Y_UNIT_TEST(Russian_PartV) { Test({.Lang = LANG_RUS}, "V"); }
    Y_UNIT_TEST(Russian_PartW) { Test({.Lang = LANG_RUS}, "W"); }
    Y_UNIT_TEST(Russian_PartX) { Test({.Lang = LANG_RUS}, "X"); }
    Y_UNIT_TEST(Russian_PartY) { Test({.Lang = LANG_RUS}, "Y"); }
    Y_UNIT_TEST(Russian_PartZ) { Test({.Lang = LANG_RUS}, "Z"); }

    Y_UNIT_TEST(Turkish_CheckUnitTestLimits) {
        CheckUnitTestLimits({.Lang = LANG_TUR});
    }

    Y_UNIT_TEST(Turkish_CheckFixlist) {
        CheckFixlist({.Lang = LANG_TUR});
    }

    Y_UNIT_TEST(Turkish) {
        Test({.Lang = LANG_TUR}, "");
    }

    Y_UNIT_TEST(Arabic_CheckUnitTestLimits) {
        CheckUnitTestLimits({.Lang = LANG_ARA});
    }

    Y_UNIT_TEST(Arabic_CheckFixlist) {
        CheckFixlist({.Lang = LANG_ARA});
    }

    Y_UNIT_TEST(Arabic) {
        Test({.Lang = LANG_ARA}, "");
    }
}
