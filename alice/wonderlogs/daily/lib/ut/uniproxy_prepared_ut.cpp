#include <alice/wonderlogs/daily/lib/ttls.h>
#include <alice/wonderlogs/daily/lib/uniproxy_prepared.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include <library/cpp/resource/registry.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

#include <util/string/split.h>

using namespace NAlice::NWonderlogs;

namespace {

// synchronize state without do_not_use_user_logs flag

constexpr inline TStringBuf UNIPROXY_LOGS = R"(
{
    "fields" = #;
    "level" = 20000;
    "levelStr" = "INFO";
    "iso_eventtime" = "2021-08-29 00:00:00";
    "message" = "SESSIONLOG: {\"Session\":{\"AppId\":\"aliced\",\"Timestamp\":\"2021-08-28T21:00:00.007830Z\",\"Uid\":\"\",\"AppType\":\"aliced\",\"IpAddr\":\"89.207.221.23\",\"SessionId\":\"337cfd1d-1f31-4766-a92b-ee16f124bcb0\",\"Action\":\"request\",\"RetryNumber\":0,\"Uuid\":\"f62c08de30a07866e201c346e9fc2ac6\"},\"Event\":{\"event\":{\"payload\":{\"speechkitVersion\":\"4.14.6\",\"device_manufacturer\":\"Yandex\",\"vins\":{\"application\":{\"device_manufacturer\":\"Yandex\",\"platform\":\"Linux\",\"device_model\":\"yandexmini\",\"quasmodrom_subgroup\":\"production\",\"device_id\":\"FF98F029D655538EE5600293\",\"uuid\":\"f62c08de30a07866e201c346e9fc2ac6\",\"app_version\":\"1.0\",\"os_version\":\"1.0\",\"device_revision\":\"\",\"quasmodrom_group\":\"production\",\"app_id\":\"aliced\"}},\"network_type\":\"\",\"supported_features\":[\"multiroom\",\"multiroom_cluster\",\"multiroom_audio_client\",\"change_alarm_sound\",\"change_alarm_sound_level\",\"music_player_allow_shots\",\"bluetooth_player\",\"audio_client\",\"audio_client_hls\",\"notifications\",\"tts_play_placeholder\",\"incoming_messenger_calls\",\"publicly_available\",\"directive_sequencer\",\"set_alarm_semantic_frame_v2\",\"muzpult\",\"audio_bitrate192\",\"audio_bitrate320\",\"prefetch_invalidation\"],\"oauth_token\":\"\",\"device_model\":\"yandexmini\",\"accept_invalid_auth\":true,\"yandexuid\":\"1024332996\",\"uuid\":\"f62c08de30a07866e201c346e9fc2ac6\",\"device_revision\":\"\",\"platform_info\":\"Linux\",\"device\":\"Yandex yandexmini\",\"auth_token\":\"51ae06cc-5c8f-48dc-93ae-7214517679e6\",\"sound_logging\":true,\"uniproxy2\":{\"session_log\":true}},\"header\":{\"name\":\"SynchronizeState\",\"messageId\":\"72450f85-6803-44c2-bcf6-bce78a092613\",\"refStreamId\":2,\"rtLogToken\":\"1630184400007750$72450f85-6803-44c2-bcf6-bce78a092613$9c151930-843e37e6-fee6311b-fe94a59\",\"namespace\":\"System\"}}}}";
    "qloud_component" = "uniproxy";
    "stackTrace" = #;
    "qloud_instance" = "-";
    "threadName" = "qloud-init";
    "_stbx" = "rt3.man--alice-production--uniproxy:4@@1341651662@@facd04f-6624904e-569ed4a0-b36607f6@@1630184400175@@1630184400@@uniproxy@@311252649@@1630184400280";
    "source_uri" = "prt://alice-production@2a02:6b8:c0b:6523:0:4234:dfdb:0;unknown_path";
    "qloud_application" = "uniproxy";
    "timestamp" = "2021-08-28T21:00:00+0000";
    "loggerName" = "stdout";
    "_logfeller_index_bucket" = "//home/logfeller/index/alice-production/uniproxy/1800-1800/1630185600/1630184400";
    "qloud_environment" = "prod";
    "rest" = {
        "pushclient_row_id" = "311615038"
    };
    "_logfeller_timestamp" = 1630184400u;
    "host" = "dtmf6domchoprqbj.man.yp-c.yandex.net";
    "qloud_project" = "alice";
    "version" = 1
};
{
    "fields" = #;
    "level" = 20000;
    "levelStr" = "INFO";
    "iso_eventtime" = "2021-08-29 00:00:00";
    "message" = "SESSIONLOG: {\"Session\":{\"AppId\":\"aliced\",\"Timestamp\":\"2021-08-28T21:00:00.016319Z\",\"Uid\":\"\",\"AppType\":\"aliced\",\"IpAddr\":\"213.87.157.16\",\"SessionId\":\"337cfd1d-1f31-4766-a92b-ee16f124bcb0\",\"Action\":\"request\",\"RetryNumber\":2,\"DoNotUseUserLogs\":true,\"Uuid\":\"f62c08de30a07866e201c346e9fc2ac6\"},\"Event\":{\"event\":{\"payload\":{\"isSeamlessActivation\":true,\"durations\":{\"onRecognitionBeginTime-onFirstMessageMergedTime\":\"471\",\"onStartVoiceInputTime-onRecognitionBeginTime\":\"68\"},\"oauth_token\":\"AgAAAABHAbrPAAQXFQr********************\",\"reconnectionCount\":0,\"ack\":1630184399,\"isSpotterActivated\":true,\"timestamps\":{\"onRecognitionBeginTime\":\"92\",\"medianAsrRtf\":107.5,\"onStartVoiceInputTime\":\"24\",\"onFirstMessageMergedTime\":\"563\",\"onFirstNonEmptyPartialTime\":\"1182\",\"onPhraseSpottedTime\":\"0\",\"onLastCompletedPartialTime\":\"3264\",\"minAsrRtf\":28.7,\"requestDurationTime\":\"4512\",\"spotterConfirmationTime\":\"1036\",\"onVinsResponseTime\":\"4529\",\"averageAsrRtf\":84.02668161,\"maxAsrRtf\":117.35},\"refMessageId\":\"6d487977-6de5-4428-b541-9acb998cad59\",\"audioProcessingMode\":\"PASS_AUDIO\",\"cancelled\":false},\"header\":{\"name\":\"RequestStat\",\"messageId\":\"045aeecf-6ca7-4e60-bb2a-76effe8a64a6\",\"ack\":1630184399,\"refStreamId\":432,\"rtLogToken\":\"1630184400016239$045aeecf-6ca7-4e60-bb2a-76effe8a64a6$408a81b6-a22c02e5-4484a9e0-eb46898d\",\"namespace\":\"Log\"}}}}";
    "qloud_component" = "uniproxy";
    "stackTrace" = #;
    "qloud_instance" = "-";
    "threadName" = "qloud-init";
    "_stbx" = "rt3.man--alice-production--uniproxy:6@@1324169156@@67b14c9d-a221b3d0-befce9c0-4e6dae2a@@1630184400017@@1630184499@@uniproxy@@316192536@@1630184400122";
    "source_uri" = "prt://alice-production@2a02:6b8:c09:3899:0:4234:488e:0;unknown_path";
    "qloud_application" = "uniproxy";
    "timestamp" = "2021-08-28T21:00:00+0000";
    "loggerName" = "stdout";
    "_logfeller_index_bucket" = "//home/logfeller/index/alice-production/uniproxy/1800-1800/1630185600/1630184400";
    "qloud_environment" = "prod";
    "rest" = {
        "pushclient_row_id" = "317753138"
    };
    "_logfeller_timestamp" = 1630184400u;
    "host" = "jzhiumqn6m66spnf.man.yp-c.yandex.net";
    "qloud_project" = "alice";
    "version" = 1
};
)";

