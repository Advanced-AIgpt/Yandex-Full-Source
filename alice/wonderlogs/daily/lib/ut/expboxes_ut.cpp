#include <alice/wonderlogs/daily/lib/expboxes.h>
#include <alice/wonderlogs/daily/lib/ttls.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/client/protos/client_info.pb.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include <library/cpp/libgit2_wrapper/unidiff.h>
#include <library/cpp/resource/registry.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

#include <util/string/split.h>

using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(Expboxes) {
    Y_UNIT_TEST(ExpboxesMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto wonderlogs = CreateRandomTable(client, directory, "wonderlogs");
        const auto expboxesActual = CreateRandomTable(client, directory, "expboxes");
        const auto expboxesErrorActual = CreateRandomTable(client, directory, "expboxes-error");

        {
            TVector<TWonderlog> rows;
            const auto rowsData = NResource::Find("wonderlogs.jsonlines");
            for (const TStringBuf wonderlogJson : StringSplitter(rowsData).Split('\n')) {
                TWonderlog wonderlogRow;
                // TODO(ran1s) MEGAMIND-3467
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = true;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(wonderlogJson), &wonderlogRow, options).ok());
                rows.push_back(wonderlogRow);
            }
            Sort(rows, [](const TWonderlog& lhs, const TWonderlog& rhs) {
                if (lhs.GetUuid() != rhs.GetUuid()) {
                    return lhs.GetUuid() < rhs.GetUuid();
                }
                return lhs.GetMessageId() < rhs.GetMessageId();
            });
            auto writer = client->CreateTableWriter<TWonderlog>(
                NYT::TRichYPath(wonderlogs).Schema(NYT::CreateTableSchema<TWonderlog>({"_uuid", "_message_id"})));

            for (const auto& row : rows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        MakeExpboxes(client, directory + "/tmp/", wonderlogs, expboxesActual, expboxesErrorActual);

        for (const auto& table : {expboxesActual}) {
            UNIT_ASSERT_EQUAL("brotli_8", client->Get(table + "/@compression_codec").AsString());
            UNIT_ASSERT_EQUAL("lrc_12_2_2", client->Get(table + "/@erasure_codec").AsString());
            UNIT_ASSERT_EQUAL("scan", client->Get(table + "/@optimize_for").AsString());

            const THashMap<TString, int> sortColumns{{"uuid", 0}, {"message_id", 1}};
            int order = 0;
            NYT::TTableSchema schema;
            NYT::Deserialize(schema, client->Get(table + "/@schema"));
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

        UNIT_ASSERT(!client->Exists(expboxesActual + "/@expiration_time"));

        {
            const auto expirationTime =
                ParseDatetime(client->Get(expboxesErrorActual + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        const auto dialogsExpected =
            NYT::NodeFromYsonString(NResource::Find("expboxes.yson"), ::NYson::EYsonType::ListFragment);

        {
            const i64 expectedSize = dialogsExpected.AsList().size();
            const auto actualSize = client->Get(expboxesActual + "/@row_count").AsInt64();
            UNIT_ASSERT_VALUES_EQUAL(expectedSize, actualSize);
        }

        TVector<NYT::TNode> expboxesActualRows;
        TVector<NYT::TNode> expboxesExpectedRows;
        for (const auto& node : dialogsExpected.AsList()) {
            expboxesExpectedRows.push_back(node);
        }

        const auto comparator = [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
            if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                return lhs["uuid"].AsString() < rhs["uuid"].AsString();
            }
            return lhs["message_id"].AsString() < rhs["message_id"].AsString();
        };

        for (auto readerActual = client->CreateTableReader<NYT::TNode>(expboxesActual); readerActual->IsValid();
             readerActual->Next()) {
            expboxesActualRows.push_back(readerActual->GetRow());
        }

        Sort(expboxesActualRows, comparator);
        Sort(expboxesExpectedRows, comparator);

        for (size_t i = 0; i < expboxesActualRows.size(); ++i) {
            TString diff;
            TStringOutput out{diff};

            NLibgit2::UnifiedDiff(
                NYT::NodeToCanonicalYsonString(expboxesExpectedRows[i], NYson::EYsonFormat::Pretty) + '\n',
                NYT::NodeToCanonicalYsonString(expboxesActualRows[i], NYson::EYsonFormat::Pretty) + '\n', 3, out,
                /* colored= */ false);

            for (auto* dialog : {&expboxesExpectedRows[i], &expboxesActualRows[i]}) {
                dialog->AsMap().erase("message_id");
                dialog->AsMap().erase("session_id");
            }

            UNIT_ASSERT_EQUAL_C(expboxesExpectedRows[i], expboxesActualRows[i], diff);
        }

        const auto expboxesErrorExpected =
            NYT::NodeFromYsonString(NResource::Find("expboxes_error.yson"), ::NYson::EYsonType::ListFragment);

        UNIT_ASSERT_VALUES_EQUAL(static_cast<i64>((expboxesErrorExpected.AsList().size())),
                                 client->Get(expboxesErrorActual + "/@row_count").AsInt64());
        TVector<NYT::TNode> dialogsErrorSortedActual;

        for (auto readerActual = client->CreateTableReader<NYT::TNode>(expboxesErrorActual); readerActual->IsValid();
             readerActual->Next()) {
            auto& actual = readerActual->GetRow();
            dialogsErrorSortedActual.push_back(actual);
        }

        Sort(dialogsErrorSortedActual, [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
            if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                return lhs["uuid"].AsString() < rhs["uuid"].AsString();
            }
            return lhs["message_id"].AsString() < rhs["message_id"].AsString();
        });

        for (size_t i = 0; i < expboxesErrorExpected.AsList().size(); i++) {
            UNIT_ASSERT_EQUAL(dialogsErrorSortedActual[i], expboxesErrorExpected.AsList()[i]);
        }
    }
}
