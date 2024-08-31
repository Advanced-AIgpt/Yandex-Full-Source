#include <google/protobuf/text_format.h>

#include "alice/library/unittest/message_diff.h"

#include <alice/wonderlogs/library/builders/uniproxy.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

const TString PROTO_UNIPROXY_ASR_DEBUG = R"(
Uuid: "lol"
MessageId: "kek"
Value {
    BurstDetector {
        TaskCount: 8
        Status: "Accepted"
        Confidence: 0.570281148
    }
    OnlineValidation {
        DecoderThreshold: -0.73
        Type: "custom_seq2seq"
        Confidence: -0.1241455078
    }
    StreamValidationContext {
        SubmittedAsrFrontMs: 1000
        MaxAsrFrontMs: 1000
        SpotterBackMs: 2000
    }
})";

const TString PROTO_UNIPROXY_PREPARED_FROM_ASR_DEBUG = R"(
Uuid: "lol"
MessageId: "kek"
SuccessfulClientRetry: false
Presence {
    MegamindRequest: false
    MegamindResponse: false
    RequestStat: false
    SpotterValidation: false
    SpotterStream: false
    Stream: false
    LogSpotter: false
    VoiceInput: false
    AsrRecognize: false
    AsrResult: false
    SynchronizeState: false
    MegamindTimings: false
    TtsTimings: false
    TtsGenerate: false
    AsrDebug: true
}
RealMessageId: true
AsrDebug {
    BurstDetector {
        TaskCount: 8
        Status: "Accepted"
        Confidence: 0.570281148
    }
    OnlineValidation {
        DecoderThreshold: -0.73
        Type: "custom_seq2seq"
        Confidence: -0.1241455078
    }
    StreamValidationContext {
        SubmittedAsrFrontMs: 1000
        MaxAsrFrontMs: 1000
        SpotterBackMs: 2000
    }
})";

const TString PROTO_LOG_SPOTTER_STREAM_MANY_MICS = R"(
Uuid: "166f281cb015d3becab28753d13d452b"
MessageId: "fab3a57f-d7d3e5a5-74f02af2-4ae5bece"
TimestampLogMs: 1630184400825
RealMessageId: false
LogSpotter {
    MessageId: "58984ff1-8a7b-4265-a8e4-af802d94e209"
    Transcript: "* \320\260\320\273\320\270\321\201\320\260"
    Source: "ru-RU-yandexmini-alisa-15Jan20-0.56-0.10-compressed-ads-detector8-am-tune-shifts-subhit-at-peak"
    Firmware: "1.79.4.13.1049191357.20210817"
    SpotterActivationInfo {
        StreamType: "vqe_0"
        QuasmodromGroup: "production"
        QuasmodromSubgroup: "production"
        Context: "activation"
        ActualSoundAfterTriggerMs: 500
        ActualSoundBeforeTriggerMs: 1500
        RequestSoundAfterTriggerMs: 500
        RequestSoundBeforeTriggerMs: 1500
        IsSpotterSound: true
        UnhandledBytes: "5536"
        DurationSubmitted: "127833680"
        GlobalStreamId: "a861eb02-1420-4c6e-99c6-811cf04fd0d2"
        SpotterStats {
            Confidences: 0.202424
            FreqFilterState: "passed"
            FreqFilterConfidence: 5.151806
        }
    }
}
Stream {
    Mds: "http://storage-int.mds.yandex.net:80/get-speechbase/1023280/70d5c62f-d98a-4dd1-82bb-ea507c3a9d72_58984ff1-8a7b-4265-a8e4-af802d94e209_1.opus"
    Format: "audio/opus"
}
SpotterMicsMergedInfo {
    StreamType: "vqe_0"
    MdsUrl: "http://storage-int.mds.yandex.net:80/get-speechbase/1023280/70d5c62f-d98a-4dd1-82bb-ea507c3a9d72_58984ff1-8a7b-4265-a8e4-af802d94e209_1.opus"
})";

const TString PROTO_UNIPROXY_PREPARED_WITH_TEST_IDS = R"(
Uuid: "ACM"
MessageId: "ICPC"
SuccessfulClientRetry: false
TimestampLogMs: 2019
Presence {
    MegamindRequest: false
    MegamindResponse: false
    RequestStat: false
    SpotterValidation: false
    SpotterStream: false
    Stream: false
    LogSpotter: false
    VoiceInput: false
    AsrRecognize: false
    AsrResult: false
    SynchronizeState: false
    MegamindTimings: false
    TtsTimings: false
    TtsGenerate: false
    AsrDebug: false
    TestIds: true
}
RealMessageId: true
TestIds: 0
TestIds: 1
)";

} // namespace