const TString UNIPROXY_PREPARED = R"(
Uuid: "f62c08de30a07866e201c346e9fc2ac6"
MessageId: "6d487977-6de5-4428-b541-9acb998cad59"
RequestStat {
    Timestamps {
        OnVinsResponseTime: "4529"
        OnRecognitionBeginTime: "92"
        RequestDurationTime: "4512"
        SpotterConfirmationTime: "1036"
        OnPhraseSpottedTime: "0"
        OnStartVoiceInputTime: "24"
    }
}
ConnectSessionId: "337cfd1d-1f31-4766-a92b-ee16f124bcb0"
SuccessfulClientRetry: false
TimestampLogMs: 1630184400007
Environment {
    QloudProject: "alice"
    QloudApplication: "uniproxy"
}
ClientIp: "213.87.157.16"
Presence {
    MegamindRequest: false
    MegamindResponse: false
    RequestStat: true
    SpotterValidation: false
    SpotterStream: false
    Stream: false
    LogSpotter: false
    VoiceInput: false
    AsrRecognize: false
    AsrResult: false
    AsrDebug: false
    SynchronizeState: true
    MegamindTimings: false
    TtsTimings: false
    TtsGenerate: false
}
RealMessageId: true
SynchronizeState {
    AuthToken: "51ae06cc-5c8f-48dc-93ae-7214517679e6"
    Application {
        AppId: "aliced"
        AppVersion: "1.0"
        OsVersion: "1.0"
        Platform: "Linux"
        DeviceId: "FF98F029D655538EE5600293"
        DeviceModel: "yandexmini"
        DeviceManufacturer: "Yandex"
    }
}
DoNotUseUserLogs: true
)";

} // namespace

