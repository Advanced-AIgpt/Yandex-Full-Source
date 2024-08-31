#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/test/batch.h>
#include <alice/nlu/granet/lib/granet.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

using namespace NGranet;

Y_UNIT_TEST_SUITE(GranetAliceBatchCollection) {

    struct TTestingFilter {
        size_t Divisor = 0;
        size_t Remainder = 0;

        bool Match(TStringBuf text) const {
            return Divisor <= 1 || THash<TStringBuf>{}(text) % Divisor == Remainder;
        }
    };

    TFsPath GetGrammarPath(const TGranetDomain& domain) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/granet/main.grnt");
    }

    TFsPath GetBatchesPath(const TGranetDomain& domain, const TFsPath& batchCollectionName) {
        return BinaryPath("alice/nlu/data/" + domain.GetDirName() + "/test/granet") / batchCollectionName;
    }

    void Test(const TGranetDomain& domain, const TFsPath& batchCollectionName, const TTestingFilter& filter) {
        TGrammar::TConstRef grammar; // lazy creation
        const TFsPath batchesPath = GetBatchesPath(domain, batchCollectionName);
        TVector<TString> subdirs;
        batchesPath.ListNames(subdirs);
        Sort(subdirs);
        for (const TString& subdir : subdirs) {
            if (!filter.Match(subdir)) {
                continue;
            }
            const TFsPath batchDir = batchesPath / subdir;
            if (!batchDir.IsDirectory() || !(batchDir / "config.json").IsFile()) {
                continue;
            }
            if (grammar == nullptr) {
                grammar = NCompiler::TCompiler().CompileFromPath(GetGrammarPath(domain), domain);
            }
            NBatch::TBatchProcessor tester({.BatchDir = batchDir, .IsAutoTest = true});
            tester.CheckUnitTestLimits();
            if (!tester.GetResultUtMessage().empty()) {
                UNIT_FAIL(tester.GetResultUtMessage());
            }
            tester.Test(grammar);
            if (!tester.GetResultUtMessage().empty()) {
                UNIT_FAIL(tester.GetResultUtMessage());
            }
        }
    }

    Y_UNIT_TEST(RussianCustomPart0) { Test({.Lang = LANG_RUS}, "custom", {10, 0}); }
    Y_UNIT_TEST(RussianCustomPart1) { Test({.Lang = LANG_RUS}, "custom", {10, 1}); }
    Y_UNIT_TEST(RussianCustomPart2) { Test({.Lang = LANG_RUS}, "custom", {10, 2}); }
    Y_UNIT_TEST(RussianCustomPart3) { Test({.Lang = LANG_RUS}, "custom", {10, 3}); }
    Y_UNIT_TEST(RussianCustomPart4) { Test({.Lang = LANG_RUS}, "custom", {10, 4}); }
    Y_UNIT_TEST(RussianCustomPart5) { Test({.Lang = LANG_RUS}, "custom", {10, 5}); }
    Y_UNIT_TEST(RussianCustomPart6) { Test({.Lang = LANG_RUS}, "custom", {10, 6}); }
    Y_UNIT_TEST(RussianCustomPart7) { Test({.Lang = LANG_RUS}, "custom", {10, 7}); }
    Y_UNIT_TEST(RussianCustomPart8) { Test({.Lang = LANG_RUS}, "custom", {10, 8}); }
    Y_UNIT_TEST(RussianCustomPart9) { Test({.Lang = LANG_RUS}, "custom", {10, 9}); }

    Y_UNIT_TEST(RussianTomPart0) { Test({.Lang = LANG_RUS}, "tom", {10, 0}); }
    Y_UNIT_TEST(RussianTomPart1) { Test({.Lang = LANG_RUS}, "tom", {10, 1}); }
    Y_UNIT_TEST(RussianTomPart2) { Test({.Lang = LANG_RUS}, "tom", {10, 2}); }
    Y_UNIT_TEST(RussianTomPart3) { Test({.Lang = LANG_RUS}, "tom", {10, 3}); }
    Y_UNIT_TEST(RussianTomPart4) { Test({.Lang = LANG_RUS}, "tom", {10, 4}); }
    Y_UNIT_TEST(RussianTomPart5) { Test({.Lang = LANG_RUS}, "tom", {10, 5}); }
    Y_UNIT_TEST(RussianTomPart6) { Test({.Lang = LANG_RUS}, "tom", {10, 6}); }
    Y_UNIT_TEST(RussianTomPart7) { Test({.Lang = LANG_RUS}, "tom", {10, 7}); }
    Y_UNIT_TEST(RussianTomPart8) { Test({.Lang = LANG_RUS}, "tom", {10, 8}); }
    Y_UNIT_TEST(RussianTomPart9) { Test({.Lang = LANG_RUS}, "tom", {10, 9}); }
}