namespace NAlice::NWonderlogs {

Y_UNIT_TEST_SUITE(Uniproxy) {
    Y_UNIT_TEST(DisjunctionDoNotUseUserLogsFalseFirst) {
        TUniproxyPreparedBuilder builder;
        const TString UUID = "lol";
        const TString MESSAGE_ID = "kek";
        {
            TUniproxyPrepared::TMessageIdToDoNotUseUserLogs messageIdToDoNotUseUserLogs;
            messageIdToDoNotUseUserLogs.SetUuid(UUID);
            messageIdToDoNotUseUserLogs.SetMessageId(MESSAGE_ID);
            messageIdToDoNotUseUserLogs.SetDoNotUseUserLogs(false);
            UNIT_ASSERT(builder.AddMessageIdToDoNotUseUserLogs(messageIdToDoNotUseUserLogs).empty());
        }
        UNIT_ASSERT(builder.UniproxyPrepared.HasDoNotUseUserLogs() && !builder.UniproxyPrepared.GetDoNotUseUserLogs());
        {
            TUniproxyPrepared::TMessageIdToDoNotUseUserLogs messageIdToDoNotUseUserLogs;
            messageIdToDoNotUseUserLogs.SetUuid(UUID);
            messageIdToDoNotUseUserLogs.SetMessageId(MESSAGE_ID);
            messageIdToDoNotUseUserLogs.SetDoNotUseUserLogs(true);
            TUniproxyPreparedBuilder::TErrors actual;
            {
                TUniproxyPrepared::TError error;
                error.SetProcess(TUniproxyPrepared::TError::P_UNIPROXY_PREPARED_REDUCER);
                error.SetReason(TUniproxyPrepared::TError::R_DIFFERENT_VALUES);
                error.SetMessage("Got different do_not_use_user_logs: 0 1");
                error.SetUuid(UUID);
                error.SetMessageId(MESSAGE_ID);
                error.SetSetraceUrl("https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=kek");
                actual.emplace_back(error);
            }
            const auto expected = builder.AddMessageIdToDoNotUseUserLogs(messageIdToDoNotUseUserLogs);
            UNIT_ASSERT_EQUAL(expected.size(), actual.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                UNIT_ASSERT_MESSAGES_EQUAL(expected[i], actual[i]);
            }
        }
        UNIT_ASSERT(std::move(builder).Build().GetDoNotUseUserLogs());
    }

    Y_UNIT_TEST(DisjunctionDoNotUseUserLogsTrueFirst) {
        TUniproxyPreparedBuilder builder;
        const TString UUID = "lol";
        const TString MESSAGE_ID = "kek";
        {
            TUniproxyPrepared::TMessageIdToDoNotUseUserLogs messageIdToDoNotUseUserLogs;
            messageIdToDoNotUseUserLogs.SetUuid(UUID);
            messageIdToDoNotUseUserLogs.SetMessageId(MESSAGE_ID);
            messageIdToDoNotUseUserLogs.SetDoNotUseUserLogs(true);
            UNIT_ASSERT(builder.AddMessageIdToDoNotUseUserLogs(messageIdToDoNotUseUserLogs).empty());
        }
        UNIT_ASSERT(builder.UniproxyPrepared.HasDoNotUseUserLogs() && builder.UniproxyPrepared.GetDoNotUseUserLogs());
        {
            TUniproxyPrepared::TMessageIdToDoNotUseUserLogs messageIdToDoNotUseUserLogs;
            messageIdToDoNotUseUserLogs.SetUuid(UUID);
            messageIdToDoNotUseUserLogs.SetMessageId(MESSAGE_ID);
            messageIdToDoNotUseUserLogs.SetDoNotUseUserLogs(false);
            TUniproxyPreparedBuilder::TErrors actual;
            {
                TUniproxyPrepared::TError error;
                error.SetProcess(TUniproxyPrepared::TError::P_UNIPROXY_PREPARED_REDUCER);
                error.SetReason(TUniproxyPrepared::TError::R_DIFFERENT_VALUES);
                error.SetMessage("Got different do_not_use_user_logs: 1 0");
                error.SetUuid(UUID);
                error.SetMessageId(MESSAGE_ID);
                error.SetSetraceUrl("https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=kek");
                actual.emplace_back(error);
            }
            const auto expected = builder.AddMessageIdToDoNotUseUserLogs(messageIdToDoNotUseUserLogs);
            UNIT_ASSERT_EQUAL(expected.size(), actual.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                UNIT_ASSERT_MESSAGES_EQUAL(expected[i], actual[i]);
            }
        }
        UNIT_ASSERT(std::move(builder).Build().GetDoNotUseUserLogs());
    }

    Y_UNIT_TEST(AddAsrDebug) {
        TUniproxyPrepared actual, expected;
        TUniproxyPreparedBuilder builder;
        TUniproxyPrepared::TAsrDebug asrDebug;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_UNIPROXY_ASR_DEBUG, &asrDebug));
        UNIT_ASSERT(builder.AddAsrDebug(asrDebug).empty());
        actual = std::move(builder).Build();
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_UNIPROXY_PREPARED_FROM_ASR_DEBUG, &expected));
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(AddTestIds) {
        TUniproxyPrepared actual, expected;
        TUniproxyPreparedBuilder builder;
        TUniproxyPrepared::TTestIds testIds;
        testIds.SetUuid("ACM");
        testIds.SetMessageId("ICPC");
        testIds.SetTimestampLogMs(2019);
        testIds.MutableTestIds()->Add(0);
        testIds.MutableTestIds()->Add(1);
        UNIT_ASSERT(builder.AddTestIds(testIds).empty());
        actual = std::move(builder).Build();
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_UNIPROXY_PREPARED_WITH_TEST_IDS, &expected));
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }
}

} // namespace NAlice::NWonderlogs
