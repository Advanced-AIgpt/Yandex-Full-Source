#include <alice/wonderlogs/daily/lib/differ.h>

#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>
#include <alice/wonderlogs/protos/wonderlogs_diff.pb.h>

#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/field_differ/lib/field_differ.h>
#include <alice/library/field_differ/protos/differ_report.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

namespace NAlice::NWonderlogs {

namespace {

const TString DIFF_SERVER_TIME_MS = R"(
Diffs {
    Path: "ServerTimeMs"
    FirstValue: "1337"
    ImportantFieldCheck: IFC_PRESENCE
}
)";

const TString DIFF_PRESENCE = R"(
Diffs {
    Path: "Presence.Asr"
    FirstValue: "1"
    ImportantFieldCheck: IFC_DIFF
}
)";

const TString WONDERLOG_PRESENCE_LHS = R"(
Presence {
    Uniproxy: true
    Megamind: true
    Asr: true
    UniproxyPresence {
        MegamindRequest: true
        MegamindResponse: true
        RequestStat: true
        SpotterValidation: true
        SpotterStream: true
        Stream: true
        LogSpotter: true
        VoiceInput: true
        AsrRecognize: true
        AsrResult: true
        SynchronizeState: true
        MegamindTimings: true
        TtsTimings: false
    }
}
)";

const TString WONDERLOG_PRESENCE_RHS = R"(
Presence {
    Uniproxy: true
    Megamind: true
    UniproxyPresence {
        MegamindRequest: false
        MegamindResponse: true
        RequestStat: true
        SpotterValidation: true
        SpotterStream: true
        Stream: true
        LogSpotter: true
        VoiceInput: true
        AsrRecognize: true
        AsrResult: true
        SynchronizeState: true
        MegamindTimings: true
        TtsTimings: false
        TtsGenerate: false
    }
}
)";

