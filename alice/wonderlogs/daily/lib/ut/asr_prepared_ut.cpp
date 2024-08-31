#include <alice/wonderlogs/daily/lib/asr_prepared.h>
#include <alice/wonderlogs/daily/lib/ttls.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/asr_prepared.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/resource/registry.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

#include <util/string/split.h>

using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(AsrPrepared) {
    Y_UNIT_TEST(AsrPreparedMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto uniproxyPrepared = CreateRandomTable(client, directory, "uniproxy-prepared");
        const auto asrLogs = CreateRandomTable(client, directory, "asr-logs");
        const auto asrPreparedActual = CreateRandomTable(client, directory, "asr-prepared");
        const auto asrErrorActual = CreateRandomTable(client, directory, "asr-error");

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
            auto writer = client->CreateTableWriter<NYT::TNode>(NYT::TRichYPath(asrLogs));
            const auto rows =
                NYT::NodeFromYsonString(NResource::Find("asr_logs.yson"), ::NYson::EYsonType::ListFragment);
            for (const auto& row : rows.AsList()) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        {
            const auto timestampFrom = ParseDatetime("2021-08-29T00:00:00+03:00");
            const auto timestampTo = ParseDatetime("2021-08-30T00:00:00+03:00");
            MakeAsrPrepared(client, directory + "/tmp/", uniproxyPrepared, {asrLogs}, asrPreparedActual,
                            asrErrorActual, *timestampFrom, *timestampTo, /* requestsShift= */ TDuration::Minutes(10));
        }

        {
            const THashMap<TString, int> sortColumns{{"uuid", 0}, {"message_id", 1}};
            int order = 0;
            NYT::TTableSchema schema;
            NYT::Deserialize(schema, client->Get(asrPreparedActual + "/@schema"));
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

        for (const auto& table : {asrPreparedActual, asrErrorActual}) {
            const auto expirationTime = ParseDatetime(client->Get(table + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        const auto asrPreparedRows = NResource::Find("asr_prepared.jsonlines");
        TVector<TStringBuf> asrPreparedSortedExpected = StringSplitter(asrPreparedRows).Split('\n');

        TVector<TAsrPrepared> asrPreparedSortedActual;
        for (auto readerActual = client->CreateTableReader<TAsrPrepared>(asrPreparedActual); readerActual->IsValid();
             readerActual->Next()) {
            asrPreparedSortedActual.push_back(readerActual->GetRow());
        }
        UNIT_ASSERT_EQUAL_C(asrPreparedSortedExpected.size(), asrPreparedSortedActual.size(),
                            TStringBuilder{} << asrPreparedSortedExpected.size() << " "
                                             << asrPreparedSortedActual.size());
        Sort(asrPreparedSortedActual, comparator);
        for (size_t i = 0; i < asrPreparedSortedExpected.size(); i++) {
            TAsrPrepared& actual = asrPreparedSortedActual[i];
            TAsrPrepared expected;
            UNIT_ASSERT(
                google::protobuf::util::JsonStringToMessage(TString(asrPreparedSortedExpected[i]), &expected).ok());

            UNIT_ASSERT_EQUAL(expected.GetPresentInUniproxyLogs(), actual.GetPresentInUniproxyLogs());
            if (!expected.GetPresentInUniproxyLogs()) {
                expected.SetMessageId("");
                actual.SetMessageId("");
            }
            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }

        const auto asrErrorRows = NResource::Find("asr_error.jsonlines");
        TVector<TStringBuf> asrErrorSortedExpected = StringSplitter(asrErrorRows).Split('\n');
        while (!asrErrorSortedExpected.empty() && asrErrorSortedExpected.back().empty()) {
            asrErrorSortedExpected.pop_back();
        }

        TVector<TAsrPrepared::TError> asrErrorSortedActual;
        for (auto readerActual = client->CreateTableReader<TAsrPrepared::TError>(asrErrorActual);
             readerActual->IsValid(); readerActual->Next()) {
            asrErrorSortedActual.push_back(readerActual->GetRow());
        }
        UNIT_ASSERT_EQUAL_C(asrErrorSortedExpected.size(), asrErrorSortedActual.size(),
                            TStringBuilder{} << asrErrorSortedExpected.size() << " " << asrErrorSortedActual.size());
        Sort(asrErrorSortedActual, [](const TAsrPrepared::TError& lhs, const TAsrPrepared::TError& rhs) {
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            return lhs.GetMessageId() < rhs.GetMessageId();
        });
        for (size_t i = 0; i < asrErrorSortedExpected.size(); i++) {
            TAsrPrepared::TError& actual = asrErrorSortedActual[i];
            TAsrPrepared::TError expected;
            UNIT_ASSERT(
                google::protobuf::util::JsonStringToMessage(TString(asrErrorSortedExpected[i]), &expected).ok());

            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }
    }
}
