#include <alice/wonderlogs/library/parsers/wonderlogs.h>

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

const TString PROTO_SPOTTER_ACTIVATION_INFO = R"(
StreamType: "vqe_0"
QuasmodromGroup: "production"
QuasmodromSubgroup: "production"
Context: "activation"
ActualSoundAfterTriggerMs: 500
ActualSoundBeforeTriggerMs: 1500
RequestSoundAfterTriggerMs: 500
RequestSoundBeforeTriggerMs: 1500
IsSpotterSound: true
UnhandledDataBytes: 5856
DurationDataSubmitted: 3122528
GlobalStreamId: "e9a63c6f-4be7-4553-bff9-2cdaa7f7307f"
SpotterStats {
    Confidences: 0.657701
    FreqFilterState: "passed"
    FreqFilterConfidence: 11.164299
})";

const TString PROTO_SPOTTER_COMMON_STATS = R"(
ActualSoundAfterTriggerMs: 500
ActualSoundBeforeTriggerMs: 1500
RequestSoundAfterTriggerMs: 500
RequestSoundBeforeTriggerMs: 1500
UnhandledBytes: 5856
DurationSubmitted: 3122528
)";

const TString PROTO_UNIPROXY_PREPARED_APP_INFO = R"(
LogSpotter {
    SpotterActivationInfo {
        QuasmodromGroup: "kek"
        QuasmodromSubgroup: "lol"
    }
}
SynchronizeState {
    Application {
        AppId: "aliced"
        AppVersion: "1.0"
        OsVersion: "1.0"
        Platform: "Linux"
        DeviceId: "FF98F029D655538EE5600293"
        DeviceModel: "yandexmini"
        DeviceManufacturer: "Yandex"
        QuasmodromGroup: "not_kek"
        QuasmodromSubgroup: "not_lol"
    }
}
)";

const TString PROTO_WONDERLOG_APP_INFO = R"(
Presence {
    Uniproxy: true
    UniproxyPresence {
    }
}
Environment {
    UniproxyEnvironment {
    }
}
RealMessageId: false
Client {
    Application {
        AppId: "aliced"
        AppVersion: "1.0"
        OsVersion: "1.0"
        Platform: "Linux"
        DeviceId: "FF98F029D655538EE5600293"
        DeviceModel: "yandexmini"
        DeviceManufacturer: "Yandex"
        QuasmodromGroup: "kek"
        QuasmodromSubgroup: "lol"
    }
})";

