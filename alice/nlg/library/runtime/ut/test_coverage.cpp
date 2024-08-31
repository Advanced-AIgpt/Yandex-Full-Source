#include <alice/nlg/library/runtime/coverage.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NNlg;

Y_UNIT_TEST_SUITE(TCoverageTest) {
    Y_UNIT_TEST(NlgCoverageFilename) {
        TCoverage cov{"/some/path/%p.nlgcov", 123456};
        UNIT_ASSERT_STRINGS_EQUAL("/some/path/123456.nlgcov", cov.GetNlgCoverageFilename());
    }

    Y_UNIT_TEST(TwoModules) {
        TCoverage cov{"%p.nlgcov", 123};

        const TString module1 = "module1";
        const TString module2 = "module2";
        cov.RegisterModule(module1, {{0, 3}});
        cov.RegisterModule(module2, {{0, 2}});
        cov.IncCounter(module1, 0);
        cov.IncCounter(module1, 2);
        cov.IncCounter(module1, 0);
        cov.IncCounter(module2, 1);
        cov.IncCounter(module2, 0);
        cov.IncCounter(module2, 1);
        cov.IncCounter(module2, 1);

        const auto jsonLines = cov.ToJsonValues();
        UNIT_ASSERT_EQUAL(2, jsonLines.size());
        UNIT_ASSERT_STRINGS_EQUAL("{\"filename\":\"module2\",\"coverage\":{\"segments\":[[0,0,1,0,1],[1,0,2,0,3]]}}",
                                  jsonLines[0].GetStringRobust());
        UNIT_ASSERT_STRINGS_EQUAL(
            "{\"filename\":\"module1\",\"coverage\":{\"segments\":[[0,0,1,0,2],[1,0,2,0,0],[2,0,3,0,1]]}}",
            jsonLines[1].GetStringRobust());
    }

    Y_UNIT_TEST(ModuleWithNoncontiniousSegments) {
        TCoverage cov{"%p.nlgcov", 123};

        const TString module1 = "module1";
        cov.RegisterModule(module1, {{0, 2}, {4, 5}});
        cov.IncCounter(module1, 0);
        cov.IncCounter(module1, 1);

        const auto jsonLines = cov.ToJsonValues();
        UNIT_ASSERT_EQUAL(1, jsonLines.size());
        UNIT_ASSERT_STRINGS_EQUAL(
            "{\"filename\":\"module1\",\"coverage\":{\"segments\":[[0,0,1,0,1],[1,0,2,0,1],[4,0,5,0,0]]}}",
            jsonLines[0].GetStringRobust());
    }

    Y_UNIT_TEST(ModuleWithUnsortedSegments) {
        TCoverage cov{"%p.nlgcov", 123};

        const TString module1 = "module1";
        cov.RegisterModule(module1, {{1, 2}, {4, 5}, {0, 1}, {3, 4}});

        const auto jsonLines = cov.ToJsonValues();
        UNIT_ASSERT_EQUAL(1, jsonLines.size());
        UNIT_ASSERT_STRINGS_EQUAL(
            "{\"filename\":\"module1\",\"coverage\":{\"segments\":[[0,0,1,0,0],[1,0,2,0,0],[3,0,4,0,0],[4,0,5,0,0]]}}",
            jsonLines[0].GetStringRobust());
    }

    Y_UNIT_TEST(RegisterModuleTwice) {
        TCoverage cov{"%p.nlgcov", 123};

        const TString module1 = "module1";
        cov.RegisterModule(module1, {{0, 1}});
        // NOTE: all subsequent module1 registrations are ignored
        cov.RegisterModule(module1, {{10, 11}});

        cov.IncCounter(module1, 0);
        UNIT_ASSERT_EXCEPTION_CONTAINS(cov.IncCounter(module1, 10), yexception,
                                       "Segment for module1:10 is not registered");
    }

    Y_UNIT_TEST(IncSegmentInNonexistingModule) {
        TCoverage cov{"%p.nlgcov", 123};

        const TString module1 = "module1";
        cov.RegisterModule(module1, {{0, 1}});
        UNIT_ASSERT_EXCEPTION(cov.IncCounter("module2", 0), yexception);
    }

    Y_UNIT_TEST(IncNonexistingSegment) {
        TCoverage cov{"%p.nlgcov", 123};

        const TString module1 = "module1";
        cov.RegisterModule(module1, {{0, 1}});
        UNIT_ASSERT_EXCEPTION_CONTAINS(cov.IncCounter(module1, 42), yexception,
                                       "Segment for module1:42 is not registered");
    }
}
