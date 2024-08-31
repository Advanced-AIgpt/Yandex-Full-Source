#include <alice/wonderlogs/daily/lib/dialogs.h>
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

Y_UNIT_TEST_SUITE(Dialogs) {
    Y_UNIT_TEST(DialogsMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto wonderlogs = CreateRandomTable(client, directory, "wonderlogs");
        const auto dialogsActual = CreateRandomTable(client, directory, "dialogs");
        const auto bannedDialogsActual = CreateRandomTable(client, directory, "banned-dialogs");
        const auto dialogsErrorActual = CreateRandomTable(client, directory, "dialogs-error");

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

        TBannedUsers bannedUsers;
        bannedUsers.Ips.insert("85.249.43.239");
        bannedUsers.Ips.insert("176.52.100.47");
        bannedUsers.Uuids.insert("46dd2ac67e94be515c5a969203a5aa91");
        bannedUsers.Uuids.insert("9af274afc18c4bca8873f93848b6df9d");
        bannedUsers.DeviceIds.insert("FF98F02911290D005664AFEF");
        bannedUsers.DeviceIds.insert("04107896830808160a90");

        MakeDialogs(client, directory + "/tmp/", wonderlogs, dialogsActual, bannedDialogsActual, dialogsErrorActual,
                    bannedUsers, /* productionEnvironment= */ {});

        for (const auto& table : {dialogsActual, bannedDialogsActual}) {
            UNIT_ASSERT_EQUAL("brotli_8", client->Get(table + "/@compression_codec").AsString());
            UNIT_ASSERT_EQUAL("lrc_12_2_2", client->Get(table + "/@erasure_codec").AsString());
            UNIT_ASSERT_EQUAL("scan", client->Get(table + "/@optimize_for").AsString());

            const THashMap<TString, int> sortColumns{{"server_time_ms", 0}, {"uuid", 1}, {"sequence_number", 2}};
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
            UNIT_ASSERT_EQUAL(3, order);
        }

        UNIT_ASSERT(!client->Exists(dialogsActual + "/@expiration_time"));

        {
            const auto expirationTime =
                ParseDatetime(client->Get(dialogsErrorActual + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        const auto dialogsExpected =
            NYT::NodeFromYsonString(NResource::Find("dialogs.yson"), ::NYson::EYsonType::ListFragment);

        {
            const i64 expectedSize = dialogsExpected.AsList().size();
            const auto actualSize = client->Get(dialogsActual + "/@row_count").AsInt64();
            UNIT_ASSERT_VALUES_EQUAL(expectedSize, actualSize);
        }

        TVector<NYT::TNode> dialogsActualRows;
        TVector<NYT::TNode> dialogsExpectedRows;
        for (const auto& node : dialogsExpected.AsList()) {
            dialogsExpectedRows.push_back(node);
        }

        const auto comparator = [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
            if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                return lhs["uuid"].AsString() < rhs["uuid"].AsString();
            }
            return lhs["request_id"].AsString() < rhs["request_id"].AsString();
        };

        for (auto readerActual = client->CreateTableReader<NYT::TNode>(dialogsActual); readerActual->IsValid();
             readerActual->Next()) {
            dialogsActualRows.push_back(readerActual->GetRow());
        }

        Sort(dialogsActualRows, comparator);
        Sort(dialogsExpectedRows, comparator);

        for (size_t i = 0; i < dialogsActualRows.size(); ++i) {
            TString diff;
            TStringOutput out{diff};

            NLibgit2::UnifiedDiff(
                NYT::NodeToCanonicalYsonString(dialogsExpectedRows[i], NYson::EYsonFormat::Pretty) + '\n',
                NYT::NodeToCanonicalYsonString(dialogsActualRows[i], NYson::EYsonFormat::Pretty) + '\n', 3, out,
                /* colored= */ false);

            UNIT_ASSERT_EQUAL_C(dialogsExpectedRows[i], dialogsActualRows[i], diff);
        }

        const auto dialogsErrorExpected =
            NYT::NodeFromYsonString(NResource::Find("dialogs_error.yson"), ::NYson::EYsonType::ListFragment);

        UNIT_ASSERT_VALUES_EQUAL(static_cast<i64>((dialogsErrorExpected.AsList().size())),
                                 client->Get(dialogsErrorActual + "/@row_count").AsInt64());
        TVector<NYT::TNode> dialogsErrorSortedActual;

        for (auto readerActual = client->CreateTableReader<NYT::TNode>(dialogsErrorActual); readerActual->IsValid();
             readerActual->Next()) {
            auto& actual = readerActual->GetRow();
            dialogsErrorSortedActual.push_back(actual);
        }

        Sort(dialogsErrorSortedActual, [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
            if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                return lhs["uuid"].AsString() < rhs["uuid"].AsString();
            }
            return lhs["request_id"].AsString() < rhs["request_id"].AsString();
        });

        for (size_t i = 0; i < dialogsErrorExpected.AsList().size(); i++) {
            UNIT_ASSERT_EQUAL(dialogsErrorSortedActual[i], dialogsErrorExpected.AsList()[i]);
        }
    }

    Y_UNIT_TEST(RobotDialogsMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto wonderlogs = CreateRandomTable(client, directory, "wonderlogs");
        const auto dialogsActual = CreateRandomTable(client, directory, "dialogs");
        const auto bannedDialogsActual = CreateRandomTable(client, directory, "banned-dialogs");
        const auto dialogsErrorActual = CreateRandomTable(client, directory, "dialogs-error");

        {
            auto writer =
                client->CreateTableWriter<TWonderlog>(wonderlogs, NYT::TTableWriterOptions().InferSchema(true));
            const auto rows = NResource::Find("robot_wonderlogs.jsonlines");
            for (const TStringBuf wonderlogJson : StringSplitter(rows).Split('\n')) {
                TWonderlog wonderlogRow;
                // TODO(ran1s) MEGAMIND-3467
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = true;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(wonderlogJson), &wonderlogRow, options).ok());
                writer->AddRow(wonderlogRow);
            }
            writer->Finish();
        }

        TBannedUsers bannedUsers;

        MakeDialogs(client, directory + "/tmp/", wonderlogs, dialogsActual, bannedDialogsActual, dialogsErrorActual,
                    bannedUsers, /* productionEnvironment= */ {});

        UNIT_ASSERT(!client->Exists(dialogsActual + "/@expiration_time"));

        {
            const auto expirationTime =
                ParseDatetime(client->Get(dialogsErrorActual + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        const auto dialogsExpected =
            NYT::NodeFromYsonString(NResource::Find("robot_dialogs.yson"), ::NYson::EYsonType::ListFragment);

        {
            const i64 expectedSize = dialogsExpected.AsList().size();
            const auto actualSize = client->Get(dialogsActual + "/@row_count").AsInt64();
            UNIT_ASSERT_VALUES_EQUAL(expectedSize, actualSize);
        }

        TVector<NYT::TNode> dialogsActualRows;
        TVector<NYT::TNode> dialogsExpectedRows;
        for (const auto& node : dialogsExpected.AsList()) {
            dialogsExpectedRows.push_back(node);
        }

        const auto comparator = [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
            if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                return lhs["uuid"].AsString() < rhs["uuid"].AsString();
            }
            return lhs["request_id"].AsString() < rhs["request_id"].AsString();
        };

        for (auto readerActual = client->CreateTableReader<NYT::TNode>(dialogsActual); readerActual->IsValid();
             readerActual->Next()) {
            dialogsActualRows.push_back(readerActual->GetRow());
        }

        Sort(dialogsActualRows, comparator);
        Sort(dialogsExpectedRows, comparator);

        for (size_t i = 0; i < dialogsActualRows.size(); ++i) {
            TString diff;
            TStringOutput out{diff};

            NLibgit2::UnifiedDiff(
                NYT::NodeToCanonicalYsonString(dialogsExpectedRows[i], NYson::EYsonFormat::Pretty) + '\n',
                NYT::NodeToCanonicalYsonString(dialogsActualRows[i], NYson::EYsonFormat::Pretty) + '\n', 3, out,
                /* colored= */ false);

            UNIT_ASSERT_EQUAL_C(dialogsExpectedRows[i], dialogsActualRows[i], diff);
        }

        const auto dialogsErrorExpected =
            NYT::NodeFromYsonString(NResource::Find("robot_dialogs_error.yson"), ::NYson::EYsonType::ListFragment);

        UNIT_ASSERT_EQUAL(static_cast<i64>((dialogsErrorExpected.AsList().size())),
                          client->Get(dialogsErrorActual + "/@row_count").AsInt64());
        TVector<NYT::TNode> dialogsErrorSortedActual;

        for (auto readerActual = client->CreateTableReader<NYT::TNode>(dialogsErrorActual); readerActual->IsValid();
             readerActual->Next()) {
            auto& actual = readerActual->GetRow();
            dialogsErrorSortedActual.push_back(actual);
        }

        Sort(dialogsErrorSortedActual, [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
            if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                return lhs["uuid"].AsString() < rhs["uuid"].AsString();
            }
            return lhs["request_id"].AsString() < rhs["request_id"].AsString();
        });

        for (size_t i = 0; i < dialogsErrorExpected.AsList().size(); i++) {
            UNIT_ASSERT_EQUAL(dialogsErrorSortedActual[i], dialogsErrorExpected.AsList()[i]);
        }
    }

    Y_UNIT_TEST(DoNotUseUserLogs) {
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ {}, /* prohibitedByRegion= */ {});
            UNIT_ASSERT(!doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ {}, /* prohibitedByRegion= */ false);
            UNIT_ASSERT(!doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ {}, /* prohibitedByRegion= */ true);
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(*doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ false, /* prohibitedByRegion= */ {});
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(!*doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ false, /* prohibitedByRegion= */ false);
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(!*doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ false, /* prohibitedByRegion= */ true);
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(*doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ true, /* prohibitedByRegion= */ {});
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(*doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ true, /* prohibitedByRegion= */ false);
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(*doNotUserUserLogs);
        }
        {
            const auto doNotUserUserLogs =
                NImpl::DoNotUseUserLogs(/* doNotUseUserLogs= */ true, /* prohibitedByRegion= */ true);
            UNIT_ASSERT(doNotUserUserLogs);
            UNIT_ASSERT(*doNotUserUserLogs);
        }
    }

    Y_UNIT_TEST(CensorDialogs) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto dialogs1 = CreateRandomTable(client, directory, "dialogs1");
        const auto dialogs2 = CreateRandomTable(client, directory, "dialogs2");
        const auto privateUsers = CreateRandomTable(client, directory, "private-users");
        const auto actualCensoredDialogs1 = CreateRandomTable(client, directory, "censored-dialogs1");
        const auto actualCensoredDialogs2 = CreateRandomTable(client, directory, "censored-dialogs2");

        for (const auto& [table, resource] :
             {std::tie(dialogs1, "dialogs1.yson"), std::tie(dialogs2, "dialogs2.yson")}) {
            const auto tableTmp = CreateRandomTable(client, directory, "dialogs-unsorted");
            auto writer =
                client->CreateTableWriter<NYT::TNode>(NYT::TRichYPath(tableTmp).Schema(NImpl::DIALOGS_SCHEMA));
            const auto rows = NYT::NodeFromYsonString(NResource::Find(resource), ::NYson::EYsonType::ListFragment);
            for (const auto& row : rows.AsList()) {
                writer->AddRow(row);
            }
            writer->Finish();
            client->Sort(NYT::TSortOperationSpec{}.AddInput(tableTmp).Output(table).SortBy(
                {"server_time_ms", "uuid", "sequence_number"}));
        }

        CensorDialogs(client, directory, {dialogs1, dialogs2}, {actualCensoredDialogs1, actualCensoredDialogs2},
                      privateUsers, /* threadCount= */ 2);

        for (const auto& [oldDialogs, newDialogs] :
             {std::tie(dialogs1, actualCensoredDialogs1), std::tie(dialogs2, actualCensoredDialogs2)}) {
            UNIT_ASSERT_EQUAL(client->Get(oldDialogs + "/@schema"), client->Get(newDialogs + "/@schema"));
        }

        for (const auto& table : {actualCensoredDialogs1, actualCensoredDialogs2}) {
            UNIT_ASSERT_EQUAL("brotli_8", client->Get(table + "/@compression_codec").AsString());
            UNIT_ASSERT_EQUAL("lrc_12_2_2", client->Get(table + "/@erasure_codec").AsString());
            UNIT_ASSERT_EQUAL("scan", client->Get(table + "/@optimize_for").AsString());

            const THashMap<TString, int> sortColumns{{"server_time_ms", 0}, {"uuid", 1}, {"sequence_number", 2}};
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
            UNIT_ASSERT_EQUAL(3, order);
        }

        for (const auto& [actualCensoredDialogs, expectedCensoredDialogs] :
             {std::tie(actualCensoredDialogs1, "censored_dialogs1.yson"),
              std::tie(actualCensoredDialogs2, "censored_dialogs2.yson")}) {
            TVector<NYT::TNode> dialogsActualRows, dialogsExpectedRows;
            for (auto readerActual = client->CreateTableReader<NYT::TNode>(actualCensoredDialogs);
                 readerActual->IsValid(); readerActual->Next()) {
                dialogsActualRows.push_back(readerActual->GetRow());
            }

            const auto rows =
                NYT::NodeFromYsonString(NResource::Find(expectedCensoredDialogs), ::NYson::EYsonType::ListFragment);
            for (const auto& node : rows.AsList()) {
                dialogsExpectedRows.push_back(node);
            }

            const auto comparator = [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
                if (lhs["request_id"].AsString() != rhs["request_id"].AsString()) {
                    return lhs["request_id"].AsString() < rhs["request_id"].AsString();
                }
                return lhs["server_time_ms"].AsUint64() < rhs["server_time_ms"].AsUint64();
            };

            Sort(dialogsActualRows, comparator);
            Sort(dialogsExpectedRows, comparator);

            UNIT_ASSERT_EQUAL_C(dialogsExpectedRows.size(), dialogsActualRows.size(),
                                TStringBuilder{} << dialogsExpectedRows.size() << ' ' << dialogsActualRows.size());

            for (size_t i = 0; i < dialogsActualRows.size(); ++i) {
                TString diff;
                TStringOutput out{diff};

                NLibgit2::UnifiedDiff(
                    NYT::NodeToCanonicalYsonString(dialogsExpectedRows[i], NYson::EYsonFormat::Pretty) + '\n',
                    NYT::NodeToCanonicalYsonString(dialogsActualRows[i], NYson::EYsonFormat::Pretty) + '\n', 3, out,
                    /* colored= */ false);

                NImpl::ChangeIds(dialogsExpectedRows[i], /* newPuid= */ "", /* newUuid= */ "",
                                 /* newNormalizedUuid= */ "", /* newDeviceId= */ "");
                NImpl::ChangeIds(dialogsActualRows[i], /* newPuid= */ "", /* newUuid= */ "",
                                 /* newNormalizedUuid= */ "", /* newDeviceId= */ "");

                UNIT_ASSERT_EQUAL_C(dialogsExpectedRows[i], dialogsActualRows[i], diff);
            }
        }
    }

    Y_UNIT_TEST(DialogsMultipleMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto wonderlogs1 = CreateRandomTable(client, directory, "wonderlogs1");
        const auto wonderlogs2 = CreateRandomTable(client, directory, "wonderlogs2");
        const auto dialogsActual1 = CreateRandomTable(client, directory, "dialogs1");
        const auto dialogsActual2 = CreateRandomTable(client, directory, "dialogs2");
        const auto bannedDialogsActual1 = CreateRandomTable(client, directory, "robot-dialogs1");
        const auto bannedDialogsActual2 = CreateRandomTable(client, directory, "robot-dialogs2");
        const auto dialogsErrorActual1 = CreateRandomTable(client, directory, "error-dialogs1");
        const auto dialogsErrorActual2 = CreateRandomTable(client, directory, "error-dialogs2");
        const std::function<bool(const TWonderlog&, const TWonderlog&)> comparator1 = [](const TWonderlog& lhs,
                                                                                         const TWonderlog& rhs) {
            if (lhs.GetServerTimeMs() != rhs.GetServerTimeMs()) {
                return lhs.GetServerTimeMs() < rhs.GetServerTimeMs();
            }
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            return lhs.GetSequenceNumber() < rhs.GetSequenceNumber();
        };
        const std::function<bool(const TWonderlog&, const TWonderlog&)> comparator2 = [](const TWonderlog& lhs,
                                                                                         const TWonderlog& rhs) {
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            return lhs.GetMessageId() < rhs.GetMessageId();
        };

        NYT::TSortColumns sortColumns1{"_server_time_ms", "_uuid", "_sequence_number"};
        NYT::TSortColumns sortColumns2{"_uuid", "_message_id"};

        for (const auto& [resource, table, comparator, sortColumns] :
             {std::tie("wonderlogs1.jsonlines", wonderlogs1, comparator1, sortColumns1),
              std::tie("wonderlogs2.jsonlines", wonderlogs2, comparator2, sortColumns2)}) {
            TVector<TWonderlog> wonderlogRows;
            const auto rows = NResource::Find(resource);
            for (const TStringBuf wonderlogJson : StringSplitter(rows).Split('\n')) {
                TWonderlog wonderlogRow;
                // TODO(ran1s) MEGAMIND-3467
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = true;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(wonderlogJson), &wonderlogRow, options).ok());
                wonderlogRows.push_back(wonderlogRow);
            }
            Sort(wonderlogRows, comparator);
            auto writer = client->CreateTableWriter<TWonderlog>(
                NYT::TRichYPath(table).Schema(NYT::CreateTableSchema<TWonderlog>(sortColumns)));

            for (const auto& row : wonderlogRows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        client->Create(directory + "/tmp", NYT::ENodeType::NT_MAP);

        MakeDialogsMultiple(client, directory + "/tmp/", {wonderlogs1, wonderlogs2}, {dialogsActual1, dialogsActual2},
                            {bannedDialogsActual1, bannedDialogsActual2}, {dialogsErrorActual1, dialogsErrorActual2},
                            /* threadCount= */ 2, /* bannedUsers= */ {}, /* productionEnvironment= */ {});
        for (const auto& [dialogsActual, dialogsExpectedResource] :
             {std::tie(dialogsActual1, "dialogs1.yson"), std::tie(dialogsActual2, "dialogs2.yson")}) {
            UNIT_ASSERT(!client->Exists(dialogsActual + "/@expiration_time"));

            {
                UNIT_ASSERT_EQUAL("brotli_8", client->Get(dialogsActual + "/@compression_codec").AsString());
                UNIT_ASSERT_EQUAL("lrc_12_2_2", client->Get(dialogsActual + "/@erasure_codec").AsString());
                UNIT_ASSERT_EQUAL("scan", client->Get(dialogsActual + "/@optimize_for").AsString());

                const THashMap<TString, int> sortColumns{{"server_time_ms", 0}, {"uuid", 1}, {"sequence_number", 2}};
                int order = 0;
                NYT::TTableSchema schema;
                NYT::Deserialize(schema, client->Get(dialogsActual + "/@schema"));
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
                UNIT_ASSERT_EQUAL(3, order);
            }

            const auto dialogsExpected =
                NYT::NodeFromYsonString(NResource::Find(dialogsExpectedResource), ::NYson::EYsonType::ListFragment);

            {
                const i64 expectedSize = dialogsExpected.AsList().size();
                const auto actualSize = client->Get(dialogsActual + "/@row_count").AsInt64();
                UNIT_ASSERT_VALUES_EQUAL(expectedSize, actualSize);
            }

            TVector<NYT::TNode> dialogsActualRows;
            TVector<NYT::TNode> dialogsExpectedRows;
            for (const auto& node : dialogsExpected.AsList()) {
                dialogsExpectedRows.push_back(node);
            }

            const auto comparator = [](const NYT::TNode& lhs, const NYT::TNode& rhs) {
                if (lhs["uuid"].AsString() != rhs["uuid"].AsString()) {
                    return lhs["uuid"].AsString() < rhs["uuid"].AsString();
                }
                return lhs["request_id"].AsString() < rhs["request_id"].AsString();
            };

            for (auto readerActual = client->CreateTableReader<NYT::TNode>(dialogsActual); readerActual->IsValid();
                 readerActual->Next()) {
                dialogsActualRows.push_back(readerActual->GetRow());
            }

            Sort(dialogsActualRows, comparator);
            Sort(dialogsExpectedRows, comparator);

            for (size_t i = 0; i < dialogsActualRows.size(); ++i) {
                TString diff;
                TStringOutput out{diff};

                NLibgit2::UnifiedDiff(
                    NYT::NodeToCanonicalYsonString(dialogsExpectedRows[i], NYson::EYsonFormat::Pretty) + '\n',
                    NYT::NodeToCanonicalYsonString(dialogsActualRows[i], NYson::EYsonFormat::Pretty) + '\n', 3, out,
                    /* colored= */ false);

                UNIT_ASSERT_EQUAL_C(dialogsExpectedRows[i], dialogsActualRows[i], diff);
            }
        }
    }

    Y_UNIT_TEST(GenerateRandomGuidAndUi64) {
        const TString uuid = "uu/206087f55d58ce39f614423652b4973";
        const TString puid = "6185004569882658942";
        const ui64 privateUntilTimeMs = 1596834000995;

        const auto puidHash = HashStringToUi64(puid);
        UNIT_ASSERT_EQUAL_C(puidHash, 1553731486626779960, ToString(puidHash));

        const auto uuidHash = HashStringToUi64(uuid);
        UNIT_ASSERT_EQUAL_C(uuidHash, 7770747449032270831, ToString(uuidHash));

        const ui64 seed = privateUntilTimeMs + puidHash + uuidHash;
        UNIT_ASSERT_EQUAL_C(seed, 9324480532493051786ull, ToString(seed));

        const auto ids = NImpl::TIds::Generate(seed);

        UNIT_ASSERT_EQUAL_C(ids.Puid, "5757234879733442190", ids.Puid);
        UNIT_ASSERT_EQUAL_C(ids.Uuid, "2dea3aac-b0640850-362f502e-50ead981", ids.Uuid);
        UNIT_ASSERT_EQUAL_C(ids.DeviceId, "efc1b61d-8fff8b43-11203d2f-e8bac3a0", ids.DeviceId);
    }

    Y_UNIT_TEST(BannedUsers) {
        {
            TWonderlog wonderlog;
            TBannedUsers bannedUsers{.Ips{"ip"}, .Uuids{"uuid"}, .DeviceIds{"device_id"}};
            UNIT_ASSERT(!bannedUsers.Banned(wonderlog));
        }
        {
            TWonderlog wonderlog;
            wonderlog.SetUuid("uuid");
            TBannedUsers bannedUsers{.Uuids{"uuid"}};
            UNIT_ASSERT(bannedUsers.Banned(wonderlog));
        }
        {
            TWonderlog wonderlog;
            wonderlog.MutableDownloadingInfo()->MutableMegamind()->SetClientIp("megamind_ip");
            TBannedUsers bannedUsers{.Ips{"megamind_ip"}};
            UNIT_ASSERT(bannedUsers.Banned(wonderlog));
        }
        {
            TWonderlog wonderlog;
            wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetClientIp("uniproxy_ip");
            TBannedUsers bannedUsers{.Ips{"uniproxy_ip"}};
            UNIT_ASSERT(bannedUsers.Banned(wonderlog));
        }
        {
            TWonderlog wonderlog;
            wonderlog.MutableSpeechkitRequest()->MutableApplication()->SetDeviceId("device_id");
            TBannedUsers bannedUsers{.DeviceIds{"device_id"}};
            UNIT_ASSERT(bannedUsers.Banned(wonderlog));
        }
        {
            TWonderlog wonderlog;
            wonderlog.MutableClient()->MutableApplication()->SetDeviceId("device_id");
            TBannedUsers bannedUsers{.DeviceIds{"device_id"}};
            UNIT_ASSERT(bannedUsers.Banned(wonderlog));
        }
        {
            TWonderlog wonderlog;
            wonderlog.SetUuid("uuid");
            wonderlog.MutableDownloadingInfo()->MutableMegamind()->SetClientIp("megamind_ip");
            wonderlog.MutableDownloadingInfo()->MutableUniproxy()->SetClientIp("uniproxy_ip");
            wonderlog.MutableSpeechkitRequest()->MutableApplication()->SetDeviceId("device_id1");
            wonderlog.MutableClient()->MutableApplication()->SetDeviceId("device_id2");
            {
                TBannedUsers bannedUsers{.Ips{"ip"}, .Uuids{"uuid1"}, .DeviceIds{"device_id"}};
                UNIT_ASSERT(!bannedUsers.Banned(wonderlog));
            }
            {
                TBannedUsers bannedUsers{.Ips{"megamind_ip"}, .Uuids{"uuid1"}, .DeviceIds{"device_id"}};
                UNIT_ASSERT(bannedUsers.Banned(wonderlog));
            }
            {
                TBannedUsers bannedUsers{.Ips{"ip"}, .Uuids{"uuid"}, .DeviceIds{"device_id"}};
                UNIT_ASSERT(bannedUsers.Banned(wonderlog));
            }
            {
                TBannedUsers bannedUsers{.Ips{"ip"}, .Uuids{"uuid1"}, .DeviceIds{"device_id1"}};
                UNIT_ASSERT(bannedUsers.Banned(wonderlog));
            }
            {
                TBannedUsers bannedUsers{.Ips{"ip"}, .Uuids{"uuid1"}, .DeviceIds{"device_id2"}};
                UNIT_ASSERT(bannedUsers.Banned(wonderlog));
            }
        }
    }
}