Y_UNIT_TEST_SUITE(Differ) {
    Y_UNIT_TEST(ContainsComplexNode) {
        {
            NYT::TNode node;
            node["a"]["b"]["c"] = 1;
            UNIT_ASSERT(NImpl::ContainsComplexNode(node));
            UNIT_ASSERT(NImpl::ContainsComplexNode(node["a"]));
            UNIT_ASSERT(NImpl::ContainsComplexNode(node["a"]["b"]));
            UNIT_ASSERT(!NImpl::ContainsComplexNode(node["a"]["b"]["c"]));
        }
        {
            NYT::TNode node;
            node["a"]["b"].Add(1);
            node["a"]["b"].Add(2);
            node["a"]["b"].Add("lol");

            {
                NYT::TNode nestedNode;
                nestedNode["a"] = 1;
                node["a"]["c"].Add(nestedNode);
            }

            {
                NYT::TNode nestedNode;
                nestedNode.Add(1);
                node["a"]["d"].Add(nestedNode);
            }
            UNIT_ASSERT(NImpl::ContainsComplexNode(node));
            UNIT_ASSERT(NImpl::ContainsComplexNode(node["a"]));
            UNIT_ASSERT(!NImpl::ContainsComplexNode(node["a"]["b"]));
            UNIT_ASSERT(NImpl::ContainsComplexNode(node["a"]["c"]));
            UNIT_ASSERT(NImpl::ContainsComplexNode(node["a"]["c"][0]));
            UNIT_ASSERT(!NImpl::ContainsComplexNode(node["a"]["c"][0]["a"]));
            UNIT_ASSERT(NImpl::ContainsComplexNode(node["a"]["d"]));
            UNIT_ASSERT(!NImpl::ContainsComplexNode(node["a"]["d"][0]));
        }
    }

    Y_UNIT_TEST(GetChangedFieldsNoDiff) {
        NYT::TNode stable;
        stable["a"]["b"]["c"] = 1;
        NYT::TNode test;
        test["a"]["b"]["c"] = 2;
        TVector<TString> changedFields;
        NImpl::GetChangedFields(stable, test, /* path= */ "", changedFields);
        UNIT_ASSERT(changedFields.empty());
    }

    Y_UNIT_TEST(GetChangedFieldsMapDiff) {
        NYT::TNode stable;
        stable["a"]["b"]["c"] = 1;
        NYT::TNode test;
        test["a"]["b"]["d"] = 2;
        TVector<TString> changedFields;
        NImpl::GetChangedFields(stable, test, /* path= */ "", changedFields);
        UNIT_ASSERT_EQUAL(TVector<TString>{"a.b.c"}, changedFields);
    }

    Y_UNIT_TEST(GetChangedFieldsNoDiffList) {
        NYT::TNode stable;
        stable["a"]["b"]["c"].Add("a");
        stable["a"]["b"]["c"].Add("b");
        stable["a"]["b"]["c"].Add("c");
        NYT::TNode test;
        test["a"]["b"]["c"].Add("a");
        test["a"]["b"]["c"].Add("b");
        test["a"]["b"]["c"].Add("c");
        TVector<TString> changedFields;
        NImpl::GetChangedFields(stable, test, /* path= */ "", changedFields);
        UNIT_ASSERT(changedFields.empty());
    }

    Y_UNIT_TEST(GetChangedFieldsNoDiffMap) {
        NYT::TNode stable;
        stable["a"]["b"]["c"]["a"] = 1;
        stable["a"]["b"]["c"]["b"] = 2;
        stable["a"]["b"]["c"]["c"] = 3;
        NYT::TNode test;
        test["a"]["b"]["c"]["a"] = 1;
        test["a"]["b"]["c"]["b"] = 1;
        test["a"]["b"]["c"]["c"] = 1;
        TVector<TString> changedFields;
        NImpl::GetChangedFields(stable, test, /* path= */ "", changedFields);
        UNIT_ASSERT(changedFields.empty());
    }

    Y_UNIT_TEST(GetChangedFieldsNoDiffMapNewFields) {
        NYT::TNode stable;
        stable["a"] = 1;
        stable["b"] = 2;
        stable["c"] = 3;
        NYT::TNode test;
        test["a"] = 1;
        test["b"] = 2;
        test["c"] = 3;
        test["d"] = 4;
        TVector<TString> changedFields;
        NImpl::GetChangedFields(stable, test, /* path= */ "", changedFields);
        UNIT_ASSERT(changedFields.empty());
    }

    Y_UNIT_TEST(GetChangedFieldsDiffMapOldFields) {
        NYT::TNode stable;
        stable["a"] = 1;
        stable["b"] = 2;
        stable["c"] = 3;
        stable["d"] = 4;
        NYT::TNode test;
        test["a"] = 1;
        test["b"] = 2;
        test["c"] = 3;
        TVector<TString> changedFields;
        NImpl::GetChangedFields(stable, test, /* path= */ "", changedFields);
        UNIT_ASSERT_EQUAL(TVector<TString>{"d"}, changedFields);
    }

    Y_UNIT_TEST(MakeWonderlogsDiff) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto stable = CreateRandomTable(client, directory, "stable");
        const auto test = CreateRandomTable(client, directory, "test");
        const auto diff = CreateRandomTable(client, directory, "diff");

        TVector<TWonderlogsDiff> differReportSortedExpected(2);
        differReportSortedExpected[0].SetMegamindRequestId("with_different_product_scenario_name");
        {
            auto& diffValue = *differReportSortedExpected[0].MutableDiff()->AddDiffs();
            diffValue.SetPath("SpeechkitResponse.MegamindAnalyticsInfo.AnalyticsInfo.value.ScenarioAnalyticsInfo."
                              "ProductScenarioName");
            diffValue.SetFirstValue("lol");
            diffValue.SetImportantFieldCheck(NAlice::EImportantFieldCheck::IFC_PRESENCE);
        }
        differReportSortedExpected[1].SetMegamindRequestId("with_same_product_scenario_name");
        differReportSortedExpected[1].MutableDiff();
        {
            auto writer = client->CreateTableWriter<TWonderlog>(stable, NYT::TTableWriterOptions().InferSchema(true));
            {
                TWonderlog wonderlog;
                wonderlog.SetUuid("uuid");
                wonderlog.SetMegamindRequestId("with_different_product_scenario_name");
                (*wonderlog.MutableSpeechkitResponse()->MutableMegamindAnalyticsInfo()->MutableAnalyticsInfo())["lol"]
                    .MutableScenarioAnalyticsInfo()
                    ->SetProductScenarioName("lol");
                writer->AddRow(wonderlog);
                differReportSortedExpected[0].SetStableWonderlog(NAlice::JsonStringFromProto(wonderlog));
            }
            {
                TWonderlog wonderlog;
                wonderlog.SetUuid("uuid");
                wonderlog.SetMegamindRequestId("with_same_product_scenario_name");
                (*wonderlog.MutableSpeechkitResponse()->MutableMegamindAnalyticsInfo()->MutableAnalyticsInfo())["lol"]
                    .MutableScenarioAnalyticsInfo()
                    ->SetProductScenarioName("lol");
                writer->AddRow(wonderlog);
                differReportSortedExpected[1].SetStableWonderlog(NAlice::JsonStringFromProto(wonderlog));
            }
            writer->Finish();
        }
        {
            auto writer = client->CreateTableWriter<TWonderlog>(test, NYT::TTableWriterOptions().InferSchema(true));
            {
                TWonderlog wonderlog;
                wonderlog.SetUuid("uuid");
                wonderlog.SetMegamindRequestId("with_different_product_scenario_name");
                (*wonderlog.MutableSpeechkitResponse()->MutableMegamindAnalyticsInfo()->MutableAnalyticsInfo())["lol"]
                    .MutableScenarioAnalyticsInfo();
                writer->AddRow(wonderlog);
                differReportSortedExpected[0].SetTestWonderlog(NAlice::JsonStringFromProto(wonderlog));
            }
            {
                TWonderlog wonderlog;
                wonderlog.SetUuid("uuid");
                wonderlog.SetMegamindRequestId("with_same_product_scenario_name");
                (*wonderlog.MutableSpeechkitResponse()->MutableMegamindAnalyticsInfo()->MutableAnalyticsInfo())["lol"]
                    .MutableScenarioAnalyticsInfo()
                    ->SetProductScenarioName("kek");
                writer->AddRow(wonderlog);
                differReportSortedExpected[1].SetTestWonderlog(NAlice::JsonStringFromProto(wonderlog));
            }
            writer->Finish();
        }

        MakeWonderlogsDiff(client, directory + "/tmp/", stable, test, diff);

        TVector<TWonderlogsDiff> differReportSortedActual;
        for (auto readerActual = client->CreateTableReader<TWonderlogsDiff>(diff); readerActual->IsValid();
             readerActual->Next()) {
            differReportSortedActual.push_back(readerActual->GetRow());
        }

        UNIT_ASSERT_EQUAL_C(differReportSortedExpected.size(), differReportSortedActual.size(),
                            TStringBuilder{} << differReportSortedExpected.size() << " "
                                             << differReportSortedActual.size());

        const auto cmp = [](const TWonderlogsDiff& lhs, const TWonderlogsDiff& rhs) {
            return lhs.GetMegamindRequestId() < rhs.GetMegamindRequestId();
        };
        Sort(differReportSortedExpected, cmp);
        Sort(differReportSortedActual, cmp);

        for (size_t i = 0; i < differReportSortedExpected.size(); i++) {
            UNIT_ASSERT_MESSAGES_EQUAL_C(differReportSortedExpected[i], differReportSortedActual[i],
                                         TStringBuilder{} << differReportSortedExpected[i].GetUuid() << " "
                                                          << differReportSortedExpected[i].GetMessageId());
        }
    }

    Y_UNIT_TEST(CheckFieldDiff) {
        {
            NAlice::TDifferReport expected;
            TWonderlog lhs, rhs;
            lhs.SetServerTimeMs(1337);
            rhs.SetServerTimeMs(0);

            NAlice::TFieldDiffer fieldDiffer;

            const auto actual = fieldDiffer.FindDiffs(lhs, rhs);

            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }

        {
            NAlice::TDifferReport expected;
            google::protobuf::TextFormat::ParseFromString(DIFF_SERVER_TIME_MS, &expected);
            TWonderlog lhs, rhs;
            lhs.SetServerTimeMs(1337);

            NAlice::TFieldDiffer fieldDiffer;

            const auto actual = fieldDiffer.FindDiffs(lhs, rhs);

            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(CheckFieldDiffWonderlogPresence) {
        NAlice::TDifferReport expected;
        google::protobuf::TextFormat::ParseFromString(DIFF_PRESENCE, &expected);

        TWonderlog lhs, rhs;
        google::protobuf::TextFormat::ParseFromString(WONDERLOG_PRESENCE_LHS, &lhs);
        google::protobuf::TextFormat::ParseFromString(WONDERLOG_PRESENCE_RHS, &rhs);

        NAlice::TFieldDiffer fieldDiffer;
        const auto actual = fieldDiffer.FindDiffs(lhs, rhs);

        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }
}

} // namespace

} // namespace NAlice::NWonderlogs
