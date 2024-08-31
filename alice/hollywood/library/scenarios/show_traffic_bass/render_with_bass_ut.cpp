# include "render_with_bass.h"

#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/hollywood/library/scenarios/show_traffic_bass/nlg/register.h>
#include <alice/hollywood/library/scenarios/show_traffic_bass/proto/show_traffic_bass.pb.h>
#include <alice/hollywood/library/scenarios/show_traffic_bass/renderer.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/generic/set.h>
#include <util/stream/file.h>

using namespace testing;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf SUFFIX_RUN_REQUEST = "_run_request.pb.txt";
constexpr TStringBuf SUFFIX_RUN_RESPONSE = "_run_response.pb.txt";
constexpr TStringBuf SUFFIX_BASS_RESPONSE = "_bass_response.json";

TFsPath GetDataPath() {
    return TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/library/scenarios/show_traffic_bass/ut/data";
}

void Register(NAlice::NNlg::TEnvironment& env) {
    NAlice::NHollywood::NLibrary::NScenarios::NShowTrafficBass::NNlg::RegisterAll(env);
}

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture()
        : Nlg_{Rng_, nullptr, &Register}
        , Ctx_{GlobalCtx_, TRTLogger::StderrLogger(), /* scenarioResources= */ nullptr, &Nlg_}
    {
        EXPECT_CALL(GlobalCtx_, Sensors()).WillRepeatedly(testing::ReturnRef(Sensors_));

        TVector<TString> dataFilenames;
        GetDataPath().ListNames(dataFilenames);
        Sort(dataFilenames.begin(), dataFilenames.end());
        for (const auto& dataFilename : dataFilenames) {
            const auto& suffixStart = dataFilename.rfind(SUFFIX_RUN_REQUEST);
            if (suffixStart == TString::npos) {
                continue;
            }
            TestCases_.emplace(TString(dataFilename, 0, suffixStart));
        }
    }

    // Pass 'overwriteExpectedFiles = true' if you want to save actual results as expected ones.
    bool TestDoImplWithData(TString testCase, TStringBuilder& outputDiff, bool overwriteExpectedFiles) {
        const auto& runRequestFilename = testCase + SUFFIX_RUN_REQUEST;
        const auto& bassResponseFilename = testCase + SUFFIX_BASS_RESPONSE;
        const auto& expectedRunResponseFilename = testCase + SUFFIX_RUN_RESPONSE;
        const auto runRequestStr = TFileInput(GetDataPath() / runRequestFilename).ReadAll();
        const auto runRequest = ParseProtoText<NScenarios::TScenarioRunRequest>(runRequestStr);
        NAppHost::NService::TTestContext serviceCtx;
        const TScenarioRunRequestWrapper request(runRequest, serviceCtx);
        const NJson::TJsonValue appHostParams;
        TScenarioHandleContext handlectx{serviceCtx, Meta_, Ctx_, Rng_, ELanguage::LANG_RUS, ELanguage::LANG_RUS, appHostParams, nullptr};

        const auto bassResponseStr = TFileInput(GetDataPath() / bassResponseFilename).ReadAll();
        NJson::TJsonValue bassResponseJson = JsonFromString(bassResponseStr);

        std::unique_ptr<NScenarios::TScenarioRunResponse> runResponse;

        NShowTrafficBass::TRenderer renderer{handlectx, request};
        try {
            runResponse = NImpl::BassShowTrafficRenderDoImpl(request, bassResponseJson, Ctx_, renderer);
        } catch (...) {
            outputDiff << "ERROR: '" << testCase << "': " << CurrentExceptionMessage() << Endl;
            return false;
        }

        UNIT_ASSERT(!runResponse->GetVersion().Empty());
        runResponse->SetVersion("trunk@TEST"); // For the sake of reproducible tests we overwrite the svn commit id in the Version

        TMaybe<NScenarios::TScenarioRunResponse> runResponseExpected;
        const auto& runResponseFilePath = GetDataPath() / expectedRunResponseFilename;
        if (runResponseFilePath.Exists()) {
            const auto runResponseExpectedStr = TFileInput(runResponseFilePath).ReadAll();
            try {
                runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
            } catch (const yexception& e) {
                outputDiff << "ERROR: " << runResponseFilePath << ": " << e.what() << Endl;
                runResponseExpected = Nothing();
            }
        }

        TMessageDiff diff(runResponseExpected.GetOrElse(NScenarios::TScenarioRunResponse{}), *runResponse);
        if (!diff.AreEqual) {
            outputDiff << "FAIL: '" << testCase << "':\n" << diff.FullDiff;

            if (overwriteExpectedFiles) {
                TFileOutput runResponseFile{TFile{runResponseFilePath, CreateAlways}};
                runResponseFile.Write(SerializeProtoText(*runResponse, /* singleLineMode = */ false));
                outputDiff << "ATTENTION: File " << runResponseFilePath << " overwritten!" << Endl;
            }
            return false;
        }
        return true;
    }

    void Test() {
        ui32 passedCount = 0;
        ui32 skippedCount = 0;
        TStringBuilder outputDiff;

        const auto overwriteExpectedFiles = FromString<int>(GetEnv("TEST_OVERWRITE_EXPECTED_FILES", "0"));
        for (const auto& testCase : TestCases_) {
            const auto isPassed = TestDoImplWithData(testCase, outputDiff, overwriteExpectedFiles);
            passedCount += int(isPassed);
        }
        TStringBuilder report;
        report << Endl << "Total " << TestCases_.size() << " tests:" << Endl;
        report << "    " << passedCount << " PASSED" << Endl;
        report << "    " << skippedCount << " SKIPPED" << Endl;
        report << "    " << TestCases_.size() - passedCount - skippedCount << " FAILED" << Endl;
        UNIT_ASSERT_EQUAL_C(passedCount + skippedCount, TestCases_.size(), report << "Details:" << Endl << outputDiff);
    }

    NAlice::TFakeRng Rng_;
    TCompiledNlgComponent Nlg_;
    TMockGlobalContext GlobalCtx_;
    TMockSensors Sensors_;
    TContext Ctx_;
    NScenarios::TRequestMeta Meta_;
    TSet<TString> TestCases_;
};

} // namespace

Y_UNIT_TEST_SUITE_F(ShowTrafficBassRender, TFixture) {
    Y_UNIT_TEST(Render) {
        Test();
    }
}

} // namespace NAlice::NHollywood