Y_UNIT_TEST_SUITE(UniproxyPrepared) {
    Y_UNIT_TEST(UniproxyPreparedMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto uniproxyEvents = CreateRandomTable(client, directory, "uniproxy-events");
        const auto uniproxyPreparedActual = CreateRandomTable(client, directory, "uniproxy-prepared");
        const auto uniproxyErrorActual = CreateRandomTable(client, directory, "uniproxy-error");

        {
            auto writer = client->CreateTableWriter<NYT::TNode>(uniproxyEvents);
            const auto events =
                NYT::NodeFromYsonString(NResource::Find("uniproxy_events.yson"), NYson::EYsonType::ListFragment);
            for (const auto& event : events.AsList()) {
                writer->AddRow(event);
            }
            writer->Finish();
        }
        {
            const auto timestampFrom = ParseDatetime("2021-08-29T00:00:00+03:00");
            const auto timestampTo = ParseDatetime("2021-08-30T00:00:00+03:00");
            MakeUniproxyPrepared(client, directory + "/tmp/", {uniproxyEvents}, uniproxyPreparedActual,
                                 uniproxyErrorActual, *timestampFrom, *timestampTo,
                                 /* requestsShift= */ TDuration::Minutes(10));
        }
        {
            const THashMap<TString, int> sortColumns{{"uuid", 0}, {"message_id", 1}};
            int order = 0;
            NYT::TTableSchema schema;
            NYT::Deserialize(schema, client->Get(uniproxyPreparedActual + "/@schema"));
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

        for (const auto& table : {uniproxyPreparedActual, uniproxyErrorActual}) {
            const auto expirationTime = ParseDatetime(client->Get(table + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        const auto uniproxyPreparedRows = NResource::Find("uniproxy_prepared.jsonlines");
        TVector<TUniproxyPrepared> uniproxyPreparedSortedExpected;

        for (const auto uniproxyPreparedRow : StringSplitter(uniproxyPreparedRows).Split('\n')) {
            TUniproxyPrepared expected;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString(uniproxyPreparedRow), &expected).ok());
            uniproxyPreparedSortedExpected.push_back(expected);
        }

        TVector<TUniproxyPrepared> uniproxyPreparedSortedActual;
        for (auto readerActual = client->CreateTableReader<TUniproxyPrepared>(uniproxyPreparedActual);
             readerActual->IsValid(); readerActual->Next()) {
            uniproxyPreparedSortedActual.push_back(readerActual->GetRow());
        }

        UNIT_ASSERT_EQUAL_C(uniproxyPreparedSortedExpected.size(), uniproxyPreparedSortedActual.size(),
                            TStringBuilder{} << uniproxyPreparedSortedExpected.size() << " "
                                             << uniproxyPreparedSortedActual.size());
        const auto comparator = [](const TUniproxyPrepared& lhs, const TUniproxyPrepared& rhs) {
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            if (lhs.GetRealMessageId() != rhs.GetRealMessageId()) {
                return !lhs.GetRealMessageId();
            }
            return lhs.GetMessageId() < rhs.GetMessageId();
        };
        Sort(uniproxyPreparedSortedExpected, comparator);
        Sort(uniproxyPreparedSortedActual, comparator);

        for (size_t i = 0; i < uniproxyPreparedSortedExpected.size(); ++i) {
            for (auto* uniproxyPrepared : {&uniproxyPreparedSortedExpected[i], &uniproxyPreparedSortedActual[i]}) {
                if (!uniproxyPrepared->GetRealMessageId()) {
                    uniproxyPrepared->SetMessageId("");
                }
                // TODO(ran1s) delete
                uniproxyPrepared->SetTimestampLogMs(1337);
            }
            UNIT_ASSERT_MESSAGES_EQUAL(uniproxyPreparedSortedExpected[i], uniproxyPreparedSortedActual[i]);
        }

        const auto uniproxyErrorRows = NResource::Find("uniproxy_error.jsonlines");
        TVector<TStringBuf> uniproxyErrorSortedExpected = StringSplitter(uniproxyErrorRows).Split('\n');
        while (!uniproxyErrorSortedExpected.empty() && uniproxyErrorSortedExpected.back().empty()) {
            uniproxyErrorSortedExpected.pop_back();
        }

        TVector<TUniproxyPrepared::TError> uniproxyErrorSortedActual;
        for (auto readerActual = client->CreateTableReader<TUniproxyPrepared::TError>(uniproxyErrorActual);
             readerActual->IsValid(); readerActual->Next()) {
            uniproxyErrorSortedActual.push_back(readerActual->GetRow());
        }

        UNIT_ASSERT_EQUAL_C(uniproxyErrorSortedExpected.size(), uniproxyErrorSortedActual.size(),
                            TStringBuilder{} << uniproxyErrorSortedExpected.size() << " "
                                             << uniproxyErrorSortedActual.size());
        Sort(uniproxyErrorSortedActual,
             [](const TUniproxyPrepared::TError& lhs, const TUniproxyPrepared::TError& rhs) {
                 if (lhs.GetUuid() != rhs.GetUuid()) {
                     return lhs.GetUuid() < rhs.GetUuid();
                 }
                 return lhs.GetMessage() < rhs.GetMessage();
             });
        for (size_t i = 0; i < uniproxyErrorSortedExpected.size(); i++) {
            TUniproxyPrepared::TError& actual = uniproxyErrorSortedActual[i];
            TUniproxyPrepared::TError expected;
            UNIT_ASSERT(
                google::protobuf::util::JsonStringToMessage(TString(uniproxyErrorSortedExpected[i]), &expected).ok());

            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(DoNotUseUserLogs) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto uniproxyEvents = CreateRandomTable(client, directory, "uniproxy-events");
        const auto uniproxyPreparedActual = CreateRandomTable(client, directory, "uniproxy-prepared");
        const auto uniproxyErrorActual = CreateRandomTable(client, directory, "uniproxy-error");

        {
            auto writer = client->CreateTableWriter<NYT::TNode>(uniproxyEvents);
            const auto events = NYT::NodeFromYsonString(UNIPROXY_LOGS, NYson::EYsonType::ListFragment);
            for (const auto& event : events.AsList()) {
                writer->AddRow(event);
            }
            writer->Finish();
        }
        {
            const auto timestampFrom = ParseDatetime("2021-08-29T00:00:00+03:00");
            const auto timestampTo = ParseDatetime("2021-08-30T00:00:00+03:00");
            MakeUniproxyPrepared(client, directory + "/tmp/", {uniproxyEvents}, uniproxyPreparedActual,
                                 uniproxyErrorActual, *timestampFrom, *timestampTo,
                                 /* requestsShift= */ TDuration::Minutes(10));
        }
        TUniproxyPrepared expected;
        google::protobuf::TextFormat::ParseFromString(UNIPROXY_PREPARED, &expected);
        TVector<TUniproxyPrepared> uniproxyPreparedVec;
        for (auto readerActual = client->CreateTableReader<TUniproxyPrepared>(uniproxyPreparedActual);
             readerActual->IsValid(); readerActual->Next()) {
            uniproxyPreparedVec.push_back(readerActual->GetRow());
        }
        UNIT_ASSERT_EQUAL(1, uniproxyPreparedVec.size());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, uniproxyPreparedVec[0]);
    }
}
