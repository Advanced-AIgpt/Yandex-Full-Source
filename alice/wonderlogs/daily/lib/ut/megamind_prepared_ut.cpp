#include <alice/wonderlogs/daily/lib/megamind_prepared.h>
#include <alice/wonderlogs/daily/lib/ttls.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/resource/registry.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

#include <util/string/split.h>

using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(MegamindPrepared) {
    Y_UNIT_TEST(MegamindPreparedMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto uniproxyPrepared = CreateRandomTable(client, directory, "uniproxy-prepared");
        const auto megamindAnalyticsLogs = CreateRandomTable(client, directory, "megamind-analytics-logs");
        const auto megamindPreparedActual = CreateRandomTable(client, directory, "megamind-prepared");
        const auto megamindErrorActual = CreateRandomTable(client, directory, "megamind-error");

        const auto comparator = [](const auto& lhs, const auto& rhs) {
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            return lhs.GetMessageId() < rhs.GetMessageId();
        };

        {
            TVector<TUniproxyPrepared> rows;
            const auto rowsData = NResource::Find("uniproxy_prepared.jsonlines");
            for (const TStringBuf uniproxyPreparedJson : StringSplitter(rowsData).Split('\n')) {
                TUniproxyPrepared uniproxyPreparedRow;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(uniproxyPreparedJson), &uniproxyPreparedRow)
                        .ok());
                rows.push_back(uniproxyPreparedRow);
            }
            Sort(rows, comparator);
            auto writer = client->CreateTableWriter<TUniproxyPrepared>(
                NYT::TRichYPath(uniproxyPrepared)
                    .Schema(NYT::CreateTableSchema<TUniproxyPrepared>({"uuid", "message_id"})));

            for (const auto& row : rows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }
        {
            auto writer = client->CreateTableWriter<NYT::TNode>(NYT::TRichYPath(megamindAnalyticsLogs));
            const auto rows = NYT::NodeFromYsonString(NResource::Find("megamind_analytics_logs.yson"),
                                                      ::NYson::EYsonType::ListFragment);
            for (const auto& row : rows.AsList()) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        {
            const auto timestampFrom = ParseDatetime("2021-08-29T00:00:00+03:00");
            const auto timestampTo = ParseDatetime("2021-08-30T00:00:00+03:00");
            MakeMegamindPrepared(client, directory + "/tmp/", uniproxyPrepared, {megamindAnalyticsLogs},
                                 megamindPreparedActual, megamindErrorActual, *timestampFrom, *timestampTo);
        }

        {
            const THashMap<TString, int> sortColumns{{"uuid", 0}, {"message_id", 1}};
            int order = 0;
            NYT::TTableSchema schema;
            NYT::Deserialize(schema, client->Get(megamindPreparedActual + "/@schema"));
            for (const auto& col : schema.Columns()) {
                const auto* expectedOrder = sortColumns.FindPtr(col.Name());
                const auto& sortOrder = col.SortOrder();
                if (expectedOrder) {
                    UNIT_ASSERT_EQUAL(*expectedOrder, order);
                    ++order;
                    UNIT_ASSERT(sortOrder);
                    UNIT_ASSERT_EQUAL(NYT::ESortOrder::SO_ASCENDING, *sortOrder);
                } else {
                    UNIT_ASSERT(!sortOrder);
                }
            }
            UNIT_ASSERT_EQUAL(2, order);
        }

        for (const auto& table : {megamindPreparedActual, megamindErrorActual}) {
            const auto expirationTime = ParseDatetime(client->Get(table + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        const auto megamindPreparedRows = NResource::Find("megamind_prepared.jsonlines");
        TVector<TStringBuf> megamindPreparedSortedExpected = StringSplitter(megamindPreparedRows).Split('\n');

        TVector<TMegamindPrepared> megamindPreparedSortedActual;
        for (auto readerActual = client->CreateTableReader<TMegamindPrepared>(megamindPreparedActual);
             readerActual->IsValid(); readerActual->Next()) {
            megamindPreparedSortedActual.push_back(readerActual->GetRow());
        }
        UNIT_ASSERT_EQUAL_C(megamindPreparedSortedExpected.size(), megamindPreparedSortedActual.size(),
                            TStringBuilder{} << megamindPreparedSortedExpected.size() << " "
                                             << megamindPreparedSortedActual.size());
        Sort(megamindPreparedSortedActual, [](const TMegamindPrepared& lhs, const TMegamindPrepared& rhs) {
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            return lhs.GetSpeechkitRequest().GetHeader().GetRequestId() <
                   rhs.GetSpeechkitRequest().GetHeader().GetRequestId();
        });
        for (size_t i = 0; i < megamindPreparedSortedExpected.size(); i++) {
            TMegamindPrepared& actual = megamindPreparedSortedActual[i];
            TMegamindPrepared expected;
            // TODO(ran1s) MEGAMIND-3467
            google::protobuf::util::JsonParseOptions options;
            options.ignore_unknown_fields = true;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString(megamindPreparedSortedExpected[i]),
                                                                    &expected, options)
                            .ok());
            // the slots are strings in the proto message and the order of the serialized json is not determined
            for (auto* megamindPrepared : {&expected, &actual}) {
                for (auto& meta : *megamindPrepared->MutableSpeechkitResponse()->MutableResponse()->MutableMeta()) {
                    meta.MutableForm()->MutableSlots()->erase(meta.GetForm().GetSlots().begin(),
                                                              meta.GetForm().GetSlots().end());
                }
            }
            UNIT_ASSERT_EQUAL(expected.GetPresentInUniproxyLogs(), actual.GetPresentInUniproxyLogs());
            if (!expected.GetPresentInUniproxyLogs()) {
                expected.SetMessageId("");
                actual.SetMessageId("");
            }
            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }

        const auto megamindErrorRows = NResource::Find("megamind_error.jsonlines");
        TVector<TStringBuf> megamindErrorSortedExpected = StringSplitter(megamindErrorRows).Split('\n');
        while (!megamindErrorSortedExpected.empty() && megamindErrorSortedExpected.back().empty()) {
            megamindErrorSortedExpected.pop_back();
        }

        TVector<TMegamindPrepared::TError> megamindErrorSortedActual;
        for (auto readerActual = client->CreateTableReader<TMegamindPrepared::TError>(megamindErrorActual);
             readerActual->IsValid(); readerActual->Next()) {
            megamindErrorSortedActual.push_back(readerActual->GetRow());
        }

        UNIT_ASSERT_EQUAL_C(megamindErrorSortedExpected.size(), megamindErrorSortedActual.size(),
                            TStringBuilder{} << megamindErrorSortedExpected.size() << " "
                                             << megamindErrorSortedActual.size());
        Sort(megamindErrorSortedActual, comparator);
        for (size_t i = 0; i < megamindErrorSortedExpected.size(); i++) {
            TMegamindPrepared::TError& actual = megamindErrorSortedActual[i];
            TMegamindPrepared::TError expected;
            UNIT_ASSERT(
                google::protobuf::util::JsonStringToMessage(TString(megamindErrorSortedExpected[i]), &expected).ok());

            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }
    }
}