const TString PROTO_UNIPROXY_PREPARED_FILLED_VOICE_INPUT = R"(
Uuid: "00000000000548ea8c7dea8e920feda8"                                                                                                                                                                                                                                                                                                                                                                                                            
MegamindRequestId: "0dbcf863-432e-4697-a9aa-5b09bb2e8eaf"                                                                                                                                                                                                                                                                                                                                                                         
MessageId: "09fa8471-35b6-4865-8641-1b490a4a1a63"                                                                                                                                                                                                                                                                                                                                                                                         
MegamindResponseId: "8bd1a4c7-3a85b1d5-45e8fbd2-be50153e"                                                                                                                                                                                                                                                                                                                                                                         
RequestStat {                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    Timestamps {                                                                                                                                                                                                                                                                                                                                                                                                                                                                
        OnSoundPlayerEndTime: "7119"                                                                                                                                                                                                                                                                                                                                                                                                                            
        OnVinsResponseTime: "5061"                                                                                                                                                                                                                                                                                                                                                                                                                                
        OnFirstSynthesisChunkTime: "5201"                                                                                                                                                                                                                                                                                                                                                                                                                 
        OnRecognitionBeginTime: "115"                                                                                                                                                                                                                                                                                                                                                                                                                         
        OnRecognitionEndTime: "4658"                                                                                                                                                                                                                                                                                                                                                                                                                            
        RequestDurationTime: "7119"                                                                                                                                                                                                                                                                                                                                                                                                                             
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
ConnectSessionId: "6e5c2113-fc57-4088-9402-dde7d7eb740b"                                                                                                                                                                                                                                                                                                                                                                            
SuccessfulClientRetry: true                                                                                                                                                                                                                                                                                                                                                                                                                                     
TimestampLogMs: 1642633110379                                                                                                                                                                                                                                                                                                                                                                                                                                 
Environment {                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    QloudProject: "alice"                                                                                                                                                                                                                                                                                                                                                                                                                                             
    QloudApplication: "uniproxy"                                                                                                                                                                                                                                                                                                                                                                                                                                
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
ClientIp: "188.162.228.123"                                                                                                                                                                                                                                                                                                                                                                                                                                     
Presence {                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
    MegamindRequest: true                                                                                                                                                                                                                                                                                                                                                                                                                                             
    MegamindResponse: true                                                                                                                                                                                                                                                                                                                                                                                                                                            
    RequestStat: true                                                                                                                                                                                                                                                                                                                                                                                                                                                     
    SpotterValidation: false                                                                                                                                                                                                                                                                                                                                                                                                                                        
    SpotterStream: false                                                                                                                                                                                                                                                                                                                                                                                                                                                
    Stream: true                                                                                                                                                                                                                                                                                                                                                                                                                                                                
    LogSpotter: false                                                                                                                                                                                                                                                                                                                                                                                                                                                     
    VoiceInput: true                                                                                                                                                                                                                                                                                                                                                                                                                                                        
    AsrRecognize: false
    AsrResult: true
    SynchronizeState: true
    MegamindTimings: true
    TtsTimings: true
    TtsGenerate: false
    AsrDebug: false
}
RealMessageId: true
VoiceInput {                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
    Topic: "dialog-general"                                                                                                                                                                                                                                                                                                                                                                                                                                         
    SpeechKitRequest {                                                                                                                                                                                                                                                                                                                                                                                                                                                    
        Header {                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
            RequestId: "0dbcf863-432e-4697-a9aa-5b09bb2e8eaf"                                                                                                                                                                                                                                                                                                                                                                             
            PrevReqId: "f7b99ff4-a90d-48ca-9ec1-706fcf831a5a"                                                                                                                                                                                                                                                                                                                                                                             
            SequenceNumber: 50                                                                                                                                                                                                                                                                                                                                                                                                                                            
        }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
        Application {                                                                                                                                                                                                                                                                                                                                                                                                                                                         
            DeviceId: "00000000-0005-48ea-8c7d-ea8e920feda8"                                                                                                                                                                                                                                                                                                                                                                                
            Lang: "ru-RU"                                                                                                                                                                                                                                                                                                                                                                                                                                                     
            ClientTime: "20220120T085828"                                                                                                                                                                                                                                                                                                                                                                                                                     
            Timezone: "Asia/Vladivostok"                                                                                                                                                                                                                                                                                                                                                                                                                        
            Epoch: "1642633108"                                                                                                                                                                                                                                                                                                                                                                                                                                         
        }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
        Request {                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
            Event {                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
                Type: voice_input                                                                                                                                                                                                                                                                                                                                                                                                                                         
                Name: ""                                                                                                                                                                                                                                                                                                                                                                                                                                                            
            }                                                                                                                                                                                                                                                                                                                                                                                                                                                                             
            DeviceState {                                                                                                                                                                                                                                                                                                                                                                                                                                                     
                SoundLevel: 10                                                                                                                                                                                                                                                                                                                                                                                                                                                
                SoundMuted: false                                                                                                                                                                                                                                                                                                                                                                                                                                         
                Navigator {                                                                                                                                                                                                                                                                                                                                                                                                                                                     
                    MapView {                                                                                                                                                                                                                                                                                                                                                                                                                                                     
                        BottomRightLon: 135.10704                                                                                                                                                                                                                                                                                                                                                                                                                 
                        TopRightLon: 135.10704                                                                                                                                                                                                                                                                                                                                                                                                                        
                        TopLeftLon: 135.105637                                                                                                                                                                                                                                                                                                                                                                                                                        
                        BottomLeftLon: 135.105637                                                                                                                                                                                                                                                                                                                                                                                                                 
                        TopLeftLat: 48.4975739                                                                                                                                                                                                                                                                                                                                                                                                                        
                        BottomLeftLat: 48.4956512
                        BottomRightLat: 48.4956512
                        TopRightLat: 48.4975739
                    }
                    SearchOptions {
                    }
                }
            }
            AdditionalOptions {
                OAuthToken: "1.711716920.162136.1646804425.1615268425362.35491.EwywfWbVlPj34wLi.8dVN-4iuAZUnMV-32GOWMAIjY*********************************************************************************************"
                BassOptions {
                    UserAgent: ""
                    FiltrationLevel: 0
                    ScreenScaleFactor: 1.75
                }
                SupportedFeatures: "open_link"
                SupportedFeatures: "navigator"
                SupportedFeatures: "maps_download_offline"
                UnsupportedFeatures: "music_sdk_player"
                DivKitVersion: "2.3"
                Permissions {
                    Granted: true
                    Name: "location"
                }
                Permissions {
                    Granted: false
                    Name: "read_contacts"
                }
                Permissions {
                    Granted: false
                    Name: "call_phone"
                }
            }
            VoiceSession: true
            ResetSession: true
        }
    }
}
TestIds: 1
TestIds: 2
DoNotUseUserLogs: false
)";

const TString PROTO_WONDERLOG_VOICE_INPUT = R"(
SpeechkitRequest {
    Header {
        RequestId: "0dbcf863-432e-4697-a9aa-5b09bb2e8eaf"
        PrevReqId: "f7b99ff4-a90d-48ca-9ec1-706fcf831a5a"
        SequenceNumber: 50
    }
    Application {
        DeviceId: "00000000-0005-48ea-8c7d-ea8e920feda8"
        Lang: "ru-RU"
        ClientTime: "20220120T085828"
        Timezone: "Asia/Vladivostok"
        Epoch: "1642633108"
    }
    Request {
        Event {
            Type: voice_input
            Name: ""
        }
        DeviceState {
            SoundLevel: 10
            SoundMuted: false
            Navigator {
                MapView {
                    BottomRightLon: 135.10704
                    TopRightLon: 135.10704
                    TopLeftLon: 135.105637
                    BottomLeftLon: 135.105637
                    TopLeftLat: 48.4975739
                    BottomLeftLat: 48.4956512
                    BottomRightLat: 48.4956512
                    TopRightLat: 48.4975739
                }
                SearchOptions {
                }
            }
        }
        AdditionalOptions {
            OAuthToken: "1.711716920.162136.1646804425.1615268425362.35491.EwywfWbVlPj34wLi.8dVN-4iuAZUnMV-32GOWMAIjY*********************************************************************************************"
            BassOptions {
                UserAgent: ""
                FiltrationLevel: 0
                ScreenScaleFactor: 1.75
            }
            SupportedFeatures: "open_link"
            SupportedFeatures: "navigator"
            SupportedFeatures: "maps_download_offline"
            UnsupportedFeatures: "music_sdk_player"
            DivKitVersion: "2.3"
            Permissions {
                Granted: true
                Name: "location"
            }
            Permissions {
                Granted: false
                Name: "read_contacts"
            }
            Permissions {
                Granted: false
                Name: "call_phone"
            }
        }
        VoiceSession: true
        ResetSession: true
        TestIDs: 1
        TestIDs: 2
    }
}
RequestStat {
    Timestamps {
        OnSoundPlayerEndTime: "7119"
        OnVinsResponseTime: "5061"
        OnFirstSynthesisChunkTime: "5201"
        OnRecognitionBeginTime: "115"
        OnRecognitionEndTime: "4658"
        RequestDurationTime: "7119"
    }
}
ServerTimeMs: 1642633110379
Presence {
    Uniproxy: true
    UniproxyPresence {
        MegamindRequest: true
        MegamindResponse: true
        RequestStat: true
        SpotterValidation: false
        SpotterStream: false
        Stream: true
        LogSpotter: false
        VoiceInput: true
        AsrRecognize: false
        AsrResult: true
        SynchronizeState: true
        MegamindTimings: true
        TtsTimings: true
        TtsGenerate: false
        AsrDebug: false
    }
}
Environment {
    UniproxyEnvironment {
        QloudProject: "alice"
        QloudApplication: "uniproxy"
    }
}
DownloadingInfo {
    Uniproxy {
        ClientIp: "188.162.228.123"
    }
}
Asr {
    Topics {
        Request: "dialog-general"
    }
})";

const TString PROTO_UNIPROXY_PREPARED_EMPTY_VOICE_INPUT = R"(
Uuid: "00000000000548ea8c7dea8e920feda8"                                                                                                                                                                                                                                                                                                                                                                                                            
MegamindRequestId: "0dbcf863-432e-4697-a9aa-5b09bb2e8eaf"                                                                                                                                                                                                                                                                                                                                                                         
MessageId: "09fa8471-35b6-4865-8641-1b490a4a1a63"                                                                                                                                                                                                                                                                                                                                                                                         
MegamindResponseId: "8bd1a4c7-3a85b1d5-45e8fbd2-be50153e"                                                                                                                                                                                                                                                                                                                                                                         
RequestStat {                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    Timestamps {                                                                                                                                                                                                                                                                                                                                                                                                                                                                
        OnSoundPlayerEndTime: "7119"                                                                                                                                                                                                                                                                                                                                                                                                                            
        OnVinsResponseTime: "5061"                                                                                                                                                                                                                                                                                                                                                                                                                                
        OnFirstSynthesisChunkTime: "5201"                                                                                                                                                                                                                                                                                                                                                                                                                 
        OnRecognitionBeginTime: "115"                                                                                                                                                                                                                                                                                                                                                                                                                         
        OnRecognitionEndTime: "4658"                                                                                                                                                                                                                                                                                                                                                                                                                            
        RequestDurationTime: "7119"                                                                                                                                                                                                                                                                                                                                                                                                                             
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
ConnectSessionId: "6e5c2113-fc57-4088-9402-dde7d7eb740b"                                                                                                                                                                                                                                                                                                                                                                            
SuccessfulClientRetry: true                                                                                                                                                                                                                                                                                                                                                                                                                                     
TimestampLogMs: 1642633110379                                                                                                                                                                                                                                                                                                                                                                                                                                 
Environment {                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
    QloudProject: "alice"                                                                                                                                                                                                                                                                                                                                                                                                                                             
    QloudApplication: "uniproxy"                                                                                                                                                                                                                                                                                                                                                                                                                                
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
ClientIp: "188.162.228.123"                                                                                                                                                                                                                                                                                                                                                                                                                                     
Presence {                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
    MegamindRequest: true                                                                                                                                                                                                                                                                                                                                                                                                                                             
    MegamindResponse: true                                                                                                                                                                                                                                                                                                                                                                                                                                            
    RequestStat: true                                                                                                                                                                                                                                                                                                                                                                                                                                                     
    SpotterValidation: false                                                                                                                                                                                                                                                                                                                                                                                                                                        
    SpotterStream: false                                                                                                                                                                                                                                                                                                                                                                                                                                                
    Stream: true                                                                                                                                                                                                                                                                                                                                                                                                                                                                
    LogSpotter: false                                                                                                                                                                                                                                                                                                                                                                                                                                                     
    VoiceInput: false                                                                                                                                                                                                                                                                                                                                                                                                                                                        
    AsrRecognize: false
    AsrResult: true
    SynchronizeState: true
    MegamindTimings: true
    TtsTimings: true
    TtsGenerate: false
    AsrDebug: false
}
RealMessageId: true
TestIds: 1
TestIds: 2
DoNotUseUserLogs: false
)";

const TString PROTO_WONDERLOG_EMPTY_VOICE_INPUT = R"(
SpeechkitRequest {
    Request {
        TestIDs: 1
        TestIDs: 2
    }
}
RequestStat {
    Timestamps {
        OnSoundPlayerEndTime: "7119"
        OnVinsResponseTime: "5061"
        OnFirstSynthesisChunkTime: "5201"
        OnRecognitionBeginTime: "115"
        OnRecognitionEndTime: "4658"
        RequestDurationTime: "7119"
    }
}
ServerTimeMs: 1642633110379
Presence {
    Uniproxy: true
    UniproxyPresence {
        MegamindRequest: true
        MegamindResponse: true
        RequestStat: true
        SpotterValidation: false
        SpotterStream: false
        Stream: true
        LogSpotter: false
        VoiceInput: false
        AsrRecognize: false
        AsrResult: true
        SynchronizeState: true
        MegamindTimings: true
        TtsTimings: true
        TtsGenerate: false
        AsrDebug: false
    }
}
Environment {
    UniproxyEnvironment {
        QloudProject: "alice"
        QloudApplication: "uniproxy"
    }
}
DownloadingInfo {
    Uniproxy {
        ClientIp: "188.162.228.123"
    }
})";


using namespace NAlice::NWonderlogs;

Y_UNIT_TEST_SUITE(Wonderlogs) {
    Y_UNIT_TEST(ParseSpotterCommonStats) {
        TLogSpotter::TSpotterActivationInfo spotterActivationInfo;
        UNIT_ASSERT(
            google::protobuf::TextFormat::ParseFromString(PROTO_SPOTTER_ACTIVATION_INFO, &spotterActivationInfo));

        TWonderlog::TSpotter::TCommonStats actual, expected;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_SPOTTER_COMMON_STATS, &expected));
        ParseSpotterCommonStats(actual, spotterActivationInfo);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseApplicationInfo) {
        TUniproxyPrepared uniproxyPrepared;
        UNIT_ASSERT(
            google::protobuf::TextFormat::ParseFromString(PROTO_UNIPROXY_PREPARED_APP_INFO, &uniproxyPrepared));

        TWonderlog actual, expected;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_WONDERLOG_APP_INFO, &expected));
        ParseUniproxyPrepared(actual, uniproxyPrepared);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseTestIdsFilledVoiceInput) {
        TUniproxyPrepared uniproxyPrepared;
        UNIT_ASSERT(
            google::protobuf::TextFormat::ParseFromString(PROTO_UNIPROXY_PREPARED_FILLED_VOICE_INPUT, &uniproxyPrepared));

        TWonderlog actual, expected;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_WONDERLOG_VOICE_INPUT, &expected));
        ParseUniproxyPrepared(actual, uniproxyPrepared);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseTestIdsEmptyVoiceInput) {
        TUniproxyPrepared uniproxyPrepared;
        UNIT_ASSERT(
            google::protobuf::TextFormat::ParseFromString(PROTO_UNIPROXY_PREPARED_EMPTY_VOICE_INPUT, &uniproxyPrepared));

        TWonderlog actual, expected;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_WONDERLOG_EMPTY_VOICE_INPUT, &expected));
        ParseUniproxyPrepared(actual, uniproxyPrepared);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }
}

} // namespace
