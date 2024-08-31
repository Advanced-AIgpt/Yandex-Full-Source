#include "alice/hollywood/library/fast_data/fast_data.h"
#include "alice/hollywood/library/framework/unittest/test_environment.h"
#include "news_fast_data.h"
#include "render_handle.h"
#include "gtest/gtest.h"

#include <alice/hollywood/library/scenarios/news/nlg/register.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/framework/framework_ut.h>
#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/protos/api/renderer/api.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <catboost/private/libs/data_types/pair.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <memory>
#include <util/folder/path.h>
#include <util/generic/algorithm.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/fwd.h>
#include <util/string/builder.h>
#include <util/system/env.h>
#include <util/system/file.h>

using namespace testing;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf SUFFIX_RUN_REQUEST = "_run_request.pb.txt";
constexpr TStringBuf SUFFIX_RUN_RESPONSE = "_run_response.pb.txt";
constexpr TStringBuf SUFFIX_BASS_RESPONSE = "_bass_response.json";
constexpr TStringBuf RENDER_DATA = "_render_data.json";
const TVector<TSmi> SMI_COLLECTION {
            {"РБК", "РБК", "rbc", "abcdef#rbc", "1", "rbc.ru", true,  "rbk_logo.png"},
            {"Медуза", "Медузу", "meduza", "fedcba#medusa", "2", "meduza.ru", true, "meduza_logo.png"}};

TFsPath GetDataPath() {
    return TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/library/scenarios/news/ut_render/data";
}

void Register(NAlice::NNlg::TEnvironment& env) {
    NAlice::NHollywood::NLibrary::NScenarios::NNews::NNlg::RegisterAll(env);
}

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture()
        : Nlg_{Rng_, nullptr, &Register}
        , Ctx_{GlobalCtx_, TRTLogger::NullLogger(), /* scenarioResources= */ nullptr, &Nlg_}
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
        NHollywoodFw::TTestEnvironment testData("news", "ru-ru");
        const auto& runRequestFilename = testCase + SUFFIX_RUN_REQUEST;
        const auto& bassResponseFilename = testCase + SUFFIX_BASS_RESPONSE;
        const auto& expectedRunResponseFilename = testCase + SUFFIX_RUN_RESPONSE;
        const auto& expectedRenderDataFilename = testCase + RENDER_DATA;
        testData.AttachFastdata(std::shared_ptr<IFastData>(new TNewsFastData(SMI_COLLECTION)));

        const auto runRequestStr = TFileInput(GetDataPath() / runRequestFilename).ReadAll();
        testData.RunRequest = ParseProtoText<NScenarios::TScenarioRunRequest>(runRequestStr);
        NAppHost::NService::TTestContext serviceCtx;
        const TScenarioRunRequestWrapper request(testData.RunRequest, serviceCtx);

        const auto bassResponseStr = TFileInput(GetDataPath() / bassResponseFilename).ReadAll();
        NJson::TJsonValue bassResponseJson = JsonFromString(bassResponseStr);

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(Ctx_.Nlg(), request, Rng_, ELanguage::LANG_RUS);
        TRunResponseBuilder builder(&nlgWrapper);

        std::unique_ptr<NScenarios::TScenarioRunResponse> runResponse;
        TRng rng{12345};
        try {
            runResponse = NImpl::NewsRenderDoImpl(request, testData.CreateRunRequest(), bassResponseJson, Ctx_, builder, rng);
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

        const auto& expectedRenderDataFilePath = GetDataPath() / expectedRenderDataFilename;
        if (expectedRenderDataFilePath.Exists()) {
            NJson::TJsonValue renderData;
            for (auto const& [cardId, cardData] : builder.GetResponseBodyBuilder()->GetRenderData()) {
                NJson::TJsonValue item;
                item.InsertValue("key", cardId);
                item.InsertValue("value", ProtoToJsonString(cardData));
                renderData.AppendValue(item);
            }
            try {
                const auto& expectedRenderData = JsonFromString(TFileInput(GetDataPath() / expectedRenderDataFilename).ReadAll());
                UNIT_ASSERT_EQUAL(expectedRenderData, renderData);
            } catch (...) {
                outputDiff << "ERROR: " << testCase << ": expected render data is different" << Endl;
                if (overwriteExpectedFiles) {
                    TFileOutput renderDataFile{TFile{expectedRenderDataFilePath, CreateAlways}};
                    renderDataFile.Write(JsonToString(renderData, /* validateUtf8 */ true, /* formatOutput */ true));
                    outputDiff << "ATTENTION: File " << expectedRenderDataFilePath << " overwritten!" << Endl;
                }
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
            passedCount += isPassed ? 1 : 0;
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
    NAppHost::NService::TTestContext ServiceContext_;
};

} // namespace

Y_UNIT_TEST_SUITE_F(NewsRenderTests, TFixture) {
    Y_UNIT_TEST(TestCasesCheck) {
        Test();
    }
}

} // namespace NAlice::NHollywood
