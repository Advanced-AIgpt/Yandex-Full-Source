#include <alice/wonderlogs/sdk/utils/speechkit_utils.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/library/experiments/utils.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NWonderlogs;
using namespace google::protobuf;

namespace {

const TString PROTO_SPEECHKIT_REQUEST_TEXT_EVENT = R"(
Header {
    RequestId: "d4fbe418-849d-4e8e-a1da-8bcb48928e01"
    PrevReqId: "2adb06c6-a7ce-4139-ad81-42d7d2390602"
    SequenceNumber: 48
    RefMessageId: "c8acc19e-2311-4d4b-bc56-b0169940b545"
    SessionId: "e49342e1-899f-4d18-b1a9-2132bbf7b0a9"
}
Application {
    AppId: "ru.yandex.searchplugin"
    AppVersion: "21.111"
    OsVersion: "9"
    Platform: "android"
    Uuid: "6a58ac56629e4138ab1db2289b8793e9"
    DeviceId: "bc70cb0242d2499b7702c4b6e5a7d7f9"
    Lang: "ru-RU"
    ClientTime: "20211212T025958"
    Timezone: "Asia/Omsk"
    Epoch: "1639256398"
    DeviceModel: "Redmi Note 8T"
    DeviceManufacturer: "xiaomi"
    DeviceRevision: ""
}
Request {
    Event {
        Type: text_input
        Text: "\320\242\321\213 \320\261\321\203\320\264\320\265\321\210\321\214    \320\220\320\262\320\260\320\273\320\276\321\200\320\260\320\271    \320\277\320\276\320\275\321\217\321\202\320\275\320\276"
        Name: ""
    }	
    Location {	
        Lat: 43.2593232	
        Lon: 76.9268675	
        Accuracy: 25.29999924	
        Recency: 39344	
    }	
    Experiments {	
        Storage {	
            key: "activation_search_redirect_experiment"	
            value {	
                String: "1"	
            }	
        }
        Storage {
        key: "afisha_poi_events"
            value {
                String: "1"
            }
        }
    }
    DeviceState {
        SoundLevel: 0
        SoundMuted: false
        IsDefaultAssistant: false
    }	
    AdditionalOptions {	
        BassOptions {	
            UserAgent: "Mozilla/5.0 (Linux; arm_64; Android 9; Redmi Note 8T) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.85 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.111.1"	
            FiltrationLevel: 1	
            ClientIP: "37.99.36.233"	
            ScreenScaleFactor: 2.75	
        }	
        SupportedFeatures: "reader_app_tts"	
        SupportedFeatures: "whocalls"	
        SupportedFeatures: "div_cards"	
        SupportedFeatures: "messengers_calls"	
        SupportedFeatures: "open_link_search_viewport"	
        SupportedFeatures: "open_link_yellowskin"	
        UnsupportedFeatures: "pwd_app_manager"	
        UnsupportedFeatures: "cloud_ui"	
        UnsupportedFeatures: "bonus_cards_camera"	
        UnsupportedFeatures: "pedometer"	
        UnsupportedFeatures: "supports_device_local_reminders"	
        UnsupportedFeatures: "whocalls_call_blocking"	
        UnsupportedFeatures: "bonus_cards_list"	
        DivKitVersion: "2.3"	
        YandexUID: "4340979321637685657"	
        ServerTimeMs: 1639256399613	
        AppInfo: "eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0="	
        DoNotUseUserLogs: false	
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
        ICookie: "2243500680643228965"	
        Expboxes: "341894,0,20;341896,0,10;341906,0,74;341907,0,38;341918,0,28;341921,0,19;341933,0,63;341939,0,89;341942,0,7;341926,0,53;317744,0,88;315374,0,16;457894,0,2;467484,0,27;466622,0,63;315580,0,11;330999,0,67;436514,0,52"	
    }	
    VoiceSession: false	
    ResetSession: false	
    TestIDs: 412802	
    TestIDs: 423296	
    TestIDs: 432016	
    LaasRegion {	
        fields {	
            key: "city_id"	
            value {	
                number_value: 162	
            }	
        }	
        fields {	
            key: "country_id_by_ip"	
            value {	
                number_value: 159	
            }	
        }	
        fields {	
            key: "regular_coordinates"	
            value {	
                list_value {	
                    values {	
                        struct_value {	
                            fields {	
                                key: "lat"	
                                value {	
                                    number_value: 43.237527	
                                }	
                            }	
                            fields {	
                                key: "lon"	
                                value {	
                                    number_value: 76.893519	
                                }	
                            }	
                            fields {	
                                key: "type"	
                                value {	
                                    number_value: 1	
                                }	
                            }	
                        }
                    }	
                }
            }
        }
        fields {
            key: "should_update_cookie"
            value {
                bool_value: false
            }	
        }
    }
    RawPersonalData: "{}"
}
ContactsProto: ""
)";

const TString PROTO_SPEECHKIT_REQUEST_SUGGESTED_EVENT = R"(
Header {
    RequestId: "2a5cd1a8-53df-4bbb-9338-2b9253f94b0d"
    PrevReqId: "60166694-c06f-41f2-a764-39ef478ccc16"
    SequenceNumber: 657
    DialogId: "f80f9b78-18cf-4a91-9d1b-96e32dfc52e0"
    RefMessageId: "c7ee2881-56cf-4447-9c51-2d43992ac68a"
    SessionId: "be8598f5-804a-430b-ac4a-2cac30513694"
}
Application {
    AppId: "ru.yandex.searchplugin"
    AppVersion: "21.114"
    OsVersion: "11"
    Platform: "android"
    Uuid: "7c3d72c3d3ed4556b455980d1b4a8e59"
    DeviceId: "b1c79d85ff16cc94f305c97237c1d3b4"
    Lang: "ru-RU"
    ClientTime: "20211211T235934"
    Timezone: "Europe/Moscow"
    Epoch: "1639256374"
    DeviceModel: "SM-A125F"
    DeviceManufacturer: "samsung"
    DeviceRevision: ""
}
Request {
    Event {
        Type: suggested_input
        Text: "\320\224\320\260"
        Name: ""
    }
    Location {
        Lat: 43.354225
        Lon: 43.94265333
        Accuracy: 2.200000048
        Recency: 14909
    }
    Experiments {
        Storage {
            key: "activation_search_redirect_experiment"
            value {
                String: "1"
            }
        }
        Storage {
            key: "afisha_poi_events"
            value {
                String: "1"
            }
        }
    }
    DeviceState {
        SoundLevel: 1
        SoundMuted: false
        IsDefaultAssistant: false
    }
    AdditionalOptions {
        BassOptions {
            UserAgent: "Mozilla/5.0 (Linux; arm_64; Android 11; SM-A125F) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.85 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.114.1"
            FiltrationLevel: 1
            ClientIP: "85.173.126.65"
            ScreenScaleFactor: 1.875
        }
        SupportedFeatures: "whocalls_call_blocking"
        SupportedFeatures: "cloud_push_implementation"
        SupportedFeatures: "bonus_cards_list"
        SupportedFeatures: "open_link_turbo_app"
        SupportedFeatures: "open_yandex_auth"
        SupportedFeatures: "music_sdk_client"
        UnsupportedFeatures: "cloud_ui"
        UnsupportedFeatures: "pedometer"
        UnsupportedFeatures: "supports_device_local_reminders"
        DivKitVersion: "2.3"
        YandexUID: "9741213511628578109"
        ServerTimeMs: 1639256399419
        AppInfo: "eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0="
        DoNotUseUserLogs: false
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
        ICookie: "5966103850847343358"
        Expboxes: "341893,0,1;341897,0,81;341906,0,82;341907,0,10;341918,0,43;341921,0,77;341935,0,1;341941,0,95;341942,0,19;341928,0,43;317744,0,60;315365,0,35;457894,0,73;462790,0,78;466622,0,62;315615,0,51;330999,0,94;436514,0,70"
    }
    VoiceSession: true
    ResetSession: false
    TestIDs: 412802
    TestIDs: 423296
    TestIDs: 432016
    LaasRegion {
        fields {
            key: "city_id"
            value {
                number_value: 100586
            }
        }
        fields {
            key: "country_id_by_ip"
            value {
                number_value: 225
            }
        }
        fields {
            key: "regular_coordinates"
            value {
                list_value {
                    values {
                        struct_value {
                            fields {
                                key: "lat"
                                value {
                                    number_value: 43.354443
                                }
                            }
                            fields {
                                key: "lon"
                                value {
                                    number_value: 43.942869
                                }
                            }
                            fields {
                                key: "type"
                                value {
                                    number_value: 1
                                }
                            }
                        }
                    }
                    values {
                        struct_value {
                            fields {
                                key: "lat"
                                value {
                                    number_value: 43.480276
                                }
                            }
                            fields {
                                key: "lon"
                                value {
                                    number_value: 43.598461
                                }
                            }
                            fields {
                                key: "type"
                                value {
                                    number_value: 0
                                }
                            }
                        }
                    }
                    values {
                        struct_value {
                            fields {
                                key: "lat"
                                value {
                                    number_value: 43.361409
                                }
                            }
                            fields {
                                key: "lon"
                                value {
                                    number_value: 43.947626
                                }
                            }
                            fields {
                                key: "type"
                                value {
                                    number_value: 0
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    RawPersonalData: "{}"
}
ContactsProto: ""
)";

const TString PROTO_SPEECHKIT_REQUEST_VOICE_EVENT1 = R"(
Header {
    RequestId: "9097376a-b433-42dc-ab58-0595e188ba53"
    PrevReqId: "a1aa5419-dec4-48c7-ab02-e2783a774488"
    SequenceNumber: 109
}
Application {
    DeviceId: "469ae81646a6e84083147ee0e42c46cb"
    Lang: "ru-RU"
    ClientTime: "20211211T235244"
    Timezone: "Europe/Moscow"
    Epoch: "1639255964"
}
Request {
    Event {
        Type: voice_input
        Name: ""
    }
    Location {
        Lat: 48.74995422
        Lon: 44.50056839
        Accuracy: 140
        Recency: 78324
    }
    DeviceState {
        SoundLevel: 7
        SoundMuted: false
        IsDefaultAssistant: false
    }
    AdditionalOptions {
        OAuthToken: "AQAAAAAvNhQZAAJ-IaZ********************"
        BassOptions {
            UserAgent: "Mozilla/5.0 (Linux; arm_64; Android 9; SM-N950F) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.4577.82 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.90.1"
            FiltrationLevel: 1
            Cookies: "..."
            ScreenScaleFactor: 4.5
            RegionId: 38
        }
        SupportedFeatures: "reader_app_tts"
        SupportedFeatures: "whocalls"
        SupportedFeatures: "div_cards"
        SupportedFeatures: "music_sdk_client"
        UnsupportedFeatures: "cloud_ui"
        UnsupportedFeatures: "bonus_cards_camera"
        UnsupportedFeatures: "pedometer"
        UnsupportedFeatures: "whocalls_call_blocking"
        UnsupportedFeatures: "bonus_cards_list"
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
    ResetSession: false
    ActivationType: "auto_listening"
}
)";

const TString PROTO_SPEECHKIT_REQUEST_VOICE_EVENT2 = R"(
Header {
    RequestId: "9097376a-b433-42dc-ab58-0595e188ba53"
    PrevReqId: "a1aa5419-dec4-48c7-ab02-e2783a774488"
    SequenceNumber: 109
}
Application {
    DeviceId: "469ae81646a6e84083147ee0e42c46cb"
    Lang: "ru-RU"
    ClientTime: "20211211T235244"
    Timezone: "Europe/Moscow"
    Epoch: "1639255964"
}
Request {
    Event {
        Type: voice_input
        HypothesisNumber: 111
        EndOfUtterance: true
        AsrResult {
            Confidence: 1
            Words {
                Value: "\320\260\320\273\320\270\321\201\320\260"
            }
            Words {
                Value: "\321\202\321\213"
            }
            Words {
                Value: "\320\267\320\275\320\260\320\265\321\210\321\214"
            }
            Normalized: "\320\220\320\273\320\270\321\201\320\260, \321\202\321\213 \320\267\320\275\320\260\320\265\321\210\321\214?"
        }
        Name: ""
    }
    Location {
        Lat: 48.74995422
        Lon: 44.50056839
        Accuracy: 140
        Recency: 78324
    }
    DeviceState {
        SoundLevel: 7
        SoundMuted: false
        IsDefaultAssistant: false
    }
    AdditionalOptions {
        OAuthToken: "AQAAAAAvNhQZAAJ-IaZ********************"
        BassOptions {
            UserAgent: "Mozilla/5.0 (Linux; arm_64; Android 9; SM-N950F) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/93.0.4577.82 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.90.1"
            FiltrationLevel: 1
            Cookies: "..."
            ScreenScaleFactor: 4.5
            RegionId: 38
        }
        SupportedFeatures: "reader_app_tts"
        SupportedFeatures: "whocalls"
        SupportedFeatures: "div_cards"
        SupportedFeatures: "music_sdk_client"
        UnsupportedFeatures: "cloud_ui"
        UnsupportedFeatures: "bonus_cards_camera"
        UnsupportedFeatures: "pedometer"
        UnsupportedFeatures: "whocalls_call_blocking"
        UnsupportedFeatures: "bonus_cards_list"
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
    ResetSession: false
    ActivationType: "auto_listening"
    EnvironmentState {
        Devices {
            Application {
                AppId: "aliced"
                AppVersion: "1.0"
                OsVersion: ""
                Platform: ""
                Uuid: "e971d3ac7722a7cb5ef57882fdae813b"
                DeviceId: "LP00000000000009757900004c178c9e"
                Lang: ""
                ClientTime: ""
                Timezone: ""
                Epoch: ""
                DeviceModel: "yandexmicro"
                DeviceManufacturer: ""
                DeviceColor: ""
                DeviceRevision: ""
                QuasmodromGroup: ""
                QuasmodromSubgroup: ""
            }
            SupportedFeatures: "relative_volume_change"
            SpeakerDeviceState {
                DeviceId: ""
                SoundLevel: 0
                SoundMuted: false
                IsTvPluggedIn: false
                AlarmsState: ""
                MicsMuted: false
                IsDefaultAssistant: false
                SoundMaxLevel: 0
                DeviceSubscriptionState {
                    Subscription: yandex_subscription
                }
            }
        }
    }
}
)";

const TString PROTO_SPEECHKIT_REQUEST_SERVER_EVENT = R"(
Header {
    RequestId: "75248ed2-18a3-4ec1-9f6b-798c4bfe283e"
    PrevReqId: "575f06c9-65e5-40a8-b342-54c48807346f"
    SequenceNumber: 811
    RefMessageId: "75248ed2-18a3-4ec1-9f6b-798c4bfe283e"
    SessionId: "f3504659-49d3-42a6-b514-aee7e7467f73"
}
Application {
    AppId: "ru.yandex.quasar.app"
    AppVersion: "1.0"
    OsVersion: "6.0.1"
    Platform: "android"
    Uuid: "24c08863948e7fabe26ea797a20fbf9c"
    DeviceId: "041079028418181d0650"
    Lang: "ru-RU"
    ClientTime: "20211211T235958"
    Timezone: "Europe/Moscow"
    Epoch: "1639256398"
    DeviceModel: "Station"
    DeviceManufacturer: "Yandex"
    DeviceRevision: ""
    QuasmodromGroup: "production"
    QuasmodromSubgroup: "production"
}
Request {
    Event {
        Type: server_action
        Payload {
            fields {
                key: "@recovery_callback"
                value {
                    struct_value {
                        fields {
                            key: "ignore_answer"
                            value {
                                bool_value: false
                            }
                        }
                        fields {
                            key: "is_led_silent"
                            value {
                                bool_value: false
                            }
                        }
                        fields {
                            key: "name"
                            value {
                                string_value: "music_thin_client_recovery"
                            }
                        }
                        fields {
                            key: "payload"
                            value {
                                struct_value {
                                    fields {
                                        key: "@request_id"
                                        value {
                                            string_value: "575f06c9-65e5-40a8-b342-54c48807346f"
                                        }
                                    }
                                    fields {
                                        key: "@scenario_name"
                                        value {
                                            string_value: "HollywoodMusic"
                                        }
                                    }
                                    fields {
                                        key: "playback_context"
                                        value {
                                            struct_value {
                                                fields {
                                                    key: "content_id"
                                                    value {
                                                        struct_value {
                                                            fields {
                                                                key: "id"
                                                                value {
                                                                    string_value: "activity:wake-up"
                                                                }
                                                            }
                                                            fields {
                                                                key: "ids"
                                                                value {
                                                                    list_value {
                                                                        values {
                                                                            string_value: "activity:wake-up"
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                            fields {
                                                                key: "type"
                                                                value {
                                                                    string_value: "Radio"
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    fields {
                                        key: "radio"
                                        value {
                                            struct_value {
                                                fields {
                                                    key: "batch_id"
                                                    value {
                                                        string_value: "b922d045-f8dc-48f4-8bbd-fa1b9d978f33.CyuE"
                                                    }
                                                }
                                                fields {
                                                    key: "session_id"
                                                    value {
                                                        string_value: "SuoZ-czphoqqS4hChm6-06Ca"
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        fields {
                            key: "type"
                            value {
                                string_value: "server_action"
                            }
                        }
                    }
                }
            }
            fields {
                key: "@request_id"
                value {
                    string_value: "575f06c9-65e5-40a8-b342-54c48807346f"
                }
            }
        }
        Name: "@@mm_stack_engine_get_next"
        IsWarmUp: true
    }
    Location {
        Lat: 59.93894958
        Lon: 30.31563568
        Accuracy: 100000
    }
    Experiments {
        Storage {
            key: "alarm_how_long"
            value {
                String: "1"
            }
        }
        Storage {
            key: "alarm_semantic_frame"
            value {
                String: "1"
            }
        }
        Storage {
            key: "alarm_snooze"
            value {
                String: "1"
            }
        }
    }
    DeviceState {
        DeviceId: "041079028418181d0650"
        SoundLevel: 3
        SoundMuted: false
        IsTvPluggedIn: true
        Video {
            CurrentScreen: "music_player"
            CurrentlyPlaying {
                Progress {
                    Played: 7251
                    Duration: 7875
                }
                RawItem {
                    Type: "movie"
                    ProviderName: "kinopoisk"
                    ProviderItemId: "4bec4e105fcc64e9b1e3d76aae0db1ad"
                    Available: 1
                    CoverUrl2x3: "https://avatars.mds.yandex.net/get-ott/224348/2a000001690543e475e949acb5b65722b11b/320x480"
                    CoverUrl16x9: "https://avatars.mds.yandex.net/get-vh/3542582/3020706213081434205-H5oN5ZEVt4ZoKbI1bDLMBg-1595518634/1920x1080"
                    ThumbnailUrl16x9: "https://avatars.mds.yandex.net/get-vh/3542582/3020706213081434205-H5oN5ZEVt4ZoKbI1bDLMBg-1595518634/640x360"
                    Name: "\320\237\320\270\321\200\320\260\321\202\321\213 \320\232\320\260\321\200\320\270\320\261\321\201\320\272\320\276\320\263\320\276 \320\274\320\276\321\200\321\217: \320\235\320\260 \321\201\321\202\321\200\320\260\320\275\320\275\321\213\321\205 \320\261\320\265\321\200\320\265\320\263\320\260\321\205"
                    Description: "\320\247\320\265\321\202\320\262\320\265\321\200\321\202\321\213\320\271 \321\204\320\270\320\273\321\214\320\274 \320\277\320\270\321\200\320\260\321\202\321\201\320\272\320\276\320\271 \321\201\320\265\321\200\320\270\320\270 \321\200\320\260\321\201\321\201\320\272\320\260\320\267\321\213\320\262\320\260\320\265\321\202 \320\276 \320\277\320\276\320\270\321\201\320\272\320\260\321\205 \320\230\321\201\321\202\320\276\321\207\320\275\320\270\320\272\320\260 \320\274\320\276\320\273\320\276\320\264\320\276\321\201\321\202\320\270. \320\222 \321\215\320\272\321\201\320\277\320\265\320\264\320\270\321\206\320\270\321\216 \320\267\320\260 \321\207\320\260\321\210\320\260\320\274\320\270 \320\276\321\202\320\277\321\200\320\260\320\262\320\273\321\217\321\216\321\202\321\201\321\217 \320\272\320\260\320\277\320\270\321\202\320\260\320\275 \320\221\320\260\321\200\320\261\320\276\321\201\321\201\320\260 (\320\224\320\266\320\265\321\204\321\204\321\200\320\270 \320\240\320\260\321\210), \320\247\320\265\321\200\320\275\320\260\321\217 \320\261\320\276\321\200\320\276\320\264\320\260 (\320\230\321\215\320\275 \320\234\320\260\320\272\321\210\320\265\320\271\320\275), \320\265\320\263\320\276 \320\264\320\276\321\207\321\214 \320\220\320\275\320\266\320\265\320\273\320\270\320\272\320\260 (\320\237\320\265\320\275\320\265\320\273\320\276\320\277\320\260 \320\232\321\200\321\203\321\201) \320\270, \320\272\320\276\320\275\320\265\321\207\320\275\320\276, \320\224\320\266\320\265\320\272 \320\222\320\276\321\200\320\276\320\261\320\265\320\271. \320\222 \321\200\320\276\320\273\320\270 \320\276\321\202\321\206\320\260 \320\224\320\266\320\265\320\272\320\260 \320\262\320\275\320\276\320\262\321\214 \320\232\320\270\321\202 \320\240\320\270\321\207\320\260\321\200\320\264\321\201, \320\260 \321\215\320\277\320\270\320\267\320\276\320\264\320\270\321\207\320\265\321\201\320\272\321\203\321\216 \321\200\320\276\320\273\321\214 \320\267\320\275\320\260\321\202\320\275\320\276\320\271 \320\264\320\260\320\274\321\213 \320\270\320\263\321\200\320\260\320\265\321\202 \320\224\320\266\321\203\320\264\320\270 \320\224\320\265\320\275\321\207. "
                    Duration: 7875
                    Genre: "\320\277\321\200\320\270\320\272\320\273\321\216\321\207\320\265\320\275\320\270\321\217, \320\261\320\276\320\265\320\262\320\270\320\272, \321\204\321\215\320\275\321\202\320\265\320\267\320\270, \320\272\320\276\320\274\320\265\320\264\320\270\321\217"
                    Rating: 7.329999924
                    ReleaseYear: 2011
                    Directors: "\320\240\320\276\320\261 \320\234\320\260\321\200\321\210\320\260\320\273\320\273"
                    Actors: "\320\224\320\266\320\276\320\275\320\275\320\270 \320\224\320\265\320\277\320\277, \320\237\320\265\320\275\320\265\320\273\320\276\320\277\320\260 \320\232\321\200\321\203\321\201, \320\224\320\266\320\265\321\204\321\204\321\200\320\270 \320\240\320\260\321\210"
                    PlayUri: "https://strm.yandex.ru/vh-ottenc-converted/vod-content/4bec4e105fcc64e9b1e3d76aae0db1ad/8093288x1620748327x02bf7fb6-57cf-44e9-bf8e-3f771e6afc8a/dash-cenc/ysign1=54d6a5e8e1fd224329fbb9fc3975cfbef37dca595b6919e9d7140b1764b55806,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=61bf85cb/sdr_hd_avc_aac.mpd"
                    ProviderInfo {
                        Type: "movie"
                        ProviderName: "kinopoisk"
                        ProviderItemId: "4bec4e105fcc64e9b1e3d76aae0db1ad"
                        Available: 1
                    }
                    MinAge: 12
                    SkippableFragments {
                        StartTime: 1
                        EndTime: 42
                        Type: "intro"
                    }
                    SkippableFragments {
                        StartTime: 7332
                        EndTime: 7875
                        Type: "credits"
                    }
                    AudioStreams {
                        Title: "\320\240\321\203\321\201\321\201\320\272\320\270\320\271"
                        Language: "rus"
                        Index: 1
                        Suggest: "\320\222\320\272\320\273\321\216\321\207\320\270 \321\200\321\203\321\201\321\201\320\272\321\203\321\216 \320\276\320\267\320\262\321\203\321\207\320\272\321\203"
                    }
                    AudioStreams {
                        Title: "\320\220\320\275\320\263\320\273\320\270\320\271\321\201\320\272\320\270\320\271"
                        Language: "eng"
                        Index: 2
                        Suggest: "\320\222\320\272\320\273\321\216\321\207\320\270 \320\260\320\275\320\263\320\273\320\270\320\271\321\201\320\272\321\203\321\216 \320\276\320\267\320\262\321\203\321\207\320\272\321\203"
                    }
                    Subtitles {
                        Title: "\320\222\321\213\320\272\320\273\321\216\321\207\320\265\320\275\321\213"
                        Language: "off"
                        Index: 3
                        Suggest: "\320\222\321\213\320\272\320\273\321\216\321\207\320\270 \321\201\321\203\320\261\321\202\320\270\321\202\321\200\321\213"
                    }
                    Subtitles {
                        Title: "\320\240\321\203\321\201\321\201\320\272\320\270\320\265"
                        Language: "rus"
                        Index: 4
                        Suggest: "\320\222\320\272\320\273\321\216\321\207\320\270 \321\201\321\203\320\261\321\202\320\270\321\202\321\200\321\213"
                    }
                    Entref: "entnext=ruw1494301"
                    PlayerRestrictionConfig {
                        SubtitlesButtonEnable: 1
                    }
                    AgeLimit: "12"
                }
                AudioLanguage: "ru"
                LastPlayTimestamp: 1639077581625
            }
            LastPlayTimestamp: 1639077581625
            Player {
                Pause: true
            }
        }
        AlarmsState: "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20211211T043000Z\r\nDTEND:20211211T043000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20211211T043000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n"
        AlarmState {
            ICalendar: "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20211211T043000Z\r\nDTEND:20211211T043000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20211211T043000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n"
            CurrentlyPlaying: false
            MaxSoundLevel: 7
        }
        DeviceConfig {
            ContentSettings: medium
            Spotter: "alisa"
            ChildContentSettings: children
        }
        Timers {
        }
        LastWatched {
            RawMovies {
                ProviderName: "kinopoisk"
                ProviderItemId: "45a175cb535fcda68053875ca76dedb9"
                Progress {
                    Played: 1188
                    Duration: 6852
                }
                Timestamp: 1638647441
                PlayUri: "https://strm.yandex.ru/vh-ottenc-converted/vod-content/45a175cb535fcda68053875ca76dedb9/8480413x1628683416xc2f76e9d-4db4-426e-9465-1f571d776884/dash-cenc/ysign1=7808dee8bceecd2f3b2ac387bc6a63c401ae5415242b28167882f295fa713281,abcID=1558,from=ya-station,pfx,region=225,sfx,ts=61b8ebff/sdr_hd_avc_aac.mpd"
                AudioLanguage: "ru"
            }
            RawMovies {
                ProviderName: "kinopoisk"
                ProviderItemId: "463ea1ab954bd0f0b85d1e4c0bb33dfd"
                Progress {
                    Played: 158
                    Duration: 8708
                }
                Timestamp: 1638716171
                PlayUri: "https://strm.yandex.ru/vh-ottenc-converted/vod-content/463ea1ab954bd0f0b85d1e4c0bb33dfd/7727743x1609507056xa91f01e5-a65e-412f-9a06-8a53df7c774e/dash-cenc/ysign1=6ec0f7fba8dbe996410e0a703c22c04c4a1f85db8f0ee3faa3ea0e45f67d0118,abcID=1558,from=ya-station,pfx,region=225,sfx,ts=61ba015f/sdr_hd_avc_aac.mpd"
                AudioLanguage: "ru"
            }
            RawTvShows {
                Item {
                    ProviderName: "kinopoisk"
                    ProviderItemId: "4649cec609af1691bfde8e8a6fa8f0ef"
                    Episode: 1
                    Season: 1
                    Progress {
                        Played: 91
                        Duration: 2815
                    }
                    Timestamp: 1638729617
                    PlayUri: "https://strm.yandex.ru/vh-ottenc-converted/vod-content/4649cec609af1691bfde8e8a6fa8f0ef/7838580x1612964871x8353f778-ade6-4435-9e69-7a8bf33fc51c/dash-cenc/ysign1=46906897ffc8d592c4c683a7a91d12d7c2013609183a2d266197621d80789a48,abcID=1558,from=ya-station,pfx,region=225,sfx,ts=61ba362c/sdr_hd_avc_aac.mpd"
                    AudioLanguage: "ru"
                }
                TvShowItem {
                    ProviderName: "kinopoisk"
                    ProviderItemId: "440c3609669e63fdb685c647cd5a68c3"
                    Progress {
                        Played: 91
                        Duration: 2815
                    }
                    Timestamp: 1638729617
                    AudioLanguage: "ru"
                }
            }
            RawVideos {
                ProviderName: "youtube"
                ProviderItemId: "vSX59L7crpA"
                Progress {
                    Duration: 2936
                }
                Timestamp: 1638996218
                PlayUri: "youtube://vSX59L7crpA"
            }
            RawVideos {
                ProviderName: "youtube"
                ProviderItemId: "a165yY_CTqg"
                Progress {
                    Played: 5
                    Duration: 43
                }
                Timestamp: 1638996246
                PlayUri: "youtube://a165yY_CTqg"
            }
        }
        AudioPlayer {
            PlayerState: Idle
            OffsetMs: 0
            CurrentlyPlaying {
                StreamId: "79587131"
                LastPlayTimestamp: 1639256398764
                Title: "Dancing On Dangerous"
                SubTitle: "Imanbek"
                StreamType: "Track"
            }
            ScenarioMeta {
                key: "@scenario_name"
                value: "HollywoodMusic"
            }
            ScenarioMeta {
                key: "owner"
                value: "music"
            }
            ScenarioMeta {
                key: "what_is_playing_answer"
                value: "Imanbek, Sean Paul, Sofia Reyes, \320\277\320\265\321\201\320\275\321\217 \"Dancing On Dangerous\""
            }
            LastPlayTimestamp: 1639256398764
            DurationMs: 0
            LastStopTimestamp: 0
            PlayedMs: 0
        }
        Bluetooth {
        }
        InternetConnection {
            Type: Wifi_2_4GHz
            Current {
                Ssid: "homeasus"
                Bssid: "2c:4d:54:6c:90:80"
                Channel: 6
            }
            Neighbours {
                Ssid: "homeasus"
                Bssid: "2c:4d:54:6c:90:80"
                Channel: 6
            }
            Neighbours {
                Ssid: "rr"
                Bssid: "f4:ec:38:a5:73:84"
                Channel: 11
            }
        }
        MicsMuted: false
        Screen {
            SupportedScreenResolutions: video_format_SD
            SupportedScreenResolutions: video_format_HD
            HdcpLevel: current_HDCP_level_none
        }
        RcuState {
            IsRcuConnected: false
        }
        SoundMaxLevel: 10
        ActiveActions {
        }
    }
    AdditionalOptions {
        BassOptions {
            UserAgent: "aba"
            ClientIP: "caba"
        }
        SupportedFeatures: "relative_volume_change"
        SupportedFeatures: "mordovia_webview"
        YandexUID: "19271160"
        ServerTimeMs: 1639256398945
        AppInfo: "eyJicm93c2VyTmFtZSI6Ik90aGVyQXBwbGljYXRpb25zIiwiZGV2aWNlVHlwZSI6InN0YXRpb24iLCJkZXZpY2VNb2RlbCI6InlhbmRleHN0YXRpb24iLCJtb2JpbGVQbGF0Zm9ybSI6ImFuZHJvaWQifQ=="
        DoNotUseUserLogs: false
        Puid: "19271160"
        ICookie: "22813390017111720"
        Expboxes: "466516,0,36;461330,0,6;470915,0,51;341892,0,43;341898,0,49;341904,0,78;341907,0,80;341915,0,76;341921,0,21;341933,0,48;341940,0,99;341942,0,26;341927,0,25;329372,0,53;329376,0,45;329382,0,87;336916,0,44;336930,0,1;336934,0,16;336938,0,51;336943,0,88;336955,0,51;336958,0,41;336963,0,2;336969,0,75;336972,0,63;336978,0,36;336980,0,80;336984,0,41;336992,0,33;336994,0,35;336999,0,28;337003,0,34;458326,0,1;315360,0,66;315374,0,21;457894,0,87;462787,0,18;466634,0,73;330999,0,18"
    }
    VoiceSession: true
    TestIDs: 348361
    TestIDs: 409426
    TestIDs: 439312
    TestIDs: 466634
    LaasRegion {
        fields {
            key: "city_id"
            value {
                number_value: 2
            }
        }
        fields {
            key: "country_id_by_ip"
            value {
                number_value: 225
            }
        }
    }
    RawPersonalData: "{\"/v1/personality/profile/alisa/kv/proactivity_history\":\"{}\",\"/v2/personality/profile/addresses/home\":{\"address_id\":\"home\",\"address_line\":\"\320\240\320\276\321\201\321\201\320\270\321\217, \320\233\320\265\320\275\320\270\320\275\320\263\321\200\320\260\320\264\321\201\320\272\320\260\321\217 \320\276\320\261\320\273\320\260\321\201\321\202\321\214, \320\242\320\276\321\201\320\275\320\265\320\275\321\201\320\272\320\276\320\265 \320\263\320\276\321\200\320\276\320\264\321\201\320\272\320\276\320\265 \320\277\320\276\321\201\320\265\320\273\320\265\320\275\320\270\320\265, \320\242\320\276\321\201\320\275\320\276\",\"address_line_short\":\"\320\242\320\276\321\201\320\275\320\276\",\"created\":\"2020-01-04T14:48:05.696000+00:00\",\"last_used\":\"2020-01-04T14:48:05.696000+00:00\",\"latitude\":59.540664,\"longitude\":30.877719,\"mined_attributes\":[],\"modified\":\"2020-01-04T14:48:05.696000+00:00\",\"tags\":[],\"title\":\"Home\"},\"/v2/personality/profile/addresses/work\":{\"address_id\":\"work\",\"address_line\":\"\320\240\320\276\321\201\321\201\320\270\321\217, \320\241\320\260\320\275\320\272\321\202-\320\237\320\265\321\202\320\265\321\200\320\261\321\203\321\200\320\263, \320\230\320\262\320\260\320\275\320\276\320\262\321\201\320\272\320\260\321\217 \321\203\320\273\320\270\321\206\320\260, 31\320\2722\",\"address_line_short\":\"\320\230\320\262\320\260\320\275\320\276\320\262\321\201\320\272\320\260\321\217 \321\203\320\273\320\270\321\206\320\260, 31\320\2722\",\"created\":\"2020-01-04T14:48:11.960000+00:00\",\"last_used\":\"2020-01-04T14:48:11.960000+00:00\",\"latitude\":59.87210846,\"longitude\":30.43280411,\"mined_attributes\":[],\"modified\":\"2021-10-01T10:41:12.240000+00:00\",\"tags\":[\"work\"],\"title\":\"\320\230\320\262\320\260\320\275\320\276\320\262\321\201\320\272\320\260\321\217 \321\203\320\273\320\270\321\206\320\260, 31\320\2722\"}}"
    SmartHomeInfo {
        Payload {
            Devices {
                Id: "6a9c719a-0d18-441c-9037-cc87f12ce711"
                Name: "\320\257\320\275\320\264\320\265\320\272\321\201 \320\241\321\202\320\260\320\275\321\206\320\270\321\217"
                Type: "devices.types.smart_speaker.yandex.station"
                QuasarInfo {
                    DeviceId: "041079028418181d0650"
                    Platform: "yandexstation"
                }
                Created: 1638387813
            }
        }
    }
    NotificationState {
    }
    ActivationType: "directive"
}
)";

const TString PROTO_SPEECHKIT_REQUEST_MUSIC_EVENT = R"(
Header {
    RequestId: "d2c5e28a-00ec-4465-acfc-414ff14e0611"
    PrevReqId: "90909244-a2f9-41bb-9079-894c508c2788"
    SequenceNumber: 225
    RefMessageId: "6a778bf9-9ca3-47f9-bed9-e0467a2828e0"
    SessionId: "54ea405c-3373-4c47-9465-720fb1c1522f"
}
Application {
    AppId: "ru.yandex.searchplugin"
    AppVersion: "21.114"
    OsVersion: "10"
    Platform: "android"
    Uuid: "84739dd5bb8c41f6b2513c730e6e0054"
    DeviceId: "1ee9abd6fda8b175381e3497034a4ffc"
    Lang: "ru-RU"
    ClientTime: "20211212T000004"
    Timezone: "Europe/Moscow"
    Epoch: "1639256404"
    DeviceModel: "STK-LX1"
    DeviceManufacturer: "HONOR"
    DeviceRevision: ""
}
Request {
    Event {
        Type: music_input
        MusicResult {
            Result: "not-music"
        }
        Name: ""
    }
    Location {
        Lat: 43.9037818
        Lon: 39.3348873
        Accuracy: 22.5
        Recency: 11458
    }
    Experiments {
        Storage {
            key: "activation_search_redirect_experiment"
            value {
                String: "1"
            }
        }
        Storage {
            key: "afisha_poi_events"
            value {
                String: "1"
            }
        }
        Storage {
            key: "ambient_sound"
            value {
                String: "1"
            }
        }
        Storage {
            key: "ambient_sounds_and_podcasts"
            value {
                String: "1"
            }
        }
        Storage {
            key: "authorized_personal_playlists"
            value {
                String: "1"
            }
        }
    }
    DeviceState {
        SoundLevel: 10
        SoundMuted: false
        Music {
            CurrentlyPlaying {
                TrackId: "38633715"
                RawTrackInfo {
                    fields {
                        key: "albums"
                        value {
                            list_value {
                                values {
                                    struct_value {
                                        fields {
                                            key: "artists"
                                            value {
                                                list_value {
                                                    values {
                                                        struct_value {
                                                            fields {
                                                                key: "composer"
                                                                value {
                                                                    bool_value: false
                                                                }
                                                            }
                                                            fields {
                                                                key: "cover"
                                                                value {
                                                                    struct_value {
                                                                        fields {
                                                                            key: "prefix"
                                                                            value {
                                                                                string_value: "c6d507c7.p.41075/"
                                                                            }
                                                                        }
                                                                        fields {
                                                                            key: "type"
                                                                            value {
                                                                                string_value: "from-artist-photos"
                                                                            }
                                                                        }
                                                                        fields {
                                                                            key: "uri"
                                                                            value {
                                                                                string_value: "avatars.yandex.net/get-music-content/33216/c6d507c7.p.41075/%%"
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                            fields {
                                                                key: "genres"
                                                                value {
                                                                    list_value {
                                                                    }
                                                                }
                                                            }
                                                            fields {
                                                                key: "id"
                                                                value {
                                                                    string_value: "41075"
                                                                }
                                                            }
                                                            fields {
                                                                key: "name"
                                                                value {
                                                                    string_value: "\320\232\320\230\320\235\320\236"
                                                                }
                                                            }
                                                            fields {
                                                                key: "various"
                                                                value {
                                                                    bool_value: false
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "buy"
                                            value {
                                                list_value {
                                                }
                                            }
                                        }
                                        fields {
                                            key: "coverUri"
                                            value {
                                                string_value: "avatars.yandex.net/get-music-content/95061/4f3808a0.a.5307396-3/%%"
                                            }
                                        }
                                        fields {
                                            key: "genre"
                                            value {
                                                string_value: "rusrock"
                                            }
                                        }
                                        fields {
                                            key: "year"
                                            value {
                                                number_value: 2018
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "artists"
                        value {
                            list_value {
                                values {
                                    struct_value {
                                        fields {
                                            key: "composer"
                                            value {
                                                bool_value: false
                                            }
                                        }
                                        fields {
                                            key: "cover"
                                            value {
                                                struct_value {
                                                    fields {
                                                        key: "prefix"
                                                        value {
                                                            string_value: "c6d507c7.p.41075/"
                                                        }
                                                    }
                                                    fields {
                                                        key: "type"
                                                        value {
                                                            string_value: "from-artist-photos"
                                                        }
                                                    }
                                                    fields {
                                                        key: "uri"
                                                        value {
                                                            string_value: "avatars.yandex.net/get-music-content/33216/c6d507c7.p.41075/%%"
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "genres"
                                            value {
                                                list_value {
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "available"
                        value {
                            bool_value: true
                        }
                    }
                    fields {
                        key: "availableForPremiumUsers"
                        value {
                            bool_value: true
                        }
                    }
                    fields {
                        key: "availableFullWithoutPermission"
                        value {
                            bool_value: false
                        }
                    }
                    fields {
                        key: "coverUri"
                        value {
                            string_value: "avatars.yandex.net/get-music-content/95061/4f3808a0.a.5307396-3/%%"
                        }
                    }
                }
            }
            Player {
                Pause: true
                Timestamp: 1639256314
            }
        }
        IsDefaultAssistant: false
    }
    AdditionalOptions {
        BassOptions {
            UserAgent: "Mozilla/5.0 (Linux; arm_64; Android 10; STK-LX1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.85 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.114.1"
            FiltrationLevel: 1
            ClientIP: "31.41.231.14"
            ScreenScaleFactor: 3
        }
        SupportedFeatures: "reader_app_tts"
        SupportedFeatures: "whocalls"
        SupportedFeatures: "cloud_push_implementation"
        SupportedFeatures: "open_link_turbo_app"
        SupportedFeatures: "open_yandex_auth"
        SupportedFeatures: "music_sdk_client"
        UnsupportedFeatures: "cloud_ui"
        UnsupportedFeatures: "bonus_cards_camera"
        UnsupportedFeatures: "pedometer"
        UnsupportedFeatures: "supports_device_local_reminders"
        UnsupportedFeatures: "bonus_cards_list"
        DivKitVersion: "2.3"
        YandexUID: "7198464001627504288"
        ServerTimeMs: 1639256409197
        AppInfo: "eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0="
        DoNotUseUserLogs: false
        Puid: "1466642495"
        Permissions {
            Granted: true
            Name: "location"
        }
        Permissions {
            Granted: true
            Name: "read_contacts"
        }
        Permissions {
            Granted: true
            Name: "call_phone"
        }
        ICookie: "653694750838634051"
        Expboxes: "341894,0,21;341896,0,68;341905,0,35;341908,0,21;341918,0,27;341921,0,69;341934,0,39;341941,0,25;341942,0,77;341929,0,65;329372,0,94;329376,0,72;329381,0,19;336915,0,36;336931,0,50;336937,0,34;336941,0,48;336943,0,6;336954,0,33;336958,0,67;336963,0,53;336968,0,27;336974,0,19;336977,0,67;336980,0,46;336987,0,66;336992,0,35;336994,0,58;336998,0,8;337001,0,45;458325,0,49;317744,0,63;315366,0,29;442531,0,62;469471,0,21;457894,0,34;462788,0,19;466622,0,48;330999,0,9;323500,0,67;436514,0,48"
    }
    VoiceSession: true
    ResetSession: false
    TestIDs: 412802
    TestIDs: 423296
    TestIDs: 432016
    LaasRegion {
        fields {
            key: "city_id"
            value {
                number_value: 10998
            }
        }
        fields {
            key: "country_id_by_ip"
            value {
                number_value: 225
            }
        }
    }
    RawPersonalData: "{}"
}
)";

const TString PROTO_SPEECHKIT_REQUEST_IMAGE_EVENT = R"(
Header {
    RequestId: "5BA04725-6C65-4A0D-914A-3FD86B49B126"
    PrevReqId: "502DEA55-D385-4F75-A378-FADE1A707377"
    SequenceNumber: 10
    RefMessageId: "b98a4f5b-e30a-4b2f-b139-23acf5f3da10"
    SessionId: "ba4b3fb6-d714-46ac-afc6-b68d910c608f"
}
Application {
    AppId: "ru.yandex.mobile.search"
    AppVersion: "2111.5.594"
    OsVersion: "14.7.1"
    Platform: "iphone"
    Uuid: "1356fa4b78784a9c81c366003c5432fc"
    DeviceId: "941497EE-7A7C-4663-974E-2615DA3DC9FD"
    Lang: "ru-RU"
    ClientTime: "20211212T000002"
    Timezone: "Europe/Moscow"
    Epoch: "1639256402"
    DeviceModel: "iPhone"
    DeviceManufacturer: "Apple"
    DeviceRevision: ""
}
Request {
    Event {
        Type: image_input
        Payload {
            fields {
                key: "capture_mode"
                value {
                    string_value: "photo"
                }
            }
            fields {
                key: "img_url"
                value {
                    string_value: "https://avatars.mds.yandex.net/get-alice/1327367/wLOmO0XsmQ1Dx8pj4NizEQ0216/fullocr"
                }
            }
        }
    }
    Location {
        Lat: 55.5461362
        Lon: 37.59306089
        Accuracy: 2000
        Recency: 151
    }
    Experiments {
        Storage {
            key: "activation_search_redirect_experiment"
            value {
                String: "1"
            }
        }
        Storage {
            key: "afisha_poi_events"
            value {
                String: "1"
            }
        }
    }
    AdditionalOptions {
        BassOptions {
            UserAgent: "Mozilla/5.0 (iPhone; CPU iPhone OS 14_7 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.0 YaBrowser/21.11.5.594.10 SA/3 Mobile/15E148 Safari/604.1"
            FiltrationLevel: 0
            ClientIP: "185.13.112.99"
            ScreenScaleFactor: 2
        }
        SupportedFeatures: "open_ibro_settings"
        SupportedFeatures: "whocalls"
        SupportedFeatures: "whocalls_call_blocking"
        SupportedFeatures: "pwd_app_manager"
        SupportedFeatures: "whocalls_message_filtering"
        SupportedFeatures: "cloud_ui"
        DivKitVersion: "2.3.0"
        YandexUID: ""
        ServerTimeMs: 1639256402515
        AppInfo: "eyJicm93c2VyTmFtZSI6IllhbmRleEJyb3dzZXIiLCJkZXZpY2VUeXBlIjoidG91Y2giLCJkZXZpY2VNb2RlbCI6IiIsIm1vYmlsZVBsYXRmb3JtIjoiaXBob25lIn0="
        DoNotUseUserLogs: false
        Puid: "490456318"
        Permissions {
            Name: "location"
            Status: true
        }
        Permissions {
            Name: "push_notifications"
            Status: false
        }
        Permissions {
            Name: "read_contacts"
            Status: false
        }
        ICookie: "130267771638135797"
        Expboxes: "341893,0,38;341896,0,46;341906,0,53;341908,0,2;341916,0,67;341924,0,35;341933,0,39;341939,0,82;341943,0,75;341929,0,87;329372,0,36;329375,0,16;329379,0,12;336931,0,90;336934,0,87;336940,0,5;336944,0,3;336955,0,39;336957,0,75;336964,0,28;336970,0,9;336972,0,26;336979,0,18;336981,0,55;336985,0,58;336991,0,1;336994,0,25;336999,0,40;337004,0,64;458325,0,19;315360,0,34;315366,0,95;469472,0,47;457894,0,57;467482,0,96;466622,0,50;330999,0,69"
    }
    VoiceSession: true
    ResetSession: true
    TestIDs: 412802
    TestIDs: 423296
    TestIDs: 432016
    TestIDs: 444881
    TestIDs: 446307
    LaasRegion {
        fields {
            key: "city_id"
            value {
                number_value: 117443
            }
        }
        fields {
            key: "country_id_by_ip"
            value {
                number_value: 225
            }
        }
    }
    RawPersonalData: "{\"/v1/personality/profile/alisa/kv/proactivity_history\":\"{\\\"RequestCount\\\":\\\"12\\\",\\\"LastStorageUpdateTime\\\":\\\"1621054539\\\"}\"}"
    SmartHomeInfo {
        Payload {
        }
    }
    MegamindCookies: ""
}
MementoData: "ChBCCgoIaAd4iq3ghQZ6AggBEiQKIDEzNTZmYTRiNzg3ODRhOWM4MWMzNjYwMDNjNTQzMmZjEgAaVAoZX19tZWdhbWluZF9fQHN0YWNrX2VuZ2luZRI3CjV0eXBlLmdvb2dsZWFwaXMuY29tL05BbGljZS5OTWVnYW1pbmQuVFN0YWNrRW5naW5lQ29yZSpeCigzYTRkNTY0ZDQ3Yjk5MDU2YzdmODg5Y2YxZGQ4ODgxNTg1OGE3YTcyEJzonwQYnOifBCIoM2E0ZDU2NGQ0N2I5OTA1NmM3Zjg4OWNmMWRkODg4MTU4NThhN2E3Mg=="
ContactsProto: "CgASAm9r"
)";

const TString PROTO_SPEECHKIT_REQUEST = R"*(
Header {
    RequestId: "8b464f68-df7b-485c-a81b-0640051c9f49"
    PrevReqId: "f5309b1a-50ee-474d-ace3-4c7723370bf4"
    SequenceNumber: 84
    RefMessageId: "8b464f68-df7b-485c-a81b-0640051c9f49"
    SessionId: "db6ba473-c073-4ec2-94a7-14d3f650bdee"
}
Application {
    AppId: "aliced"
    AppVersion: "1.0"
    OsVersion: "1.0"
    Platform: "Linux"
    Uuid: "58341b91fe76e4b64fa9604a2d2893f0"
    DeviceId: "FF98F029669E41603B6B4993"
    Lang: "ru-RU"
    ClientTime: "20210828T205955"
    Timezone: "Europe/Moscow"
    Epoch: "1630184395"
    DeviceModel: "yandexmini"
    DeviceManufacturer: "Yandex"
    DeviceRevision: ""
    QuasmodromGroup: "production"
    QuasmodromSubgroup: "production"
}
Request {
    Event {
        Type: voice_input
        HypothesisNumber: 114
        EndOfUtterance: true
        AsrResult {
            Utterance: "\321\207\321\202\320\276 \321\202\320\260\320\272\320\276\320\265 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 \320\277\320\276-\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270?"
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\263\320\260\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276\320\273\320\264\320\265\321\200"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: "\320\247\321\202\320\276 \321\202\320\260\320\272\320\276\320\265 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 \320\277\320\276-\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270?"
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\263\320\260\320\263\320\260\321\207\320\277\321\203\320\273\320\264\320\265\321\200"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\263\320\260\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\321\203\320\273\320\264\320\265\321\200"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\263\320\260\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\321\203\320\273\320\261\320\265\321\200\320\263"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\263\320\260\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276\320\273\320\264\320\260\321\200\320\272"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\273\321\217\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\321\203\320\273\320\264\320\265\321\200"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\321\200\320\276\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\321\203\320\273\320\264\320\265\321\200"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Words {
                Value: "\321\207\321\202\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\202\320\260\320\272\320\276\320\265"
                Confidence: 1
            }
            Words {
                Value: "\320\273\321\217\320\263\320\260\321\207"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276\320\273\320\264\320\265\321\200"
                Confidence: 1
            }
            Words {
                Value: "\320\277\320\276"
                Confidence: 1
            }
            Words {
                Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                Confidence: 1
            }
            Normalized: ""
        }
        AsrResult {
            Utterance: ""
            Confidence: 1
            Normalized: ""
        }
        BiometryScoring {
            Status: "ok"
            RequestId: "8b464f68-df7b-485c-a81b-0640051c9f49"
            GroupId: "aef372b7d1ae0d50ab9f60542314d414"
        }
        BiometryClassification {
            Status: "ok"
            Scores {
                ClassName: "adult"
                Confidence: 0.98682326
                Tag: "children"
            }
            Scores {
                ClassName: "child"
                Confidence: 0.01317676
                Tag: "children"
            }
            Scores {
                ClassName: "female"
                Confidence: 0.053047426
                Tag: "gender"
            }
            Scores {
                ClassName: "male"
                Confidence: 0.9469526
                Tag: "gender"
            }
            Simple {
                ClassName: "adult"
                Tag: "children"
            }
            Simple {
                ClassName: "male"
                Tag: "gender"
            }
        }
        OriginalZeroAsrHypothesisIndex: 0
    }
    Location {
        Lat: 56.12589264
        Lon: 47.39093781
        Accuracy: 140
    }
    Experiments {
        Storage {
            key: "alarm_how_long"
            value {
                String: "1"
            }
        }
        Storage {
            key: "alarm_snooze"
            value {
                String: "1"
            }
        }
        Storage {
            key: "ambient_sound"
            value {
                String: "1"
            }
        }
        Storage {
            key: "audio_bitrate192"
            value {
                String: "1"
            }
        }
        Storage {
            key: "bg_enable_player_next_track_v2"
            value {
                String: "1"
            }
        }
        Storage {
            key: "bg_fresh_granet_experiment=bg_enable_player_next_track_v2"
            value {
                String: "1"
            }
        }
        Storage {
            key: "biometry_like"
            value {
                String: "1"
            }
        }
        Storage {
            key: "biometry_remove"
            value {
                String: "1"
            }
        }
        Storage {
            key: "cachalot_mm_context_save"
            value {
                String: "1"
            }
        }
        Storage {
            key: "change_alarm_sound"
            value {
                String: "1"
            }
        }
        Storage {
            key: "change_alarm_sound_music"
            value {
                String: "1"
            }
        }
        Storage {
            key: "change_alarm_sound_radio"
            value {
                String: "1"
            }
        }
        Storage {
            key: "change_alarm_with_sound"
            value {
                String: "1"
            }
        }
        Storage {
            key: "context_load_apply"
            value {
                String: "1"
            }
        }
        Storage {
            key: "dialog_4178_newcards"
            value {
                String: "1"
            }
        }
        Storage {
            key: "disable_interruption_spotter"
            value {
                String: "1"
            }
        }
        Storage {
            key: "dj_service_for_games_onboarding"
            value {
                String: "1"
            }
        }
        Storage {
            key: "drm_tv_stream"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_biometry_scoring"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_multiroom"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_ner_for_skills"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_partials"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_reminders_todos"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_timers_alarms"
            value {
                String: "1"
            }
        }
        Storage {
            key: "enable_tts_gpu"
            value {
                String: "1"
            }
        }
        Storage {
            key: "ether"
            value {
                String: "https://yandex.ru/portal/station/main"
            }
        }
        Storage {
            key: "fairytale_fallback"
            value {
                String: "1"
            }
        }
        Storage {
            key: "fairytale_search_text_noprefix"
            value {
                String: "1"
            }
        }
        Storage {
            key: "film_gallery"
            value {
                String: "1"
            }
        }
        Storage {
            key: "fm_radio_recommend"
            value {
                String: "1"
            }
        }
        Storage {
            key: "general_conversation"
            value {
                String: "1"
            }
        }
        Storage {
            key: "how_much"
            value {
                String: "1"
            }
        }
        Storage {
            key: "hw_enable_evening_show"
            value {
                String: "1"
            }
        }
        Storage {
            key: "hw_enable_morning_show"
            value {
                String: "1"
            }
        }
        Storage {
            key: "hw_enable_morning_show_good_morning"
            value {
                String: "1"
            }
        }
        Storage {
            key: "hw_music_thin_client"
            value {
                String: "1"
            }
        }
        Storage {
            key: "hw_music_thin_client_playlist"
            value {
                String: "1"
            }
        }
        Storage {
            key: "ignore_trash_classified_results"
            value {
                String: "1"
            }
        }
        Storage {
            key: "iot"
            value {
                String: "1"
            }
        }
        Storage {
            key: "k_schastiyu_dlya_companii"
            value {
                String: "1"
            }
        }
        Storage {
            key: "kv_saas_activation_experiment"
            value {
                String: "1"
            }
        }
        Storage {
            key: "market_beru_disable"
            value {
                String: "1"
            }
        }
        Storage {
            key: "market_disable"
            value {
                String: "1"
            }
        }
        Storage {
            key: "market_orders_status_disable"
            value {
                String: "1"
            }
        }
        Storage {
            key: "medium_ru_explicit_content"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_disable_music"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_disable_protocol_scenario=MordoviaVideoSelection"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_enable_partial_preclassifier"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_enable_player_features"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_enable_protocol_scenario=AliceShow"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_enable_protocol_scenario=HollywoodHardcodedMusic"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_enable_session_reset"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_partial_preclassifier_threshold=0.0322"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mm_protocol_priority_scenario_early_win"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mordovia"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mordovia_long_listening"
            value {
                String: "1"
            }
        }
        Storage {
            key: "mordovia_support_channels"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_biometry"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_check_plus_promo"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_exp__dj_rl@no"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_partials"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_personalization"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_play_fm_radio_on_attempt=2"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_recognizer"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_session"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_show_first_track"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_sing_song"
            value {
                String: "1"
            }
        }
        Storage {
            key: "music_use_websearch"
            value {
                String: "1"
            }
        }
        Storage {
            key: "new_fairytale_quasar"
            value {
                String: "1"
            }
        }
        Storage {
            key: "new_music_radio_nlg"
            value {
                String: "1"
            }
        }
        Storage {
            key: "new_nlg"
            value {
                String: "1"
            }
        }
        Storage {
            key: "new_special_playlists"
            value {
                String: "1"
            }
        }
        Storage {
            key: "personal_tv_channel"
            value {
                String: "1"
            }
        }
        Storage {
            key: "personal_tv_help"
            value {
                String: "1"
            }
        }
        Storage {
            key: "personalization"
            value {
                String: "1"
            }
        }
        Storage {
            key: "play_radio_if_no_plus"
            value {
                String: "1"
            }
        }
        Storage {
            key: "podcasts"
            value {
                String: "1"
            }
        }
        Storage {
            key: "pure_general_conversation"
            value {
                String: "1"
            }
        }
        Storage {
            key: "quasar"
            value {
                String: "1"
            }
        }
        Storage {
            key: "quasar_biometry_limit_users"
            value {
                String: "1"
            }
        }
        Storage {
            key: "quasar_gc_instead_of_search"
            value {
                String: "1"
            }
        }
        Storage {
            key: "quasar_tv"
            value {
                String: "1"
            }
        }
        Storage {
            key: "radio_fixes"
            value {
                String: "1"
            }
        }
        Storage {
            key: "radio_play_in_quasar"
            value {
                String: "1"
            }
        }
        Storage {
            key: "radio_play_onboarding"
            value {
                String: "1"
            }
        }
        Storage {
            key: "read_factoid_source"
            value {
                String: "1"
            }
        }
        Storage {
            key: "recurring_purchase"
            value {
                String: "1"
            }
        }
        Storage {
            key: "shopping_list"
            value {
                String: "1"
            }
        }
        Storage {
            key: "sleep_timers"
            value {
                String: "1"
            }
        }
        Storage {
            key: "supress_multi_activation"
            value {
                String: "1"
            }
        }
        Storage {
            key: "taxi"
            value {
                String: "1"
            }
        }
        Storage {
            key: "taxi_nlu"
            value {
                String: "1"
            }
        }
        Storage {
            key: "translate"
            value {
                String: "1"
            }
        }
        Storage {
            key: "tts_domain_music"
            value {
                String: "1"
            }
        }
        Storage {
            key: "tv"
            value {
                String: "1"
            }
        }
        Storage {
            key: "tv_stream"
            value {
                String: "1"
            }
        }
        Storage {
            key: "tv_vod_translation"
            value {
                String: "1"
            }
        }
        Storage {
            key: "tv_without_channel_status_check"
            value {
                String: "1"
            }
        }
        Storage {
            key: "ugc_enabled"
            value {
                String: "1"
            }
        }
        Storage {
            key: "uniproxy_vins_sessions"
            value {
                String: "1"
            }
        }
        Storage {
            key: "use_contacts"
            value {
                String: "1"
            }
        }
        Storage {
            key: "use_memento"
            value {
                String: "1"
            }
        }
        Storage {
            key: "use_trash_talk_classifier"
            value {
                String: "1"
            }
        }
        Storage {
            key: "username_auto_insert"
            value {
                String: "1"
            }
        }
        Storage {
            key: "video_disable_films_webview_searchscreen"
            value {
                String: "1"
            }
        }
        Storage {
            key: "video_disable_webview_searchscreen"
            value {
                String: "1"
            }
        }
        Storage {
            key: "video_not_use_native_youtube_api"
            value {
                String: "1"
            }
        }
        Storage {
            key: "video_omit_youtube_restriction"
            value {
                String: "1"
            }
        }
        Storage {
            key: "video_qproxy_players"
            value {
                String: "1"
            }
        }
        Storage {
            key: "vins_e2e_partials"
            value {
                String: "1"
            }
        }
        Storage {
            key: "vins_music_play_force_irrelevant"
            value {
                String: "1"
            }
        }
        Storage {
            key: "weather_precipitation"
            value {
                String: "1"
            }
        }
        Storage {
            key: "weather_precipitation_starts_ends"
            value {
                String: "1"
            }
        }
        Storage {
            key: "weather_precipitation_type"
            value {
                String: "1"
            }
        }
    }
    DeviceState {
        DeviceId: "FF98F029669E41603B6B4993"
        SoundLevel: 4
        SoundMuted: false
        IsTvPluggedIn: false
        Music {
            CurrentlyPlaying {
                TrackId: "79476206"
                RawTrackInfo {
                    fields {
                        key: "albums"
                        value {
                            list_value {
                                values {
                                    struct_value {
                                        fields {
                                            key: "artists"
                                            value {
                                                list_value {
                                                    values {
                                                        struct_value {
                                                            fields {
                                                                key: "composer"
                                                                value {
                                                                    bool_value: false
                                                                }
                                                            }
                                                            fields {
                                                                key: "cover"
                                                                value {
                                                                    struct_value {
                                                                        fields {
                                                                            key: "prefix"
                                                                            value {
                                                                                string_value: "dd140ebd.a.14443769-1"
                                                                            }
                                                                        }
                                                                        fields {
                                                                            key: "type"
                                                                            value {
                                                                                string_value: "from-album-cover"
                                                                            }
                                                                        }
                                                                        fields {
                                                                            key: "uri"
                                                                            value {
                                                                                string_value: "avatars.yandex.net/get-music-content/4399644/dd140ebd.a.14443769-1/%%"
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                            fields {
                                                                key: "genres"
                                                                value {
                                                                    list_value {
                                                                    }
                                                                }
                                                            }
                                                            fields {
                                                                key: "id"
                                                                value {
                                                                    number_value: 11028533
                                                                }
                                                            }
                                                            fields {
                                                                key: "name"
                                                                value {
                                                                    string_value: "\320\224\320\265\321\202\321\201\320\272\320\260\321\217 \320\260\321\203\320\264\320\270\320\276\320\272\320\275\320\270\320\263\320\260"
                                                                }
                                                            }
                                                            fields {
                                                                key: "various"
                                                                value {
                                                                    bool_value: false
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "available"
                                            value {
                                                bool_value: true
                                            }
                                        }
                                        fields {
                                            key: "availableForMobile"
                                            value {
                                                bool_value: true
                                            }
                                        }
                                        fields {
                                            key: "availableForPremiumUsers"
                                            value {
                                                bool_value: true
                                            }
                                        }
                                        fields {
                                            key: "availablePartially"
                                            value {
                                                bool_value: false
                                            }
                                        }
                                        fields {
                                            key: "bests"
                                            value {
                                                list_value {
                                                    values {
                                                        number_value: 79476216
                                                    }
                                                    values {
                                                        number_value: 79476203
                                                    }
                                                    values {
                                                        number_value: 79476215
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "buy"
                                            value {
                                                list_value {
                                                }
                                            }
                                        }
                                        fields {
                                            key: "childContent"
                                            value {
                                                bool_value: true
                                            }
                                        }
                                        fields {
                                            key: "coverUri"
                                            value {
                                                string_value: "avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%"
                                            }
                                        }
                                        fields {
                                            key: "genre"
                                            value {
                                                string_value: "fairytales"
                                            }
                                        }
                                        fields {
                                            key: "id"
                                            value {
                                                number_value: 14445807
                                            }
                                        }
                                        fields {
                                            key: "labels"
                                            value {
                                                list_value {
                                                    values {
                                                        struct_value {
                                                            fields {
                                                                key: "id"
                                                                value {
                                                                    number_value: 2942036
                                                                }
                                                            }
                                                            fields {
                                                                key: "name"
                                                                value {
                                                                    string_value: "\320\224\320\265\321\202\321\201\320\272\320\260\321\217 \320\260\321\203\320\264\320\270\320\276\320\272\320\275\320\270\320\263\320\260"
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "likesCount"
                                            value {
                                                number_value: 273
                                            }
                                        }
                                        fields {
                                            key: "metaType"
                                            value {
                                                string_value: "children"
                                            }
                                        }
                                        fields {
                                            key: "ogImage"
                                            value {
                                                string_value: "avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%"
                                            }
                                        }
                                        fields {
                                            key: "recent"
                                            value {
                                                bool_value: false
                                            }
                                        }
                                        fields {
                                            key: "releaseDate"
                                            value {
                                                string_value: "2021-03-12T00:00:00+03:00"
                                            }
                                        }
                                        fields {
                                            key: "title"
                                            value {
                                                string_value: "\320\241\320\272\320\260\320\267\320\272\320\270 \320\275\320\260\321\200\320\276\320\264\320\276\320\262 \320\274\320\270\321\200\320\260"
                                            }
                                        }
                                        fields {
                                            key: "trackCount"
                                            value {
                                                number_value: 21
                                            }
                                        }
                                        fields {
                                            key: "trackPosition"
                                            value {
                                                struct_value {
                                                    fields {
                                                        key: "index"
                                                        value {
                                                            number_value: 5
                                                        }
                                                    }
                                                    fields {
                                                        key: "volume"
                                                        value {
                                                            number_value: 1
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "type"
                                            value {
                                                string_value: "fairy-tale"
                                            }
                                        }
                                        fields {
                                            key: "version"
                                            value {
                                                string_value: "\321\200\320\260\320\264\320\270\320\276\321\201\320\277\320\265\320\272\321\202\320\260\320\272\320\273\320\270"
                                            }
                                        }
                                        fields {
                                            key: "veryImportant"
                                            value {
                                                bool_value: false
                                            }
                                        }
                                        fields {
                                            key: "year"
                                            value {
                                                number_value: 2021
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "artists"
                        value {
                            list_value {
                                values {
                                    struct_value {
                                        fields {
                                            key: "composer"
                                            value {
                                                bool_value: false
                                            }
                                        }
                                        fields {
                                            key: "cover"
                                            value {
                                                struct_value {
                                                    fields {
                                                        key: "prefix"
                                                        value {
                                                            string_value: "dd140ebd.a.14443769-1"
                                                        }
                                                    }
                                                    fields {
                                                        key: "type"
                                                        value {
                                                            string_value: "from-album-cover"
                                                        }
                                                    }
                                                    fields {
                                                        key: "uri"
                                                        value {
                                                            string_value: "avatars.yandex.net/get-music-content/4399644/dd140ebd.a.14443769-1/%%"
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        fields {
                                            key: "genres"
                                            value {
                                                list_value {
                                                }
                                            }
                                        }
                                        fields {
                                            key: "id"
                                            value {
                                                number_value: 11028533
                                            }
                                        }
                                        fields {
                                            key: "name"
                                            value {
                                                string_value: "\320\224\320\265\321\202\321\201\320\272\320\260\321\217 \320\260\321\203\320\264\320\270\320\276\320\272\320\275\320\270\320\263\320\260"
                                            }
                                        }
                                        fields {
                                            key: "various"
                                            value {
                                                bool_value: false
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "available"
                        value {
                            bool_value: true
                        }
                    }
                    fields {
                        key: "availableForPremiumUsers"
                        value {
                            bool_value: true
                        }
                    }
                    fields {
                        key: "availableFullWithoutPermission"
                        value {
                            bool_value: false
                        }
                    }
                    fields {
                        key: "batchId"
                        value {
                            string_value: "sync-1383eba5-8e0d-479c-a8da-da2da125d8fe"
                        }
                    }
                    fields {
                        key: "batchInfo"
                        value {
                            struct_value {
                                fields {
                                    key: "albumId"
                                    value {
                                        string_value: "14445807"
                                    }
                                }
                                fields {
                                    key: "durationMs"
                                    value {
                                        number_value: 204070
                                    }
                                }
                                fields {
                                    key: "general"
                                    value {
                                        bool_value: false
                                    }
                                }
                                fields {
                                    key: "itemType"
                                    value {
                                        string_value: "track"
                                    }
                                }
                                fields {
                                    key: "rid"
                                    value {
                                        string_value: "804431da-f24e-4e77-8e64-1197fcc13319"
                                    }
                                }
                                fields {
                                    key: "syncRid"
                                    value {
                                        string_value: "sync-1383eba5-8e0d-479c-a8da-da2da125d8fe"
                                    }
                                }
                                fields {
                                    key: "type"
                                    value {
                                        string_value: "dynamic"
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "coverUri"
                        value {
                            string_value: "avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%"
                        }
                    }
                    fields {
                        key: "durationMs"
                        value {
                            number_value: 204070
                        }
                    }
                    fields {
                        key: "fileSize"
                        value {
                            number_value: 0
                        }
                    }
                    fields {
                        key: "id"
                        value {
                            string_value: "79476206"
                        }
                    }
                    fields {
                        key: "isSuitableForChildren"
                        value {
                            bool_value: true
                        }
                    }
                    fields {
                        key: "lyricsAvailable"
                        value {
                            bool_value: false
                        }
                    }
                    fields {
                        key: "major"
                        value {
                            struct_value {
                                fields {
                                    key: "id"
                                    value {
                                        number_value: 308
                                    }
                                }
                                fields {
                                    key: "name"
                                    value {
                                        string_value: "ONERPM"
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "normalization"
                        value {
                            struct_value {
                                fields {
                                    key: "gain"
                                    value {
                                        number_value: -3.4
                                    }
                                }
                                fields {
                                    key: "peak"
                                    value {
                                        number_value: 32766
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "ogImage"
                        value {
                            string_value: "avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%"
                        }
                    }
                    fields {
                        key: "previewDurationMs"
                        value {
                            number_value: 30000
                        }
                    }
                    fields {
                        key: "r128"
                        value {
                            struct_value {
                                fields {
                                    key: "i"
                                    value {
                                        number_value: -17.21
                                    }
                                }
                                fields {
                                    key: "tp"
                                    value {
                                        number_value: 0.02
                                    }
                                }
                            }
                        }
                    }
                    fields {
                        key: "realId"
                        value {
                            string_value: "79476206"
                        }
                    }
                    fields {
                        key: "rememberPosition"
                        value {
                            bool_value: true
                        }
                    }
                    fields {
                        key: "storageDir"
                        value {
                            string_value: ""
                        }
                    }
                    fields {
                        key: "title"
                        value {
                            string_value: "\320\220\320\267\320\265\321\200\320\261\320\260\320\271\320\264\320\266\320\260\320\275\321\201\320\272\320\260\321\217 \321\201\320\272\320\260\320\267\320\272\320\260 \342\200\224 \320\233\320\270\321\201 \320\270 \320\272\321\203\321\200\320\276\320\277\320\260\321\202\320\272\320\260"
                        }
                    }
                    fields {
                        key: "trackSharingFlag"
                        value {
                            string_value: "COVER_ONLY"
                        }
                    }
                    fields {
                        key: "type"
                        value {
                            string_value: "fairy-tale"
                        }
                    }
                }
                LastPlayTimestamp: 1630176825288
            }
            PlaylistOwner: ""
            Player {
                Pause: true
                Timestamp: 1630178528
            }
            SessionId: "em7ezuNz"
            LastPlayTimestamp: 1630176825288
        }
        AlarmsState: "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20210814T060000Z\r\nDTEND:20210814T060000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20210814T060000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n"
        AlarmState {
            ICalendar: "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20210814T060000Z\r\nDTEND:20210814T060000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20210814T060000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n"
            CurrentlyPlaying: false
            SoundAlarmSetting {
                Type: "music"
                RawInfo {
                    fields {
                        key: "filters"
                        value {
                            struct_value {
                                fields {
                                    key: "personality"
                                    value {
                                        string_value: "is_user_stream"
                                    }
                                }
                            }
                        }
                    }
                }
            }
            MaxSoundLevel: 7
        }
        DeviceConfig {
            ContentSettings: children
            Spotter: "alisa"
            ChildContentSettings: safe
        }
        Timers {
        }
        LastWatched {
        }
        AudioPlayer {
            PlayerState: Finished
            OffsetMs: 151000
            CurrentlyPlaying {
                StreamId: "6098528"
                LastPlayTimestamp: 1630099164919
                Title: "\320\241\320\277\321\217\321\202 \321\203\321\201\321\202\320\260\320\273\321\213\320\265 \320\270\320\263\321\200\321\203\321\210\320\272\320\270"
                SubTitle: "\320\236\320\273\320\265\320\263 \320\220\320\275\320\276\321\204\321\200\320\270\320\265\320\262"
                StreamType: "Track"
            }
            ScenarioMeta {
                key: "@scenario_name"
                value: "HollywoodMusic"
            }
            ScenarioMeta {
                key: "owner"
                value: "music"
            }
            ScenarioMeta {
                key: "what_is_playing_answer"
                value: "\320\236\320\273\320\265\320\263 \320\220\320\275\320\276\321\204\321\200\320\270\320\265\320\262, \320\277\320\265\321\201\320\275\321\217 \"\320\241\320\277\321\217\321\202 \321\203\321\201\321\202\320\260\320\273\321\213\320\265 \320\270\320\263\321\200\321\203\321\210\320\272\320\270\""
            }
            LastPlayTimestamp: 1630099164919
            DurationMs: 151000
            LastStopTimestamp: 1630099317052
            PlayedMs: 151000
        }
        Bluetooth {
        }
        InternetConnection {
            Type: Wifi_2_4GHz
            Current {
                Ssid: "InfoLink-Wi-Fi."
                Bssid: "f8:f0:82:5b:dc:6e"
                Channel: 1
            }
            Neighbours {
                Ssid: "InfoLink-Wi-Fi."
                Bssid: "f8:f0:82:5b:dc:6e"
                Channel: 1
            }
            Neighbours {
                Ssid: "MegaFon_2.4G_2F109F"
                Bssid: "08:c6:b3:2f:10:a0"
                Channel: 7
            }
        }
        MicsMuted: false
        SoundMaxLevel: 10
    }
    AdditionalOptions {
        BassOptions {
            UserAgent: "Mozilla\\/5.0 (Linux; Android 6.0.1; Station Build\\/MOB30J; wv) AppleWebKit\\/537.36 (KHTML, like Gecko) Version\\/4.0 Chrome\\/61.0.3163.98 Safari\\/537.36 YandexStation\\/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)"
            ClientIP: "79.133.141.197"
        }
        SupportedFeatures: "multiroom"
        SupportedFeatures: "multiroom_cluster"
        SupportedFeatures: "multiroom_audio_client"
        SupportedFeatures: "change_alarm_sound"
        SupportedFeatures: "change_alarm_sound_level"
        SupportedFeatures: "music_player_allow_shots"
        SupportedFeatures: "bluetooth_player"
        SupportedFeatures: "audio_client"
        SupportedFeatures: "audio_client_hls"
        SupportedFeatures: "notifications"
        SupportedFeatures: "tts_play_placeholder"
        SupportedFeatures: "incoming_messenger_calls"
        SupportedFeatures: "publicly_available"
        SupportedFeatures: "directive_sequencer"
        SupportedFeatures: "set_alarm_semantic_frame_v2"
        SupportedFeatures: "muzpult"
        SupportedFeatures: "audio_bitrate192"
        SupportedFeatures: "audio_bitrate320"
        SupportedFeatures: "prefetch_invalidation"
        YandexUID: "1425164113"
        ServerTimeMs: 1630184399523
        AppInfo: "eyJicm93c2VyTmFtZSI6Ik90aGVyQXBwbGljYXRpb25zIiwiZGV2aWNlVHlwZSI6InN0YXRpb25fbWluaSIsImRldmljZU1vZGVsIjoieWFuZGV4bWluaSIsIm1vYmlsZVBsYXRmb3JtIjoiYW5kcm9pZCJ9"
        DoNotUseUserLogs: false
        Puid: "1425164113"
        ICookie: "9014366150703152919"
        Expboxes: "375477,0,55;391026,0,43;341893,0,15;341898,0,22;341906,0,81;341910,0,43;341916,0,6;341923,0,54;341934,0,65;341938,0,10;341942,0,98;341928,0,93;329372,0,25;329377,0,99;329379,0,32;336918,0,80;336932,0,76;336937,0,13;336938,0,57;336943,0,1;336952,0,47;336959,0,98;336964,0,86;336969,0,16;336973,0,32;336977,0,82;336983,0,51;336986,0,75;336990,0,83;336996,0,42;336998,0,8;337003,0,86;404842,0,99;315360,0,51;409851,0,4;315365,0,35;405527,0,42;315614,0,21;330999,0,52"
    }
    VoiceSession: true
    TestIDs: 348361
    TestIDs: 383587
    TestIDs: 378426
    TestIDs: 400238
    TestIDs: 375477
    TestIDs: 391026
    TestIDs: 341893
    TestIDs: 341898
    TestIDs: 341906
    TestIDs: 341910
    TestIDs: 341916
    TestIDs: 341923
    TestIDs: 341934
    TestIDs: 341938
    TestIDs: 341942
    TestIDs: 341928
    TestIDs: 329372
    TestIDs: 329377
    TestIDs: 329379
    TestIDs: 336918
    TestIDs: 336932
    TestIDs: 336937
    TestIDs: 336938
    TestIDs: 336943
    TestIDs: 336952
    TestIDs: 336959
    TestIDs: 336964
    TestIDs: 336969
    TestIDs: 336973
    TestIDs: 336977
    TestIDs: 336983
    TestIDs: 336986
    TestIDs: 336990
    TestIDs: 336996
    TestIDs: 336998
    TestIDs: 337003
    TestIDs: 404842
    TestIDs: 315360
    TestIDs: 409851
    TestIDs: 315365
    TestIDs: 405527
    TestIDs: 315614
    LaasRegion {
        fields {
            key: "city_id"
            value {
                number_value: 45
            }
        }
        fields {
            key: "country_id_by_ip"
            value {
                number_value: 225
            }
        }
        fields {
            key: "is_anonymous_vpn"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_gdpr"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_hosting"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_mobile"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_public_proxy"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_serp_trusted_net"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_tor"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_user_choice"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_yandex_net"
            value {
                bool_value: false
            }
        }
        fields {
            key: "is_yandex_staff"
            value {
                bool_value: false
            }
        }
        fields {
            key: "latitude"
            value {
                number_value: 56.125889
            }
        }
        fields {
            key: "location_accuracy"
            value {
                number_value: 140
            }
        }
        fields {
            key: "location_unixtime"
            value {
                number_value: 1630184120
            }
        }
        fields {
            key: "longitude"
            value {
                number_value: 47.390862
            }
        }
        fields {
            key: "precision"
            value {
                number_value: 2
            }
        }
        fields {
            key: "probable_regions"
            value {
                list_value {
                    values {
                        struct_value {
                            fields {
                                key: "region_id"
                                value {
                                    number_value: 45
                                }
                            }
                            fields {
                                key: "weight"
                                value {
                                    number_value: 1
                                }
                            }
                        }
                    }
                }
            }
        }
        fields {
            key: "probable_regions_reliability"
            value {
                number_value: 1
            }
        }
        fields {
            key: "region_by_ip"
            value {
                number_value: 45
            }
        }
        fields {
            key: "region_home"
            value {
                number_value: 45
            }
        }
        fields {
            key: "region_id"
            value {
                number_value: 217096
            }
        }
        fields {
            key: "regular_coordinates"
            value {
                list_value {
                    values {
                        struct_value {
                            fields {
                                key: "lat"
                                value {
                                    number_value: 56.125641
                                }
                            }
                            fields {
                                key: "lon"
                                value {
                                    number_value: 47.389925
                                }
                            }
                            fields {
                                key: "type"
                                value {
                                    number_value: 1
                                }
                            }
                        }
                    }
                    values {
                        struct_value {
                            fields {
                                key: "lat"
                                value {
                                    number_value: 56.11174
                                }
                            }
                            fields {
                                key: "lon"
                                value {
                                    number_value: 47.488506
                                }
                            }
                            fields {
                                key: "type"
                                value {
                                    number_value: 0
                                }
                            }
                        }
                    }
                    values {
                        struct_value {
                            fields {
                                key: "lat"
                                value {
                                    number_value: 56.118437
                                }
                            }
                            fields {
                                key: "lon"
                                value {
                                    number_value: 47.452439
                                }
                            }
                            fields {
                                key: "type"
                                value {
                                    number_value: 0
                                }
                            }
                        }
                    }
                }
            }
        }
        fields {
            key: "should_update_cookie"
            value {
                bool_value: false
            }
        }
        fields {
            key: "suspected_latitude"
            value {
                number_value: 56.125889
            }
        }
        fields {
            key: "suspected_location_accuracy"
            value {
                number_value: 140
            }
        }
        fields {
            key: "suspected_location_unixtime"
            value {
                number_value: 1630184120
            }
        }
        fields {
            key: "suspected_longitude"
            value {
                number_value: 47.390862
            }
        }
        fields {
            key: "suspected_precision"
            value {
                number_value: 2
            }
        }
        fields {
            key: "suspected_region_city"
            value {
                number_value: 45
            }
        }
        fields {
            key: "suspected_region_id"
            value {
                number_value: 217096
            }
        }
    }
    RawPersonalData: "{\"/v1/personality/profile/alisa/kv/alice_children_biometry\":\"enabled\",\"/v1/personality/profile/alisa/kv/alice_proactivity\":\"enabled\"}"
    SmartHomeInfo {
        Payload {
            Devices {
                Id: "40e00ad5-98ec-4911-8226-c7e4b8651a1f"
                Name: "\320\257\320\275\320\264\320\265\320\272\321\201 \320\234\320\270\320\275\320\270"
                Type: "devices.types.smart_speaker.yandex.station.mini"
                QuasarInfo {
                    DeviceId: "FF98F029669E41603B6B4993"
                    Platform: "yandexmini"
                }
                Created: 1628432547
            }
        }
    }
    NotificationState {
    }
    ActivationType: "spotter"
}
MementoData: "CuQMQuEMCt4MCObnqYkGOApCGAoKYWxpY2Vfc2hvdxIKCIj/nIkGEAkYMEIXCgl0aW1lcl9zZXQSCgjs5I+JBhAIGC1CEwoFbXVzaWMSCgjm56mJBhAKGDNCHQoPbXVzaWNfcGxheWxpc3RzEgoItKvoiAYQBRggQhMKBXJhZGlvEgoI3LL/iAYQBxgpQhUKB2FtYmllbnQSCgiezfeIBhAGGCZCGwoNbXVzaWNfZmlsdGVycxIKCObnqYkGEAoYM0IZCgtjaGlsZF9tdXNpYxIKCMrl1YgGEAQYG1rhAwoKYW1iaWVudF9fMRL9AgowcGVyc29uYWxfYXNzaXN0YW50LnNjZW5hcmlvcy5tdXNpY19hbWJpZW50X3NvdW5kGjQKMgowcGVyc29uYWxfYXNzaXN0YW50LnNjZW5hcmlvcy5tdXNpY19hbWJpZW50X3NvdW5kGmMKYQoncGVyc29uYWxfYXNzaXN0YW50LnNjZW5hcmlvcy5tdXNpY19wbGF5EjYKEHNwZWNpYWxfcGxheWxpc3QaImFtYmllbnRfc291bmRzX2RlZmF1bHR8cmFpbl9zb3VuZHMaaApmCidwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLm11c2ljX3BsYXkSOwoLc2VhcmNoX3RleHQaLCg/OtGI0YPQvHzQt9Cy0YPQuikuKig/OtC60LDQvNC40L180LTQvtC20LQpIiTQstC60LvRjtGH0Lgg0LfQstGD0LrQuCDQutCw0LzQuNC90LAiHtCy0LrQu9GO0YfQuCDRiNGD0Lwg0LTQvtC20LTRjxoHYW1iaWVudCIkcGVyc29uYWxfYXNzaXN0YW50LmhhbmRjcmFmdGVkLmhlbGxvMiRiYjlmZmYwNy1hYzFiLTQ2OTYtOTEyYy0zNGFkYjVhZjQ4ZjlatQEKCHJhZGlvX18xElYKJ3BlcnNvbmFsX2Fzc2lzdGFudC5zY2VuYXJpb3MucmFkaW9fcGxheRorCikKJ3BlcnNvbmFsX2Fzc2lzdGFudC5zY2VuYXJpb3MucmFkaW9fcGxheRoFcmFkaW8iJHBlcnNvbmFsX2Fzc2lzdGFudC5oYW5kY3JhZnRlZC5oZWxsbzIkNTZiZmU5MzgtZTYxMC00NzZhLWI4YjMtMGE2ZDc4YTBmMTQ3WrsBCgx0aW1lcl9zZXRfXzESVAomcGVyc29uYWxfYXNzaXN0YW50LnNjZW5hcmlvcy50aW1lcl9zZXQaKgooCiZwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnRpbWVyX3NldBoJdGltZXJfc2V0IiRwZXJzb25hbF9hc3Npc3RhbnQuaGFuZGNyYWZ0ZWQuaGVsbG8yJDNmZmE2MmNiLTk5NDItNDViNC05NDA4LWNjNjEwMmI5NTNkNlqGAgodbW9ybmluZ19zaG93X19oZWxsb19tb3JuaW5nXzESjAEKIWFsaWNlLnZpbnNsZXNzLm11c2ljLm1vcm5pbmdfc2hvdxpDCkEKKXBlcnNvbmFsX2Fzc2lzdGFudC5zY2VuYXJpb3MubW9ybmluZ19zaG93EhQKCXNob3dfdHlwZRoHbW9ybmluZyIi0LLQutC70Y7Rh9C4INGD0YLRgNC10L3QtdC1INGI0L7RgxoKYWxpY2Vfc2hvdyIkcGVyc29uYWxfYXNzaXN0YW50LmhhbmRjcmFmdGVkLmhlbGxvMiQwNWIzNTJiOC0zYTI4LTQ2MTgtYWZmMC03NjdhZGNmZmQ2OGFa9QEKDm11c2ljX21vb2RfMV8yEoABCidwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLm11c2ljX3BsYXkaVQpRCidwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLm11c2ljX3BsYXkSGQoEbW9vZBIEbW9vZBoFaGFwcHkiBG1vb2QqC0IJQgcSBWhhcHB5KAEaBW11c2ljGg1tdXNpY19maWx0ZXJzIiRwZXJzb25hbF9hc3Npc3RhbnQuaGFuZGNyYWZ0ZWQuaGVsbG8yJDE2MWVkMTBjLTRhZGQtNDdkYS1hYWYwLTA1ZDkxNWQyNjcxN2IkMTYxZWQxMGMtNGFkZC00N2RhLWFhZjAtMDVkOTE1ZDI2NzE3aDNwM3jm56mJBhIcChhGRjk4RjAyOTY2OUU0MTYwM0I2QjQ5OTMSACpkCig5NGUyNmYyYzFlODk4MmYxODFmZjhmZGQ2NTQyODNlMmYyOGE3Mjc2EP///////////wEYhdD9AyIoOTRlMjZmMmMxZTg5ODJmMTgxZmY4ZmRkNjU0MjgzZTJmMjhhNzI3Ng=="
)*";

const TString PROTO_SPEECHKIT_RESPONSE = R"(
Header {
    RequestId: "8b464f68-df7b-485c-a81b-0640051c9f49"
    ResponseId: "bb97fc42-759322b0-e061eff8-978cfb2d"
    SequenceNumber: 84
    RefMessageId: "8b464f68-df7b-485c-a81b-0640051c9f49"
    SessionId: "db6ba473-c073-4ec2-94a7-14d3f650bdee"
}
VoiceResponse {
    OutputSpeech {
        Type: "simple"
        Text: "\320\237\320\276\320\264\320\276\320\267\321\200\320\265\320\262\320\260\321\216, \321\207\321\202\320\276 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 <speaker voice=\"shitova\"> - \321\215\321\202\320\276 \320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
    }
    ShouldListen: false
}
Response {
    Cards {
        Type: "simple_text"
        Text: "\320\237\320\276\320\264\320\276\320\267\321\200\320\265\320\262\320\260\321\216, \321\207\321\202\320\276 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 - \321\215\321\202\320\276 \320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
    }
    QualityStorage {
        PreclassifierPredicts {
            key: "GeneralConversation"
            value: 0
        }
        PreclassifierPredicts {
            key: "HardcodedResponse"
            value: 0
        }
        PreclassifierPredicts {
            key: "HollywoodMusic"
            value: -5.47678423
        }
        PreclassifierPredicts {
            key: "IoT"
            value: 0
        }
        PreclassifierPredicts {
            key: "Search"
            value: 1.15632915
        }
        PreclassifierPredicts {
            key: "SideSpeech"
            value: 0
        }
        PreclassifierPredicts {
            key: "SkillDiscoveryGc"
            value: 0
        }
        PreclassifierPredicts {
            key: "Translation"
            value: 0
        }
        PreclassifierPredicts {
            key: "Video"
            value: -5.4341712
        }
        PreclassifierPredicts {
            key: "Vins"
            value: -0.787874043
        }
        PostclassifierPredicts {
            key: "GeneralConversation"
            value: 0
        }
        PostclassifierPredicts {
            key: "HollywoodMusic"
            value: -3.54384041
        }
        PostclassifierPredicts {
            key: "Search"
            value: 0.460568786
        }
        PostclassifierPredicts {
            key: "SideSpeech"
            value: -2.4131763
        }
        PostclassifierPredicts {
            key: "Translation"
            value: 0
        }
        PostclassifierPredicts {
            key: "Video"
            value: -4.38396311
        }
        PostclassifierPredicts {
            key: "Vins"
            value: 0.0404732749
        }
        PostclassificationWinReason: WR_PRIORITY
        ScenariosInformation {
            key: "AddPointTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Alarm"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Alice4Business"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "AliceShow"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "AutomotiveRadio"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Bluetooth"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Bugreport"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "CecCommands"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Centaur"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Commands"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Covid19"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Dialogovo"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "DialogovoB2b"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "DiskMyPhotos"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "DoNothing"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "DrawPicture"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "DriveOrder"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ExternalSkillFlashBriefing"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ExternalSkillRecipes"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "FindPoiTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Food"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "GameSuggest"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "GeneralConversation"
            value {
                Reason: LR_PRIORITY
            }
        }
        ScenariosInformation {
            key: "GeneralConversationHeavy"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "GeneralConversationTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "GetMyLocationTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "GetWeatherTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "HandcraftedTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "HappyNewYear"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "HardcodedResponse"
            value {
                Reason: LR_IRRELEVANT
            }
        }
        ScenariosInformation {
            key: "HollywoodAlarm"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "HollywoodHardcodedMusic"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "HollywoodMusic"
            value {
                Reason: LR_PRIORITY
            }
        }
        ScenariosInformation {
            key: "HollywoodZeroTesting"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ImageWhatIsThis"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "IoTGranet"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "IoTScenarios"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "IoTVoiceDiscovery"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "LinkARemote"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "MapsDownloadOffline"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "MarketHowMuch"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "MarketOrdersStatus"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "MessengerCall"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Miles"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "MordoviaVideoSelection"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "MovieSuggest"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "NaviExternalConfirmationTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "News"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "NotificationsManager"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "OnboardingCriticalUpdate"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "OpenAppsFixlist"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "PhoneNotification"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "PhotoFrame"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "RandomNumber"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Reask"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Reminders"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Repeat"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Route"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Search"
            value {
                Reason: LR_PRIORITY
            }
        }
        ScenariosInformation {
            key: "ShowGif"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ShowRouteTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ShowTraffic"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ShowTrafficBass"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ShowTvChannelsGallery"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "SideSpeech"
            value {
                Reason: LR_PRIORITY
            }
        }
        ScenariosInformation {
            key: "SimSim"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "SkillsDiscovery"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "SubscriptionsManager"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "SwitchLayerTr"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Taximeter"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Theremin"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Transcription"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "TransformFace"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "TvChannels"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Video"
            value {
                Reason: LR_PRIORITY
            }
        }
        ScenariosInformation {
            key: "VideoCommand"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "VideoMusicalClips"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "VideoPlayConcert"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "VideoRater"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "VideoTrailer"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Vins"
            value {
                Reason: LR_PRIORITY
            }
        }
        ScenariosInformation {
            key: "Weather"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "Wizard"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ZapravkiB2B"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ZenSearch"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
        ScenariosInformation {
            key: "ZeroTesting"
            value {
                Reason: LR_NOT_ALLOWED
            }
        }
    }
    Experiments {
    }
    Suggest {
        Items {
            Type: "action"
            Title: "\320\237\320\276\320\262\321\202\320\276\321\200\320\270"
            Directives {
                Type: "client_action"
                Name: "type"
                AnalyticsType: "render_buttons_type"
                Payload {
                    fields {
                        key: "text"
                        value {
                            string_value: "\320\237\320\276\320\262\321\202\320\276\321\200\320\270"
                        }
                    }
                }
            }
            Directives {
                Type: "server_action"
                Name: "on_suggest"
                IgnoreAnswer: true
                Payload {
                    fields {
                        key: "@request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "@scenario_name"
                        value {
                            string_value: "Vins"
                        }
                    }
                    fields {
                        key: "button_id"
                        value {
                            string_value: "a40b02f-6e1c4901-c986dc3f-3b0215f4"
                        }
                    }
                    fields {
                        key: "caption"
                        value {
                            string_value: "\320\237\320\276\320\262\321\202\320\276\321\200\320\270"
                        }
                    }
                    fields {
                        key: "request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "scenario_name"
                        value {
                            string_value: "Translation"
                        }
                    }
                }
                IsLedSilent: true
            }
        }
        Items {
            Type: "action"
            Title: "\320\220 \320\275\320\260 \321\205\320\276\321\200\320\262\320\260\321\202\321\201\320\272\320\276\320\274?"
            Directives {
                Type: "client_action"
                Name: "type"
                AnalyticsType: "render_buttons_type"
                Payload {
                    fields {
                        key: "text"
                        value {
                            string_value: "\320\220 \320\275\320\260 \321\205\320\276\321\200\320\262\320\260\321\202\321\201\320\272\320\276\320\274?"
                        }
                    }
                }
            }
            Directives {
                Type: "server_action"
                Name: "on_suggest"
                IgnoreAnswer: true
                Payload {
                    fields {
                        key: "@request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "@scenario_name"
                        value {
                            string_value: "Vins"
                        }
                    }
                    fields {
                        key: "button_id"
                        value {
                            string_value: "2e213e69-6c2beab9-d1d55084-dc2e8c6e"
                        }
                    }
                    fields {
                        key: "caption"
                        value {
                            string_value: "\320\220 \320\275\320\260 \321\205\320\276\321\200\320\262\320\260\321\202\321\201\320\272\320\276\320\274?"
                        }
                    }
                    fields {
                        key: "request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "scenario_name"
                        value {
                            string_value: "Translation"
                        }
                    }
                }
                IsLedSilent: true
            }
        }
        Items {
            Type: "action"
            Title: "\320\220 \320\275\320\260 \321\205\320\270\320\275\320\264\320\270?"
            Directives {
                Type: "client_action"
                Name: "type"
                AnalyticsType: "render_buttons_type"
                Payload {
                    fields {
                        key: "text"
                        value {
                            string_value: "\320\220 \320\275\320\260 \321\205\320\270\320\275\320\264\320\270?"
                        }
                    }
                }
            }
            Directives {
                Type: "server_action"
                Name: "on_suggest"
                IgnoreAnswer: true
                Payload {
                    fields {
                        key: "@request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "@scenario_name"
                        value {
                            string_value: "Vins"
                        }
                    }
                    fields {
                        key: "button_id"
                        value {
                            string_value: "70605933-a3ee2c65-7b5bfa10-39a50e4a"
                        }
                    }
                    fields {
                        key: "caption"
                        value {
                            string_value: "\320\220 \320\275\320\260 \321\205\320\270\320\275\320\264\320\270?"
                        }
                    }
                    fields {
                        key: "request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "scenario_name"
                        value {
                            string_value: "Translation"
                        }
                    }
                }
                IsLedSilent: true
            }
        }
        Items {
            Type: "action"
            Title: "\320\220 \320\275\320\260 \320\260\321\200\320\260\320\261\321\201\320\272\320\276\320\274?"
            Directives {
                Type: "client_action"
                Name: "type"
                AnalyticsType: "render_buttons_type"
                Payload {
                    fields {
                        key: "text"
                        value {
                            string_value: "\320\220 \320\275\320\260 \320\260\321\200\320\260\320\261\321\201\320\272\320\276\320\274?"
                        }
                    }
                }
            }
            Directives {
                Type: "server_action"
                Name: "on_suggest"
                IgnoreAnswer: true
                Payload {
                    fields {
                        key: "@request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "@scenario_name"
                        value {
                            string_value: "Vins"
                        }
                    }
                    fields {
                        key: "button_id"
                        value {
                            string_value: "74a63a26-665f6365-4227206d-38470285"
                        }
                    }
                    fields {
                        key: "caption"
                        value {
                            string_value: "\320\220 \320\275\320\260 \320\260\321\200\320\260\320\261\321\201\320\272\320\276\320\274?"
                        }
                    }
                    fields {
                        key: "request_id"
                        value {
                            string_value: "8b464f68-df7b-485c-a81b-0640051c9f49"
                        }
                    }
                    fields {
                        key: "scenario_name"
                        value {
                            string_value: "Translation"
                        }
                    }
                }
                IsLedSilent: true
            }
        }
    }
    Templates {
    }
    DirectivesExecutionPolicy: BeforeSpeech
}
Version: "vins/stable-159-4@8565965"
MegamindAnalyticsInfo {
    AnalyticsInfo {
        key: "Translation"
        value {
            ScenarioAnalyticsInfo {
                Intent: "translate"
                ProductScenarioName: "translate"
                ScenarioTimings {
                    Timings {
                        key: "run"
                        value {
                            StartTimestamp: 1630184399599427
                        }
                    }
                }
            }
            Version: "vins/stable-159-4@8565965"
            SemanticFrame {
                Name: "translate"
                Slots {
                    Name: "input_lang_src"
                    Type: "string"
                    Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                    }
                }
                Slots {
                    Name: "lang_dst"
                    Type: "string"
                    Value: "\321\200\321\203\321\201\321\201\320\272\320\270\320\271"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\321\200\321\203\321\201\321\201\320\272\320\270\320\271"
                    }
                }
                Slots {
                    Name: "lang_src"
                    Type: "string"
                    Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270\320\271"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270\320\271"
                    }
                }
                Slots {
                    Name: "result"
                    Type: "string"
                    Value: "\320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
                    }
                }
                Slots {
                    Name: "speed"
                    Type: "num"
                    AcceptedTypes: "num"
                }
                Slots {
                    Name: "suggest_langs"
                    Type: "string"
                    Value: "\321\205\320\276\321\200\320\262\320\260\321\202\321\201\320\272\320\270\320\271 \321\205\320\270\320\275\320\264\320\270 \320\260\321\200\320\260\320\261\321\201\320\272\320\270\320\271"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\321\205\320\276\321\200\320\262\320\260\321\202\321\201\320\272\320\270\320\271 \321\205\320\270\320\275\320\264\320\270 \320\260\321\200\320\260\320\261\321\201\320\272\320\270\320\271"
                    }
                }
                Slots {
                    Name: "text"
                    Type: "string"
                    Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    }
                }
                Slots {
                    Name: "text_to_translate"
                    Type: "string"
                    Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    }
                }
                Slots {
                    Name: "text_to_translate_voice"
                    Type: "string"
                    Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    }
                }
                Slots {
                    Name: "translate_service"
                    Type: "string"
                    AcceptedTypes: "string"
                }
                Slots {
                    Name: "voice"
                    Type: "string"
                    Value: "\320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
                    AcceptedTypes: "string"
                    TypedValue {
                        Type: "string"
                        String: "\320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
                    }
                }
            }
            FrameActions {
                key: "1"
                value {
                    NluHint {
                        FrameName: "1"
                    }
                    Directives {
                        List {
                            TypeTextDirective {
                                Name: "render_buttons_type"
                                Text: "\320\237\320\276\320\262\321\202\320\276\321\200\320\270"
                            }
                        }
                    }
                }
            }
            FrameActions {
                key: "2"
                value {
                    NluHint {
                        FrameName: "2"
                    }
                    Directives {
                        List {
                            TypeTextDirective {
                                Name: "render_buttons_type"
                                Text: "\320\220 \320\275\320\260 \321\205\320\276\321\200\320\262\320\260\321\202\321\201\320\272\320\276\320\274?"
                            }
                        }
                    }
                }
            }
            FrameActions {
                key: "3"
                value {
                    NluHint {
                        FrameName: "3"
                    }
                    Directives {
                        List {
                            TypeTextDirective {
                                Name: "render_buttons_type"
                                Text: "\320\220 \320\275\320\260 \321\205\320\270\320\275\320\264\320\270?"
                            }
                        }
                    }
                }
            }
            FrameActions {
                key: "4"
                value {
                    NluHint {
                        FrameName: "4"
                    }
                    Directives {
                        List {
                            TypeTextDirective {
                                Name: "render_buttons_type"
                                Text: "\320\220 \320\275\320\260 \320\260\321\200\320\260\320\261\321\201\320\272\320\276\320\274?"
                            }
                        }
                    }
                }
            }
            FrameActions {
                key: "ellipsis"
                value {
                    NluHint {
                        FrameName: "personal_assistant.scenarios.translate.translation__ellipsis"
                    }
                    Callback {
                        Name: "translation_ellipsis"
                        Payload {
                            fields {
                                key: "from_language"
                                value {
                                    string_value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270\320\271"
                                }
                            }
                            fields {
                                key: "phrase"
                                value {
                                    string_value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                                }
                            }
                        }
                    }
                }
            }
            FrameActions {
                key: "quicker"
                value {
                    NluHint {
                        FrameName: "personal_assistant.scenarios.translate.translation__quicker"
                    }
                    Frame {
                        Name: "personal_assistant.scenarios.translate.translation"
                        Slots {
                            Name: "phrase"
                            Type: "string"
                            Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                            AcceptedTypes: "string"
                        }
                        Slots {
                            Name: "from_language"
                            Type: "string"
                            Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270\320\271"
                            AcceptedTypes: "string"
                        }
                        Slots {
                            Name: "to_language"
                            Type: "string"
                            Value: "\321\200\321\203\321\201\321\201\320\272\320\270\320\271"
                            AcceptedTypes: "string"
                        }
                        Slots {
                            Name: "speed"
                            Type: "num"
                            Value: "0.9"
                            AcceptedTypes: "num"
                        }
                        Slots {
                            Name: "settings"
                            Type: "string"
                            Value: "quicker"
                            AcceptedTypes: "string"
                        }
                    }
                }
            }
            FrameActions {
                key: "repeat"
                value {
                    NluHint {
                        FrameName: "repeat"
                    }
                    Callback {
                        Name: "repeat"
                        Payload {
                            fields {
                                key: "voice"
                                value {
                                    string_value: "\320\277\320\276\320\273\320\264\320\265\321\200 \320\263\320\260\320\263\320\260\321\207"
                                }
                            }
                        }
                    }
                }
            }
            FrameActions {
                key: "slower"
                value {
                    NluHint {
                        FrameName: "personal_assistant.scenarios.translate.translation__slower"
                    }
                    Frame {
                        Name: "personal_assistant.scenarios.translate.translation"
                        Slots {
                            Name: "phrase"
                            Type: "string"
                            Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                            AcceptedTypes: "string"
                        }
                        Slots {
                            Name: "from_language"
                            Type: "string"
                            Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270\320\271"
                            AcceptedTypes: "string"
                        }
                        Slots {
                            Name: "to_language"
                            Type: "string"
                            Value: "\321\200\321\203\321\201\321\201\320\272\320\270\320\271"
                            AcceptedTypes: "string"
                        }
                        Slots {
                            Name: "speed"
                            Type: "num"
                            Value: "0.9"
                            AcceptedTypes: "num"
                        }
                        Slots {
                            Name: "settings"
                            Type: "string"
                            Value: "slower"
                            AcceptedTypes: "string"
                        }
                    }
                }
            }
            MatchedSemanticFrames {
                Name: "personal_assistant.scenarios.translate.translation"
                Slots {
                    Name: "phrase"
                    Type: "string"
                    Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                    AcceptedTypes: "string"
                }
                Slots {
                    Name: "from_language"
                    Type: "string"
                    Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                    AcceptedTypes: "string"
                }
            }
        }
    }
    OriginalUtterance: "\321\207\321\202\320\276 \321\202\320\260\320\272\320\276\320\265 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 \320\277\320\276 \321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
    ModifiersInfo {
        Proactivity {
            SemanticFramesInfo {
                Source: Begemot
                SemanticFrames {
                    Name: "personal_assistant.scenarios.translate.translation"
                    Slots {
                        Name: "phrase"
                        Type: "string"
                        Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                        AcceptedTypes: "string"
                    }
                    Slots {
                        Name: "from_language"
                        Type: "string"
                        Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                        AcceptedTypes: "string"
                    }
                }
            }
            Source: "Translation"
        }
    }
    ScenarioTimings {
        key: "GeneralConversation"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399596471
                }
            }
        }
    }
    ScenarioTimings {
        key: "HardcodedResponse"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399597204
                }
            }
        }
    }
    ScenarioTimings {
        key: "HollywoodMusic"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399597982
                }
            }
        }
    }
    ScenarioTimings {
        key: "Search"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399923778
                }
            }
        }
    }
    ScenarioTimings {
        key: "SideSpeech"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399598758
                }
            }
        }
    }
    ScenarioTimings {
        key: "Translation"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399599427
                }
            }
        }
    }
    ScenarioTimings {
        key: "Video"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399600396
                }
            }
        }
    }
    ScenarioTimings {
        key: "Vins"
        value {
            Timings {
                key: "run"
                value {
                    StartTimestamp: 1630184399601309
                }
            }
        }
    }
    WinnerScenario {
        Name: "Translation"
    }
    PostClassifyDuration: 7137
    ShownUtterance: "\320\247\321\202\320\276 \321\202\320\260\320\272\320\276\320\265 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 \320\277\320\276-\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270?"
    IoTUserInfo {
        Devices {
            Id: "40e00ad5-98ec-4911-8226-c7e4b8651a1f"
            Name: "\320\257\320\275\320\264\320\265\320\272\321\201 \320\234\320\270\320\275\320\270"
            Type: YandexStationMiniDeviceType
            ExternalId: "FF98F029669E41603B6B4993.yandexmini"
            ExternalName: "\320\257\320\275\320\264\320\265\320\272\321\201 \320\234\320\270\320\275\320\270"
            SkillId: "Q"
            DeviceInfo {
                Manufacturer: "Yandex Services AG"
                Model: "YNDX-0004"
            }
            QuasarInfo {
                DeviceId: "FF98F029669E41603B6B4993"
                Platform: "yandexmini"
            }
            CustomData: "{\"device_id\":\"FF98F029669E41603B6B4993\",\"platform\":\"yandexmini\"}"
            Updated: 1628455445
            Created: 1628432547
            HouseholdId: "4696a330-2212-4829-8975-a4bf7be03cd3"
            OriginalType: YandexStationMiniDeviceType
            Status: OnlineDeviceState
            StatusUpdated: 1629453046
            AnalyticsType: "devices.types.smart_speaker.yandex.station.mini"
            AnalyticsName: "\320\243\320\274\320\275\320\276\320\265 \321\203\321\201\321\202\321\200\320\276\320\271\321\201\321\202\320\262\320\276"
            IconURL: "http://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.mini.png"
        }
        Households {
            Id: "4696a330-2212-4829-8975-a4bf7be03cd3"
            Name: "\320\224\320\236\320\234"
            Longitude: 47.390522
            Latitude: 56.126339
            Address: "\320\240\320\276\321\201\321\201\320\270\321\217, \320\247\321\203\320\262\320\260\321\210\321\201\320\272\320\260\321\217 \320\240\320\265\321\201\320\277\321\203\320\261\320\273\320\270\320\272\320\260, \320\247\320\265\320\261\320\276\320\272\321\201\320\260\321\200\321\213, \320\274\320\270\320\272\321\200\320\276\321\200\320\260\320\271\320\276\320\275 \320\235\320\276\320\262\321\213\320\271 \320\223\320\276\321\200\320\276\320\264, \320\235\320\276\320\262\320\276\320\263\320\276\321\200\320\276\320\264\321\201\320\272\320\260\321\217 \321\203\320\273\320\270\321\206\320\260, 38"
        }
        Households {
            Id: "882102ad-e30f-40c1-9014-fc867f01f3f7"
            Name: "\320\234\320\276\320\271 \320\264\320\276\320\274"
        }
        CurrentHouseholdId: "4696a330-2212-4829-8975-a4bf7be03cd3"
    }
    UserProfile {
        Subscriptions: "basic-kinopoisk"
        Subscriptions: "basic-music"
        Subscriptions: "basic-plus"
        Subscriptions: "station-lease-plus"
        HasYandexPlus: true
    }
    Location {
        Lat: 56.12589264
        Lon: 47.39093781
        Accuracy: 140
        Recency: 0
        Speed: 0
    }
    ChosenUtterance: "\321\207\321\202\320\276 \321\202\320\260\320\272\320\276\320\265 \320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200 \320\277\320\276 \321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
    ModifiersAnalyticsInfo {
        Proactivity {
            SemanticFramesInfo {
                Source: Begemot
                SemanticFrames {
                    Name: "personal_assistant.scenarios.translate.translation"
                    Slots {
                        Name: "phrase"
                        Type: "string"
                        Value: "\320\263\320\260\320\263\320\260\321\207 \320\277\320\276\320\273\320\264\320\265\321\200"
                        AcceptedTypes: "string"
                    }
                    Slots {
                        Name: "from_language"
                        Type: "string"
                        Value: "\321\207\321\203\320\262\320\260\321\210\321\201\320\272\320\270"
                        AcceptedTypes: "string"
                    }
                }
            }
            Source: "Translation"
        }
    }
}
)";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_TEXT_EVENT = R"(
{
    "lang":"ru-RU",
    "sequence_number":48,
    "laas_region":{
        "country_id_by_ip":159,
        "regular_coordinates":[
            {
                "lat":43.237527,
                "type":1,
                "lon":76.893519
            }
        ],
        "should_update_cookie":false,
        "city_id":162
    },
    "device_id":"bc70cb02-42d2-499b-7702-c4b6e5a7d7f9",
    "device_state":{
        "sound_level":0,
        "is_default_assistant":false,
        "sound_muted":false
    },
    "callback_args":{
        
    },
    "utterance":{
        "end_of_utterance":true,
        "payload":null,
        "input_source":"text",
        "text":"Ты будешь    Авалорай    понятно"
    },
    "test_ids":[
        412802,
        423296,
        432016
    ],
    "prev_req_id":"2adb06c6-a7ce-4139-ad81-42d7d2390602",
    "experiments":{
        "activation_search_redirect_experiment":"1",
        "afisha_poi_events":"1"
    },
    "client_time":"1639256398",
    "uuid":"6a58ac56-629e-4138-ab1d-b2289b8793e9",
    "additional_options":{
        "server_time_ms":"1639256399613",
        "supported_features":[
            "reader_app_tts",
            "whocalls",
            "div_cards",
            "messengers_calls",
            "open_link_search_viewport",
            "open_link_yellowskin"
        ],
        "bass_options":{
            "filtration_level":1,
            "screen_scale_factor":2.75,
            "client_ip":"37.99.36.233",
            "user_agent":"Mozilla\/5.0 (Linux; arm_64; Android 9; Redmi Note 8T) AppleWebKit\/537.36 (KHTML, like Gecko) Chrome\/94.0.4606.85 BroPP\/1.0 SA\/3 Mobile Safari\/537.36 YandexSearch\/21.111.1"
        },
        "unsupported_features":[
            "pwd_app_manager",
            "cloud_ui",
            "bonus_cards_camera",
            "pedometer",
            "supports_device_local_reminders",
            "whocalls_call_blocking",
            "bonus_cards_list"
        ],
        "permissions":[
            {
                "name":"location",
                "granted":true
            },
            {
                "name":"read_contacts",
                "granted":false
            },
            {
                "name":"call_phone",
                "granted":false
            }
        ],
        "icookie":"2243500680643228965",
        "divkit_version":"2.3",
        "app_info":"eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0=",
        "yandex_uid":"4340979321637685657",
        "expboxes":"341894,0,20;341896,0,10;341906,0,74;341907,0,38;341918,0,28;341921,0,19;341933,0,63;341939,0,89;341942,0,7;341926,0,53;317744,0,88;315374,0,16;457894,0,2;467484,0,27;466622,0,63;315580,0,11;330999,0,67;436514,0,52",
        "do_not_use_user_logs":false
    },
    "srcrwr":{
        
    },
    "callback_name":null,
    "voice_session":false,
    "request_id":"d4fbe418-849d-4e8e-a1da-8bcb48928e01",
    "dialog_id":null,
    "reset_session":false,
    "app_info":{
        "device_model":"Redmi Note 8T",
        "os_version":"9",
        "platform":"android",
        "app_version":"21.111",
        "app_id":"ru.yandex.searchplugin",
        "device_manufacturer":"xiaomi",
        "device_revision":""
    },
    "location":{
        "lat":43.2593232,
        "lon":76.9268675,
        "recency":39344,
        "accuracy":25.29999924
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_SUGGESTED_EVENT = R"(
{
    "lang":"ru-RU",
    "sequence_number":657,
    "laas_region":{
        "country_id_by_ip":225,
        "regular_coordinates":[
            {
                "lat":43.354443,
                "lon":43.942869,
                "type":1
            },
            {
                "lat":43.480276,
                "type":0,
                "lon":43.598461
            },
            {
                "lat":43.361409,
                "lon":43.947626,
                "type":0
            }
        ],
        "city_id":100586
    },
    "device_id":"b1c79d85-ff16-cc94-f305-c97237c1d3b4",
    "device_state":{
        "sound_level":1,
        "is_default_assistant":false,
        "sound_muted":false
    },
    "callback_args":{
        
    },
    "utterance":{
        "end_of_utterance":true,
        "payload":null,
        "input_source":"suggested",
        "text":"Да"
    },
    "test_ids":[
        412802,
        423296,
        432016
    ],
    "prev_req_id":"60166694-c06f-41f2-a764-39ef478ccc16",
    "experiments":{
        "afisha_poi_events":"1",
        "activation_search_redirect_experiment":"1"
    },
    "client_time":"1639256374",
    "uuid":"7c3d72c3-d3ed-4556-b455-980d1b4a8e59",
    "additional_options":{
        "server_time_ms":"1639256399419",
        "supported_features":[
            "whocalls_call_blocking",
            "cloud_push_implementation",
            "bonus_cards_list",
            "open_link_turbo_app",
            "open_yandex_auth",
            "music_sdk_client"
        ],
        "bass_options":{
            "filtration_level":1,
            "screen_scale_factor":1.875,
            "client_ip":"85.173.126.65",
            "user_agent":"Mozilla\/5.0 (Linux; arm_64; Android 11; SM-A125F) AppleWebKit\/537.36 (KHTML, like Gecko) Chrome\/94.0.4606.85 BroPP\/1.0 SA\/3 Mobile Safari\/537.36 YandexSearch\/21.114.1"
        },
        "unsupported_features":[
            "cloud_ui",
            "pedometer",
            "supports_device_local_reminders"
        ],
        "permissions":[
            {
                "name":"location",
                "granted":true
            },
            {
                "name":"read_contacts",
                "granted":false
            },
            {
                "name":"call_phone",
                "granted":false
            }
        ],
        "icookie":"5966103850847343358",
        "divkit_version":"2.3",
        "app_info":"eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0=",
        "yandex_uid":"9741213511628578109",
        "expboxes":"341893,0,1;341897,0,81;341906,0,82;341907,0,10;341918,0,43;341921,0,77;341935,0,1;341941,0,95;341942,0,19;341928,0,43;317744,0,60;315365,0,35;457894,0,73;462790,0,78;466622,0,62;315615,0,51;330999,0,94;436514,0,70",
        "do_not_use_user_logs":false
    },
    "srcrwr":{
        
    },
    "callback_name":null,
    "voice_session":true,
    "request_id":"2a5cd1a8-53df-4bbb-9338-2b9253f94b0d",
    "dialog_id":"f80f9b78-18cf-4a91-9d1b-96e32dfc52e0",
    "reset_session":false,
    "app_info":{
        "device_model":"SM-A125F",
        "os_version":"11",
        "platform":"android",
        "app_version":"21.114",
        "app_id":"ru.yandex.searchplugin",
        "device_manufacturer":"samsung",
        "device_revision":""
    },
    "location":{
        "lat":43.354225,
        "lon":43.94265333,
        "recency":14909,
        "accuracy":2.200000048
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_VOICE_EVENT1 = R"(
{
    "lang":"ru-RU",
    "sequence_number":109,
    "device_id":"469ae816-46a6-e840-8314-7ee0e42c46cb",
    "device_state":{
        "sound_level":7,
        "is_default_assistant":false,
        "sound_muted":false
    },
    "callback_args":{
        
    },
    "utterance":{
        "hypothesis_number":null,
        "end_of_utterance":null,
        "payload":null,
        "input_source":"voice"
    },
    "test_ids":[
        
    ],
    "prev_req_id":"a1aa5419-dec4-48c7-ab02-e2783a774488",
    "client_time":"1639255964",
    "uuid":"",
    "additional_options":{
        "oauth_token":"AQAAAAAvNhQZAAJ-IaZ********************",
        "permissions":[
            {
                "name":"location",
                "granted":true
            },
            {
                "name":"read_contacts",
                "granted":false
            },
            {
                "name":"call_phone",
                "granted":false
            }
        ],
        "supported_features":[
            "reader_app_tts",
            "whocalls",
            "div_cards",
            "music_sdk_client"
        ],
        "bass_options":{
            "region_id":38,
            "filtration_level":1,
            "screen_scale_factor":4.5,
            "cookies":[
                "..."
            ],
            "user_agent":"Mozilla\/5.0 (Linux; arm_64; Android 9; SM-N950F) AppleWebKit\/537.36 (KHTML, like Gecko) Chrome\/93.0.4577.82 BroPP\/1.0 SA\/3 Mobile Safari\/537.36 YandexSearch\/21.90.1"
        },
        "divkit_version":"2.3",
        "unsupported_features":[
            "cloud_ui",
            "bonus_cards_camera",
            "pedometer",
            "whocalls_call_blocking",
            "bonus_cards_list"
        ]
    },
    "srcrwr":{
        
    },
    "callback_name":null,
    "voice_session":true,
    "request_id":"9097376a-b433-42dc-ab58-0595e188ba53",
    "dialog_id":null,
    "reset_session":false,
    "app_info":{
        "device_model":"",
        "os_version":"",
        "platform":"",
        "app_version":"",
        "app_id":"",
        "device_manufacturer":"",
        "device_revision":""
    },
    "location":{
        "lat":48.74995422,
        "lon":44.50056839,
        "recency":78324,
        "accuracy":140
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_SERVER_EVENT = R"(
{
    "lang":"ru-RU",
    "sequence_number":811,
    "laas_region":{
        "country_id_by_ip":225,
        "city_id":2
    },
    "device_id":"041079028418181d0650",
    "device_state":{
        "device_id":"041079028418181d0650",
        "screen":{
            "supported_screen_resolutions":[
                "video_format_SD",
                "video_format_HD"
            ],
            "hdcp_level":"current_HDCP_level_none"
        },
        "timers":{
            
        },
        "video":{
            "player":{
                "pause":true
            },
            "current_screen":"music_player",
            "currently_playing":{
                "audio_language":"ru",
                "item":{
                    "name":"Пираты Карибского моря: На странных берегах",
                    "entref":"entnext=ruw1494301",
                    "directors":"Роб Маршалл",
                    "actors":"Джонни Депп, Пенелопа Крус, Джеффри Раш",
                    "cover_url_2x3":"https:\/\/avatars.mds.yandex.net\/get-ott\/224348\/2a000001690543e475e949acb5b65722b11b\/320x480",
                    "provider_info":[
                        {
                            "available":1,
                            "type":"movie",
                            "provider_item_id":"4bec4e105fcc64e9b1e3d76aae0db1ad",
                            "provider_name":"kinopoisk"
                        }
                    ],
                    "genre":"приключения, боевик, фэнтези, комедия",
                    "provider_item_id":"4bec4e105fcc64e9b1e3d76aae0db1ad",
                    "player_restriction_config":{
                        "subtitles_button_enable":1
                    },
                    "release_year":2011,
                    "provider_name":"kinopoisk",
                    "cover_url_16x9":"https:\/\/avatars.mds.yandex.net\/get-vh\/3542582\/3020706213081434205-H5oN5ZEVt4ZoKbI1bDLMBg-1595518634\/1920x1080",
                    "play_uri":"https:\/\/strm.yandex.ru\/vh-ottenc-converted\/vod-content\/4bec4e105fcc64e9b1e3d76aae0db1ad\/8093288x1620748327x02bf7fb6-57cf-44e9-bf8e-3f771e6afc8a\/dash-cenc\/ysign1=54d6a5e8e1fd224329fbb9fc3975cfbef37dca595b6919e9d7140b1764b55806,abcID=3386,from=ya-station,pfx,region=225,sfx,ts=61bf85cb\/sdr_hd_avc_aac.mpd",
                    "thumbnail_url_16x9":"https:\/\/avatars.mds.yandex.net\/get-vh\/3542582\/3020706213081434205-H5oN5ZEVt4ZoKbI1bDLMBg-1595518634\/640x360",
                    "min_age":12,
                    "type":"movie",
                    "audio_streams":[
                        {
                            "index":1,
                            "language":"rus",
                            "suggest":"Включи русскую озвучку",
                            "title":"Русский"
                        },
                        {
                            "index":2,
                            "language":"eng",
                            "suggest":"Включи английскую озвучку",
                            "title":"Английский"
                        }
                    ],
                    "rating":7.329999924,
                    "age_limit":"12",
                    "available":1,
                    "skippable_fragments":[
                        {
                            "type":"intro",
                            "end_time":42,
                            "start_time":1
                        },
                        {
                            "type":"credits",
                            "end_time":7875,
                            "start_time":7332
                        }
                    ],
                    "duration":7875,
                    "subtitles":[
                        {
                            "index":3,
                            "language":"off",
                            "suggest":"Выключи субтитры",
                            "title":"Выключены"
                        },
                        {
                            "index":4,
                            "language":"rus",
                            "suggest":"Включи субтитры",
                            "title":"Русские"
                        }
                    ],
                    "description":"Четвертый фильм пиратской серии рассказывает о поисках Источника молодости. В экспедицию за чашами отправляются капитан Барбосса (Джеффри Раш), Черная борода (Иэн Макшейн), его дочь Анжелика (Пенелопа Крус) и, конечно, Джек Воробей. В роли отца Джека вновь Кит Ричардс, а эпизодическую роль знатной дамы играет Джуди Денч. "
                },
                "progress":{
                    "duration":7875,
                    "played":7251
                },
                "last_play_timestamp":1639077581625
            },
            "last_play_timestamp":1639077581625
        },
        "sound_level":3,
        "is_tv_plugged_in":true,
        "alarm_state":{
            "icalendar":"BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-\/\/Yandex LTD\/\/NONSGML Quasar\/\/EN\r\nBEGIN:VEVENT\r\nDTSTART:20211211T043000Z\r\nDTEND:20211211T043000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20211211T043000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n",
            "max_sound_level":7,
            "currently_playing":false
        },
        "alarms_state":"BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-\/\/Yandex LTD\/\/NONSGML Quasar\/\/EN\r\nBEGIN:VEVENT\r\nDTSTART:20211211T043000Z\r\nDTEND:20211211T043000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20211211T043000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n",
        "internet_connection":{
            "neighbours":[
                {
                    "channel":6,
                    "bssid":"2c:4d:54:6c:90:80",
                    "ssid":"homeasus"
                },
                {
                    "channel":11,
                    "bssid":"f4:ec:38:a5:73:84",
                    "ssid":"rr"
                }
            ],
            "type":"Wifi_2_4GHz",
            "current":{
                "channel":6,
                "bssid":"2c:4d:54:6c:90:80",
                "ssid":"homeasus"
            }
        },
        "mics_muted":false,
        "sound_muted":false,
        "sound_max_level":10,
        "active_actions":{
            
        },
        "last_watched":{
            "videos":[
                {
                    "timestamp":1638996218,
                    "play_uri":"youtube:\/\/vSX59L7crpA",
                    "provider_item_id":"vSX59L7crpA",
                    "progress":{
                        "duration":2936
                    },
                    "provider_name":"youtube"
                },
                {
                    "timestamp":1638996246,
                    "play_uri":"youtube:\/\/a165yY_CTqg",
                    "provider_item_id":"a165yY_CTqg",
                    "progress":{
                        "duration":43,
                        "played":5
                    },
                    "provider_name":"youtube"
                }
            ],
            "movies":[
                {
                    "audio_language":"ru",
                    "timestamp":1638647441,
                    "play_uri":"https:\/\/strm.yandex.ru\/vh-ottenc-converted\/vod-content\/45a175cb535fcda68053875ca76dedb9\/8480413x1628683416xc2f76e9d-4db4-426e-9465-1f571d776884\/dash-cenc\/ysign1=7808dee8bceecd2f3b2ac387bc6a63c401ae5415242b28167882f295fa713281,abcID=1558,from=ya-station,pfx,region=225,sfx,ts=61b8ebff\/sdr_hd_avc_aac.mpd",
                    "provider_item_id":"45a175cb535fcda68053875ca76dedb9",
                    "progress":{
                        "duration":6852,
                        "played":1188
                    },
                    "provider_name":"kinopoisk"
                },
                {
                    "audio_language":"ru",
                    "timestamp":1638716171,
                    "play_uri":"https:\/\/strm.yandex.ru\/vh-ottenc-converted\/vod-content\/463ea1ab954bd0f0b85d1e4c0bb33dfd\/7727743x1609507056xa91f01e5-a65e-412f-9a06-8a53df7c774e\/dash-cenc\/ysign1=6ec0f7fba8dbe996410e0a703c22c04c4a1f85db8f0ee3faa3ea0e45f67d0118,abcID=1558,from=ya-station,pfx,region=225,sfx,ts=61ba015f\/sdr_hd_avc_aac.mpd",
                    "provider_item_id":"463ea1ab954bd0f0b85d1e4c0bb33dfd",
                    "progress":{
                        "duration":8708,
                        "played":158
                    },
                    "provider_name":"kinopoisk"
                }
            ],
            "tv_shows":[
                {
                    "item":{
                        "audio_language":"ru",
                        "provider_name":"kinopoisk",
                        "episode":1,
                        "provider_item_id":"4649cec609af1691bfde8e8a6fa8f0ef",
                        "play_uri":"https:\/\/strm.yandex.ru\/vh-ottenc-converted\/vod-content\/4649cec609af1691bfde8e8a6fa8f0ef\/7838580x1612964871x8353f778-ade6-4435-9e69-7a8bf33fc51c\/dash-cenc\/ysign1=46906897ffc8d592c4c683a7a91d12d7c2013609183a2d266197621d80789a48,abcID=1558,from=ya-station,pfx,region=225,sfx,ts=61ba362c\/sdr_hd_avc_aac.mpd",
                        "season":1,
                        "progress":{
                            "duration":2815,
                            "played":91
                        },
                        "timestamp":1638729617
                    },
                    "tv_show_item":{
                        "audio_language":"ru",
                        "timestamp":1638729617,
                        "provider_item_id":"440c3609669e63fdb685c647cd5a68c3",
                        "progress":{
                            "duration":2815,
                            "played":91
                        },
                        "provider_name":"kinopoisk"
                    }
                }
            ]
        },
        "bluetooth":{
            
        },
        "audio_player":{
            "played_ms":0,
            "offset_ms":0,
            "duration_ms":0,
            "last_stop_timestamp":0,
            "last_play_timestamp":1639256398764,
            "player_state":"Idle",
            "scenario_meta":{
                "owner":"music",
                "what_is_playing_answer":"Imanbek, Sean Paul, Sofia Reyes, песня \"Dancing On Dangerous\"",
                "@scenario_name":"HollywoodMusic"
            },
            "current_stream":{
                "stream_type":"Track",
                "subtitle":"Imanbek",
                "stream_id":"79587131",
                "title":"Dancing On Dangerous",
                "last_play_timestamp":1639256398764
            }
        },
        "rcu":{
            "is_rcu_connected":false
        },
        "device_config":{
            "content_settings":"medium",
            "child_content_settings":"children",
            "spotter":"alisa"
        }
    },
    "callback_args":{
        "@request_id":"575f06c9-65e5-40a8-b342-54c48807346f",
        "@recovery_callback":{
            "name":"music_thin_client_recovery",
            "payload":{
                "@request_id":"575f06c9-65e5-40a8-b342-54c48807346f",
                "radio":{
                    "batch_id":"b922d045-f8dc-48f4-8bbd-fa1b9d978f33.CyuE",
                    "session_id":"SuoZ-czphoqqS4hChm6-06Ca"
                },
                "playback_context":{
                    "content_id":{
                        "ids":[
                            "activity:wake-up"
                        ],
                        "type":"Radio",
                        "id":"activity:wake-up"
                    }
                },
                "@scenario_name":"HollywoodMusic"
            },
            "ignore_answer":false,
            "type":"server_action",
            "is_led_silent":false
        }
    },
    "utterance":null,
    "test_ids":[
        348361,
        409426,
        439312,
        466634
    ],
    "prev_req_id":"575f06c9-65e5-40a8-b342-54c48807346f",
    "experiments":{
        "alarm_how_long":"1",
        "alarm_snooze":"1",
        "alarm_semantic_frame":"1"
    },
    "client_time":"1639256398",
    "uuid":"24c08863-948e-7fab-e26e-a797a20fbf9c",
    "additional_options":{
        "server_time_ms":"1639256398945",
        "supported_features":[
            "relative_volume_change",
            "mordovia_webview"
        ],
        "bass_options":{
            "client_ip":"caba",
            "user_agent":"aba"
        },
        "puid":"19271160",
        "icookie":"22813390017111720",
        "app_info":"eyJicm93c2VyTmFtZSI6Ik90aGVyQXBwbGljYXRpb25zIiwiZGV2aWNlVHlwZSI6InN0YXRpb24iLCJkZXZpY2VNb2RlbCI6InlhbmRleHN0YXRpb24iLCJtb2JpbGVQbGF0Zm9ybSI6ImFuZHJvaWQifQ==",
        "yandex_uid":"19271160",
        "expboxes":"466516,0,36;461330,0,6;470915,0,51;341892,0,43;341898,0,49;341904,0,78;341907,0,80;341915,0,76;341921,0,21;341933,0,48;341940,0,99;341942,0,26;341927,0,25;329372,0,53;329376,0,45;329382,0,87;336916,0,44;336930,0,1;336934,0,16;336938,0,51;336943,0,88;336955,0,51;336958,0,41;336963,0,2;336969,0,75;336972,0,63;336978,0,36;336980,0,80;336984,0,41;336992,0,33;336994,0,35;336999,0,28;337003,0,34;458326,0,1;315360,0,66;315374,0,21;457894,0,87;462787,0,18;466634,0,73;330999,0,18",
        "do_not_use_user_logs":false
    },
    "srcrwr":{
        
    },
    "callback_name":"@@mm_stack_engine_get_next",
    "voice_session":true,
    "request_id":"75248ed2-18a3-4ec1-9f6b-798c4bfe283e",
    "dialog_id":null,
    "app_info":{
        "device_model":"Station",
        "os_version":"6.0.1",
        "platform":"android",
        "app_version":"1.0",
        "app_id":"ru.yandex.quasar.app",
        "device_manufacturer":"Yandex",
        "device_revision":""
    },
    "location":{
        "lat":59.93894958,
        "lon":30.31563568,
        "accuracy":100000
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_MUSIC_EVENT = R"(
{
    "lang":"ru-RU",
    "sequence_number":225,
    "laas_region":{
        "country_id_by_ip":225,
        "city_id":10998
    },
    "device_id":"1ee9abd6-fda8-b175-381e-3497034a4ffc",
    "device_state":{
        "music":{
            "player":{
                "timestamp":1639256314,
                "pause":true
            },
            "currently_playing":{
                "track_info":{
                    "artists":[
                        {
                            "composer":false,
                            "cover":{
                                "uri":"avatars.yandex.net\/get-music-content\/33216\/c6d507c7.p.41075\/%%",
                                "type":"from-artist-photos",
                                "prefix":"c6d507c7.p.41075\/"
                            },
                            "genres":[
                                
                            ]
                        }
                    ],
                    "availableFullWithoutPermission":false,
                    "coverUri":"avatars.yandex.net\/get-music-content\/95061\/4f3808a0.a.5307396-3\/%%",
                    "available":true,
                    "availableForPremiumUsers":true,
                    "albums":[
                        {
                            "artists":[
                                {
                                    "name":"КИНО",
                                    "composer":false,
                                    "cover":{
                                        "uri":"avatars.yandex.net\/get-music-content\/33216\/c6d507c7.p.41075\/%%",
                                        "type":"from-artist-photos",
                                        "prefix":"c6d507c7.p.41075\/"
                                    },
                                    "various":false,
                                    "id":"41075",
                                    "genres":[
                                        
                                    ]
                                }
                            ],
                            "genre":"rusrock",
                            "coverUri":"avatars.yandex.net\/get-music-content\/95061\/4f3808a0.a.5307396-3\/%%",
                            "year":2018,
                            "buy":[
                                
                            ]
                        }
                    ]
                },
                "track_id":"38633715"
            }
        },
        "sound_level":10,
        "is_default_assistant":false,
        "sound_muted":false
    },
    "callback_args":{
        
    },
    "utterance":{
        "payload":{
            "data":{
                
            },
            "error_text":"",
            "result":"not-music"
        },
        "input_source":"music",
        "text":null
    },
    "test_ids":[
        412802,
        423296,
        432016
    ],
    "prev_req_id":"90909244-a2f9-41bb-9079-894c508c2788",
    "experiments":{
        "authorized_personal_playlists":"1",
        "activation_search_redirect_experiment":"1",
        "afisha_poi_events":"1",
        "ambient_sounds_and_podcasts":"1",
        "ambient_sound":"1"
    },
    "client_time":"1639256404",
    "uuid":"84739dd5-bb8c-41f6-b251-3c730e6e0054",
    "additional_options":{
        "server_time_ms":"1639256409197",
        "supported_features":[
            "reader_app_tts",
            "whocalls",
            "cloud_push_implementation",
            "open_link_turbo_app",
            "open_yandex_auth",
            "music_sdk_client"
        ],
        "puid":"1466642495",
        "bass_options":{
            "filtration_level":1,
            "screen_scale_factor":3,
            "client_ip":"31.41.231.14",
            "user_agent":"Mozilla\/5.0 (Linux; arm_64; Android 10; STK-LX1) AppleWebKit\/537.36 (KHTML, like Gecko) Chrome\/94.0.4606.85 BroPP\/1.0 SA\/3 Mobile Safari\/537.36 YandexSearch\/21.114.1"
        },
        "unsupported_features":[
            "cloud_ui",
            "bonus_cards_camera",
            "pedometer",
            "supports_device_local_reminders",
            "bonus_cards_list"
        ],
        "permissions":[
            {
                "name":"location",
                "granted":true
            },
            {
                "name":"read_contacts",
                "granted":true
            },
            {
                "name":"call_phone",
                "granted":true
            }
        ],
        "icookie":"653694750838634051",
        "divkit_version":"2.3",
        "app_info":"eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0=",
        "yandex_uid":"7198464001627504288",
        "expboxes":"341894,0,21;341896,0,68;341905,0,35;341908,0,21;341918,0,27;341921,0,69;341934,0,39;341941,0,25;341942,0,77;341929,0,65;329372,0,94;329376,0,72;329381,0,19;336915,0,36;336931,0,50;336937,0,34;336941,0,48;336943,0,6;336954,0,33;336958,0,67;336963,0,53;336968,0,27;336974,0,19;336977,0,67;336980,0,46;336987,0,66;336992,0,35;336994,0,58;336998,0,8;337001,0,45;458325,0,49;317744,0,63;315366,0,29;442531,0,62;469471,0,21;457894,0,34;462788,0,19;466622,0,48;330999,0,9;323500,0,67;436514,0,48",
        "do_not_use_user_logs":false
    },
    "srcrwr":{
        
    },
    "callback_name":null,
    "voice_session":true,
    "request_id":"d2c5e28a-00ec-4465-acfc-414ff14e0611",
    "dialog_id":null,
    "reset_session":false,
    "app_info":{
        "device_model":"STK-LX1",
        "os_version":"10",
        "platform":"android",
        "app_version":"21.114",
        "app_id":"ru.yandex.searchplugin",
        "device_manufacturer":"HONOR",
        "device_revision":""
    },
    "location":{
        "lat":43.9037818,
        "lon":39.3348873,
        "recency":11458,
        "accuracy":22.5
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_IMAGE_EVENT = R"(
{
    "lang":"ru-RU",
    "sequence_number":10,
    "laas_region":{
        "country_id_by_ip":225,
        "city_id":117443
    },
    "device_id":"941497EE-7A7C-4663-974E-2615DA3DC9FD",
    "device_state":{
        
    },
    "callback_args":{
        "capture_mode":"photo",
        "img_url":"https:\/\/avatars.mds.yandex.net\/get-alice\/1327367\/wLOmO0XsmQ1Dx8pj4NizEQ0216\/fullocr"
    },
    "utterance":{
        "payload":{
            "data":{
                "capture_mode":"photo",
                "img_url":"https:\/\/avatars.mds.yandex.net\/get-alice\/1327367\/wLOmO0XsmQ1Dx8pj4NizEQ0216\/fullocr"
            }
        },
        "input_source":"image",
        "text":null
    },
    "test_ids":[
        412802,
        423296,
        432016,
        444881,
        446307
    ],
    "prev_req_id":"502DEA55-D385-4F75-A378-FADE1A707377",
    "experiments":{
        "activation_search_redirect_experiment":"1",
        "afisha_poi_events":"1"
    },
    "client_time":"1639256402",
    "uuid":"1356fa4b-7878-4a9c-81c3-66003c5432fc",
    "additional_options":{
        "server_time_ms":"1639256402515",
        "supported_features":[
            "open_ibro_settings",
            "whocalls",
            "whocalls_call_blocking",
            "pwd_app_manager",
            "whocalls_message_filtering",
            "cloud_ui"
        ],
        "puid":"490456318",
        "bass_options":{
            "filtration_level":0,
            "screen_scale_factor":2,
            "client_ip":"185.13.112.99",
            "user_agent":"Mozilla\/5.0 (iPhone; CPU iPhone OS 14_7 like Mac OS X) AppleWebKit\/605.1.15 (KHTML, like Gecko) Version\/14.0 YaBrowser\/21.11.5.594.10 SA\/3 Mobile\/15E148 Safari\/604.1"
        },
        "permissions":[
            {
                "name":"location",
                "status":true
            },
            {
                "name":"push_notifications",
                "status":false
            },
            {
                "name":"read_contacts",
                "status":false
            }
        ],
        "icookie":"130267771638135797",
        "divkit_version":"2.3.0",
        "app_info":"eyJicm93c2VyTmFtZSI6IllhbmRleEJyb3dzZXIiLCJkZXZpY2VUeXBlIjoidG91Y2giLCJkZXZpY2VNb2RlbCI6IiIsIm1vYmlsZVBsYXRmb3JtIjoiaXBob25lIn0=",
        "yandex_uid":"",
        "expboxes":"341893,0,38;341896,0,46;341906,0,53;341908,0,2;341916,0,67;341924,0,35;341933,0,39;341939,0,82;341943,0,75;341929,0,87;329372,0,36;329375,0,16;329379,0,12;336931,0,90;336934,0,87;336940,0,5;336944,0,3;336955,0,39;336957,0,75;336964,0,28;336970,0,9;336972,0,26;336979,0,18;336981,0,55;336985,0,58;336991,0,1;336994,0,25;336999,0,40;337004,0,64;458325,0,19;315360,0,34;315366,0,95;469472,0,47;457894,0,57;467482,0,96;466622,0,50;330999,0,69",
        "do_not_use_user_logs":false
    },
    "srcrwr":{
        
    },
    "callback_name":null,
    "voice_session":true,
    "request_id":"5BA04725-6C65-4A0D-914A-3FD86B49B126",
    "dialog_id":null,
    "reset_session":true,
    "app_info":{
        "device_model":"iPhone",
        "os_version":"14.7.1",
        "platform":"iphone",
        "app_version":"2111.5.594",
        "app_id":"ru.yandex.mobile.search",
        "device_manufacturer":"Apple",
        "device_revision":""
    },
    "location":{
        "lat":55.5461362,
        "lon":37.59306089,
        "recency":151,
        "accuracy":2000
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_REQUEST_VOICE_EVENT2 = R"(
{
    "lang":"ru-RU",
    "sequence_number":109,
    "device_id":"469ae816-46a6-e840-8314-7ee0e42c46cb",
    "device_state":{
        "sound_level":7,
        "is_default_assistant":false,
        "sound_muted":false
    },
    "callback_args":{
        
    },
    "utterance":{
        "hypothesis_number":111,
        "end_of_utterance":true,
        "payload":null,
        "input_source":"voice",
        "text":"алиса ты знаешь"
    },
    "test_ids":[
        
    ],
    "prev_req_id":"a1aa5419-dec4-48c7-ab02-e2783a774488",
    "client_time":"1639255964",
    "uuid":"",
    "additional_options":{
        "oauth_token":"AQAAAAAvNhQZAAJ-IaZ********************",
        "permissions":[
            {
                "name":"location",
                "granted":true
            },
            {
                "name":"read_contacts",
                "granted":false
            },
            {
                "name":"call_phone",
                "granted":false
            }
        ],
        "supported_features":[
            "reader_app_tts",
            "whocalls",
            "div_cards",
            "music_sdk_client"
        ],
        "bass_options":{
            "region_id":38,
            "filtration_level":1,
            "screen_scale_factor":4.5,
            "cookies":[
                "..."
            ],
            "user_agent":"Mozilla\/5.0 (Linux; arm_64; Android 9; SM-N950F) AppleWebKit\/537.36 (KHTML, like Gecko) Chrome\/93.0.4577.82 BroPP\/1.0 SA\/3 Mobile Safari\/537.36 YandexSearch\/21.90.1"
        },
        "divkit_version":"2.3",
        "unsupported_features":[
            "cloud_ui",
            "bonus_cards_camera",
            "pedometer",
            "whocalls_call_blocking",
            "bonus_cards_list"
        ]
    },
    "srcrwr":{
        
    },
    "callback_name":null,
    "voice_session":true,
    "request_id":"9097376a-b433-42dc-ab58-0595e188ba53",
    "dialog_id":null,
    "reset_session":false,
    "app_info":{
        "device_model":"",
        "os_version":"",
        "platform":"",
        "app_version":"",
        "app_id":"",
        "device_manufacturer":"",
        "device_revision":""
    },
    "enviroment_state":[
        {
            "device_state":{
                "sound_level":0,
                "mics_muted":false,
                "device_id":"",
                "sound_max_level":0,
                "is_default_assistant":false,
                "is_tv_plugged_in":false,
                "subscription_state":{
                    "subscription":"yandex_subscription"
                },
                "sound_muted":false,
                "alarms_state":""
            },
            "supported_features":[
                "relative_volume_change"
            ],
            "application":{
                "device_manufacturer":"",
                "lang":"",
                "platform":"",
                "device_model":"yandexmicro",
                "quasmodrom_subgroup":"",
                "device_id":"LP00000000000009757900004c178c9e",
                "uuid":"e971d3ac7722a7cb5ef57882fdae813b",
                "device_color":"",
                "app_version":"1.0",
                "os_version":"",
                "client_time":"",
                "device_revision":"",
                "quasmodrom_group":"",
                "timestamp":"",
                "timezone":"",
                "app_id":"aliced"
            }
        }
    ],
    "location":{
        "lat":48.74995422,
        "lon":44.50056839,
        "recency":78324,
        "accuracy":140
    }
})";

constexpr TStringBuf JSON_RESPONSE1 = R"(
{
    "header":{
        "response_id":"deadbeef",
        "request_id":"RequestId",
        "dialog_id":null,
        "ref_message_id":"RefMessageId",
        "session_id":"SessionId"
    },
    "response":{
        "card":{
            "type":"simple_text",
            "text":"Text"
        },
        "cards":[
            {
                "type":"simple_text",
                "text":"Text"
            }
        ],
        "experiments":{
            
        },
        "directives":[
            
        ]
    },
    "voice_response":{
        "output_speech":{
            "text":"Voice",
            "type":"simple"
        },
        "should_listen":false
    }
})";

constexpr TStringBuf JSON_RESPONSE2 = R"(
{
    "header":{
        "response_id":"deadbeef",
        "request_id":"RequestId",
        "dialog_id":null,
        "ref_message_id":"RefMessageId",
        "session_id":"SessionId"
    },
    "response":{
        "card":{
            "type":"simple_text",
            "text":"Text"
        },
        "cards":[
            {
                "type":"simple_text",
                "text":"Text"
            }
        ],
        "experiments":{
            
        },
        "directives":[
            
        ],
        "force_voice_answer":true,
        "special_buttons":[
            
        ],
        "suggest":{
            "items":[
                1,
                2,
                3
            ]
        }
    },
    "voice_response":{
        "should_listen":false
    }
})";

constexpr TStringBuf JSON_EMPTY_RESPONSE1 = R"(
{
    "header":{
        "response_id":"deadbeef",
        "request_id":"RequestId",
        "dialog_id":null,
        "ref_message_id":"RefMessageId",
        "session_id":"SessionId"
    },
    "voice_response":{
        "should_listen":false
    }
})";

constexpr TStringBuf JSON_EMPTY_RESPONSE2 = R"(
{
    "header":{
        "response_id":"deadbeef",
        "request_id":"RequestId",
        "dialog_id":null,
        "ref_message_id":"RefMessageId",
        "session_id":"SessionId"
    },
    "response": "...",
    "voice_response":{
        "should_listen":false
    }
})";

constexpr TStringBuf JSON_VINS_LIKE_RESPONSE1 = R"(
{
    "directives":[
        
    ],
    "experiments":{
        
    },
    "force_voice_answer":false,
    "suggests":[
        
    ],
    "special_buttons":[
        
    ],
    "should_listen":false,
    "voice_text":"Voice",
    "cards":[
        {
            "type":"simple_text",
            "text":"Text"
        }
    ]
})";

constexpr TStringBuf JSON_VINS_LIKE_RESPONSE2 = R"(
{
    "directives":[
        
    ],
    "experiments":{
        
    },
    "force_voice_answer":true,
    "suggests":[
        1,
        2,
        3
    ],
    "should_listen":false,
    "special_buttons":[
        
    ],
    "voice_text":null,
    "cards":[
        {
            "type":"simple_text",
            "text":"Text"
        }
    ]
})";

constexpr TStringBuf JSON_VINS_LIKE_LOG = R"*(
{
    "response_id":"bb97fc42-759322b0-e061eff8-978cfb2d",
    "lang":"ru-RU",
    "utterance_source":"voice",
    "request":{
        "lang":"ru-RU",
        "sequence_number":84,
        "laas_region":{
            "suspected_longitude":47.390862,
            "is_yandex_staff":false,
            "country_id_by_ip":225,
            "probable_regions":[
                {
                    "region_id":45,
                    "weight":1
                }
            ],
            "is_hosting":false,
            "region_home":45,
            "region_id":217096,
            "latitude":56.125889,
            "location_accuracy":140,
            "is_serp_trusted_net":false,
            "precision":2,
            "suspected_location_accuracy":140,
            "suspected_location_unixtime":1630184120,
            "is_yandex_net":false,
            "is_public_proxy":false,
            "is_mobile":false,
            "suspected_region_city":45,
            "regular_coordinates":[
                {
                    "lat":56.125641,
                    "type":1,
                    "lon":47.389925
                },
                {
                    "lat":56.11174,
                    "lon":47.488506,
                    "type":0
                },
                {
                    "lat":56.118437,
                    "lon":47.452439,
                    "type":0
                }
            ],
            "is_tor":false,
            "region_by_ip":45,
            "is_user_choice":false,
            "is_gdpr":false,
            "longitude":47.390862,
            "probable_regions_reliability":1,
            "is_anonymous_vpn":false,
            "suspected_precision":2,
            "location_unixtime":1630184120,
            "should_update_cookie":false,
            "suspected_region_id":217096,
            "city_id":45,
            "suspected_latitude":56.125889
        },
        "device_id":"FF98F029669E41603B6B4993",
        "device_state":{
            "sound_level":4,
            "bluetooth":{
                
            },
            "music":{
                "player":{
                    "timestamp":1630178528,
                    "pause":true
                },
                "playlist_owner":"",
                "currently_playing":{
                    "track_info":{
                        "previewDurationMs":30000,
                        "fileSize":0,
                        "storageDir":"",
                        "realId":"79476206",
                        "batchInfo":{
                            "durationMs":204070,
                            "type":"dynamic",
                            "syncRid":"sync-1383eba5-8e0d-479c-a8da-da2da125d8fe",
                            "albumId":"14445807",
                            "general":false,
                            "rid":"804431da-f24e-4e77-8e64-1197fcc13319",
                            "itemType":"track"
                        },
                        "rememberPosition":true,
                        "id":"79476206",
                        "artists":[
                            {
                                "cover":{
                                    "uri":"avatars.yandex.net/get-music-content/4399644/dd140ebd.a.14443769-1/%%",
                                    "type":"from-album-cover",
                                    "prefix":"dd140ebd.a.14443769-1"
                                },
                                "name":"Детская аудиокнига",
                                "composer":false,
                                "various":false,
                                "id":11028533,
                                "genres":[
                                    
                                ]
                            }
                        ],
                        "ogImage":"avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%",
                        "batchId":"sync-1383eba5-8e0d-479c-a8da-da2da125d8fe",
                        "durationMs":204070,
                        "major":{
                            "name":"ONERPM",
                            "id":308
                        },
                        "type":"fairy-tale",
                        "normalization":{
                            "gain":-3.4,
                            "peak":32766
                        },
                        "isSuitableForChildren":true,
                        "available":true,
                        "title":"Азербайджанская сказка — Лис и куропатка",
                        "coverUri":"avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%",
                        "trackSharingFlag":"COVER_ONLY",
                        "availableFullWithoutPermission":false,
                        "availableForPremiumUsers":true,
                        "albums":[
                            {
                                "bests":[
                                    79476216,
                                    79476203,
                                    79476215
                                ],
                                "availablePartially":false,
                                "buy":[
                                    
                                ],
                                "genre":"fairytales",
                                "availableForMobile":true,
                                "artists":[
                                    {
                                        "cover":{
                                            "uri":"avatars.yandex.net/get-music-content/4399644/dd140ebd.a.14443769-1/%%",
                                            "type":"from-album-cover",
                                            "prefix":"dd140ebd.a.14443769-1"
                                        },
                                        "composer":false,
                                        "name":"Детская аудиокнига",
                                        "various":false,
                                        "id":11028533,
                                        "genres":[
                                            
                                        ]
                                    }
                                ],
                                "trackPosition":{
                                    "volume":1,
                                    "index":5
                                },
                                "id":14445807,
                                "labels":[
                                    {
                                        "name":"Детская аудиокнига",
                                        "id":2942036
                                    }
                                ],
                                "ogImage":"avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%",
                                "releaseDate":"2021-03-12T00:00:00+03:00",
                                "likesCount":273,
                                "type":"fairy-tale",
                                "coverUri":"avatars.yandex.net/get-music-content/4304260/92636db0.a.14445807-1/%%",
                                "available":true,
                                "title":"Сказки народов мира",
                                "childContent":true,
                                "trackCount":21,
                                "year":2021,
                                "metaType":"children",
                                "availableForPremiumUsers":true,
                                "recent":false,
                                "veryImportant":false,
                                "version":"радиоспектакли"
                            }
                        ],
                        "r128":{
                            "i":-17.21,
                            "tp":0.02
                        },
                        "lyricsAvailable":false
                    },
                    "track_id":"79476206",
                    "last_play_timestamp":1630176825288
                },
                "last_play_timestamp":1630176825288,
                "session_id":"em7ezuNz"
            },
            "alarm_state":{
                "sound_alarm_setting":{
                    "type":"music",
                    "info":{
                        "filters":{
                            "personality":"is_user_stream"
                        }
                    }
                },
                "icalendar":"BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20210814T060000Z\r\nDTEND:20210814T060000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20210814T060000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n",
                "max_sound_level":7,
                "currently_playing":false
            },
            "mics_muted":false,
            "device_id":"FF98F029669E41603B6B4993",
            "sound_max_level":10,
            "internet_connection":{
                "neighbours":[
                    {
                        "channel":1,
                        "bssid":"f8:f0:82:5b:dc:6e",
                        "ssid":"InfoLink-Wi-Fi."
                    },
                    {
                        "channel":7,
                        "bssid":"08:c6:b3:2f:10:a0",
                        "ssid":"MegaFon_2.4G_2F109F"
                    }
                ],
                "type":"Wifi_2_4GHz",
                "current":{
                    "channel":1,
                    "bssid":"f8:f0:82:5b:dc:6e",
                    "ssid":"InfoLink-Wi-Fi."
                }
            },
            "timers":{
                
            },
            "audio_player":{
                "played_ms":151000,
                "offset_ms":151000,
                "duration_ms":151000,
                "last_stop_timestamp":1630099317052,
                "last_play_timestamp":1630099164919,
                "player_state":"Finished",
                "scenario_meta":{
                    "owner":"music",
                    "what_is_playing_answer":"Олег Анофриев, песня \"Спят усталые игрушки\"",
                    "@scenario_name":"HollywoodMusic"
                },
                "current_stream":{
                    "stream_type":"Track",
                    "subtitle":"Олег Анофриев",
                    "stream_id":"6098528",
                    "title":"Спят усталые игрушки",
                    "last_play_timestamp":1630099164919
                }
            },
            "is_tv_plugged_in":false,
            "last_watched":{
                
            },
            "device_config":{
                "content_settings":"children",
                "child_content_settings":"safe",
                "spotter":"alisa"
            },
            "sound_muted":false,
            "alarms_state":"BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20210814T060000Z\r\nDTEND:20210814T060000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20210814T060000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n"
        },
        "callback_args":{
            
        },
        "utterance":{
            "hypothesis_number":114,
            "end_of_utterance":true,
            "payload":null,
            "input_source":"voice",
            "text":"что такое гагач полдер по чувашски"
        },
        "test_ids":[
            348361,
            383587,
            378426,
            400238,
            375477,
            391026,
            341893,
            341898,
            341906,
            341910,
            341916,
            341923,
            341934,
            341938,
            341942,
            341928,
            329372,
            329377,
            329379,
            336918,
            336932,
            336937,
            336938,
            336943,
            336952,
            336959,
            336964,
            336969,
            336973,
            336977,
            336983,
            336986,
            336990,
            336996,
            336998,
            337003,
            404842,
            315360,
            409851,
            315365,
            405527,
            315614
        ],
        "prev_req_id":"f5309b1a-50ee-474d-ace3-4c7723370bf4",
        "experiments":{
            "music_session":"1",
            "enable_multiroom":"1",
            "video_disable_films_webview_searchscreen":"1",
            "mm_enable_session_reset":"1",
            "personal_tv_help":"1",
            "music":"1",
            "enable_biometry_scoring":"1",
            "vins_music_play_force_irrelevant":"1",
            "music_biometry":"1",
            "enable_reminders_todos":"1",
            "mm_disable_music":"1",
            "audio_bitrate192":"1",
            "fairytale_fallback":"1",
            "quasar_biometry_limit_users":"1",
            "mordovia":"1",
            "music_exp__dj_rl@no":"1",
            "video_omit_youtube_restriction":"1",
            "translate":"1",
            "cachalot_mm_context_save":"1",
            "read_factoid_source":"1",
            "enable_tts_gpu":"1",
            "mm_protocol_priority_scenario_early_win":"1",
            "shopping_list":"1",
            "mm_enable_partial_preclassifier":"1",
            "alarm_snooze":"1",
            "fm_radio_recommend":"1",
            "change_alarm_sound_music":"1",
            "ignore_trash_classified_results":"1",
            "hw_enable_morning_show":"1",
            "new_nlg":"1",
            "video_disable_webview_searchscreen":"1",
            "personalization":"1",
            "enable_partials":"1",
            "enable_ner_for_skills":"1",
            "music_sing_song":"1",
            "mm_enable_protocol_scenario=HollywoodHardcodedMusic":"1",
            "recurring_purchase":"1",
            "use_contacts":"1",
            "taxi_nlu":"1",
            "music_play_fm_radio_on_attempt=2":"1",
            "use_memento":"1",
            "mm_enable_protocol_scenario=AliceShow":"1",
            "mm_disable_protocol_scenario=MordoviaVideoSelection":"1",
            "hw_music_thin_client_playlist":"1",
            "radio_play_in_quasar":"1",
            "use_trash_talk_classifier":"1",
            "film_gallery":"1",
            "video_not_use_native_youtube_api":"1",
            "context_load_apply":"1",
            "taxi":"1",
            "new_special_playlists":"1",
            "hw_music_thin_client":"1",
            "k_schastiyu_dlya_companii":"1",
            "weather_precipitation_type":"1",
            "new_fairytale_quasar":"1",
            "vins_e2e_partials":"1",
            "uniproxy_vins_sessions":"1",
            "quasar_tv":"1",
            "bg_enable_player_next_track_v2":"1",
            "fairytale_search_text_noprefix":"1",
            "change_alarm_sound_radio":"1",
            "quasar":"1",
            "play_radio_if_no_plus":"1",
            "weather_precipitation":"1",
            "podcasts":"1",
            "mm_enable_player_features":"1",
            "medium_ru_explicit_content":"1",
            "tv_stream":"1",
            "tv_without_channel_status_check":"1",
            "ether":"https://yandex.ru/portal/station/main",
            "mordovia_support_channels":"1",
            "mm_partial_preclassifier_threshold=0.0322":"1",
            "change_alarm_with_sound":"1",
            "tv":"1",
            "hw_enable_morning_show_good_morning":"1",
            "tv_vod_translation":"1",
            "music_show_first_track":"1",
            "personal_tv_channel":"1",
            "tts_domain_music":"1",
            "pure_general_conversation":"1",
            "market_disable":"1",
            "hw_enable_evening_show":"1",
            "enable_timers_alarms":"1",
            "quasar_gc_instead_of_search":"1",
            "mordovia_long_listening":"1",
            "market_orders_status_disable":"1",
            "video_qproxy_players":"1",
            "username_auto_insert":"1",
            "music_personalization":"1",
            "music_partials":"1",
            "dialog_4178_newcards":"1",
            "disable_interruption_spotter":"1",
            "drm_tv_stream":"1",
            "market_beru_disable":"1",
            "ambient_sound":"1",
            "kv_saas_activation_experiment":"1",
            "dj_service_for_games_onboarding":"1",
            "bg_fresh_granet_experiment=bg_enable_player_next_track_v2":"1",
            "weather_precipitation_starts_ends":"1",
            "music_use_websearch":"1",
            "change_alarm_sound":"1",
            "biometry_like":"1",
            "music_check_plus_promo":"1",
            "how_much":"1",
            "iot":"1",
            "radio_play_onboarding":"1",
            "general_conversation":"1",
            "ugc_enabled":"1",
            "sleep_timers":"1",
            "radio_fixes":"1",
            "music_recognizer":"1",
            "biometry_remove":"1",
            "supress_multi_activation":"1",
            "new_music_radio_nlg":"1",
            "alarm_how_long":"1"
        },
        "client_time":"1630184395",
        "uuid":"58341b91-fe76-e4b6-4fa9-604a2d2893f0",
        "additional_options":{
            "server_time_ms":"1630184399523",
            "supported_features":[
                "multiroom",
                "multiroom_cluster",
                "multiroom_audio_client",
                "change_alarm_sound",
                "change_alarm_sound_level",
                "music_player_allow_shots",
                "bluetooth_player",
                "audio_client",
                "audio_client_hls",
                "notifications",
                "tts_play_placeholder",
                "incoming_messenger_calls",
                "publicly_available",
                "directive_sequencer",
                "set_alarm_semantic_frame_v2",
                "muzpult",
                "audio_bitrate192",
                "audio_bitrate320",
                "prefetch_invalidation"
            ],
            "bass_options":{
                "client_ip":"79.133.141.197",
                "user_agent":"Mozilla\\/5.0 (Linux; Android 6.0.1; Station Build\\/MOB30J; wv) AppleWebKit\\/537.36 (KHTML, like Gecko) Version\\/4.0 Chrome\\/61.0.3163.98 Safari\\/537.36 YandexStation\\/2.3.0.3.373060213.20190204.develop.ENG (YandexIO)"
            },
            "puid":"1425164113",
            "icookie":"9014366150703152919",
            "app_info":"eyJicm93c2VyTmFtZSI6Ik90aGVyQXBwbGljYXRpb25zIiwiZGV2aWNlVHlwZSI6InN0YXRpb25fbWluaSIsImRldmljZU1vZGVsIjoieWFuZGV4bWluaSIsIm1vYmlsZVBsYXRmb3JtIjoiYW5kcm9pZCJ9",
            "yandex_uid":"1425164113",
            "expboxes":"375477,0,55;391026,0,43;341893,0,15;341898,0,22;341906,0,81;341910,0,43;341916,0,6;341923,0,54;341934,0,65;341938,0,10;341942,0,98;341928,0,93;329372,0,25;329377,0,99;329379,0,32;336918,0,80;336932,0,76;336937,0,13;336938,0,57;336943,0,1;336952,0,47;336959,0,98;336964,0,86;336969,0,16;336973,0,32;336977,0,82;336983,0,51;336986,0,75;336990,0,83;336996,0,42;336998,0,8;337003,0,86;404842,0,99;315360,0,51;409851,0,4;315365,0,35;405527,0,42;315614,0,21;330999,0,52",
            "do_not_use_user_logs":false
        },
        "srcrwr":{
            
        },
        "callback_name":null,
        "voice_session":true,
        "request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
        "dialog_id":null,
        "app_info":{
            "device_model":"yandexmini",
            "os_version":"1.0",
            "platform":"Linux",
            "app_version":"1.0",
            "app_id":"aliced",
            "device_manufacturer":"Yandex",
            "device_revision":""
        },
        "location":{
            "lat":56.12589264,
            "lon":47.39093781,
            "accuracy":140
        }
    },
    "device_id":"FF98F029669E41603B6B4993",
    "app_id":"pa",
    "location_lon":47.39093781,
    "callback_args":{
        
    },
    "experiments":{
        "music_session":"1",
        "enable_multiroom":"1",
        "video_disable_films_webview_searchscreen":"1",
        "mm_enable_session_reset":"1",
        "personal_tv_help":"1",
        "music":"1",
        "enable_biometry_scoring":"1",
        "vins_music_play_force_irrelevant":"1",
        "music_biometry":"1",
        "enable_reminders_todos":"1",
        "mm_disable_music":"1",
        "audio_bitrate192":"1",
        "fairytale_fallback":"1",
        "quasar_biometry_limit_users":"1",
        "mordovia":"1",
        "music_exp__dj_rl@no":"1",
        "video_omit_youtube_restriction":"1",
        "translate":"1",
        "cachalot_mm_context_save":"1",
        "read_factoid_source":"1",
        "enable_tts_gpu":"1",
        "mm_protocol_priority_scenario_early_win":"1",
        "shopping_list":"1",
        "mm_enable_partial_preclassifier":"1",
        "alarm_snooze":"1",
        "fm_radio_recommend":"1",
        "change_alarm_sound_music":"1",
        "ignore_trash_classified_results":"1",
        "hw_enable_morning_show":"1",
        "new_nlg":"1",
        "video_disable_webview_searchscreen":"1",
        "personalization":"1",
        "enable_partials":"1",
        "enable_ner_for_skills":"1",
        "music_sing_song":"1",
        "mm_enable_protocol_scenario=HollywoodHardcodedMusic":"1",
        "recurring_purchase":"1",
        "use_contacts":"1",
        "taxi_nlu":"1",
        "music_play_fm_radio_on_attempt=2":"1",
        "use_memento":"1",
        "mm_enable_protocol_scenario=AliceShow":"1",
        "mm_disable_protocol_scenario=MordoviaVideoSelection":"1",
        "hw_music_thin_client_playlist":"1",
        "radio_play_in_quasar":"1",
        "use_trash_talk_classifier":"1",
        "film_gallery":"1",
        "video_not_use_native_youtube_api":"1",
        "context_load_apply":"1",
        "taxi":"1",
        "new_special_playlists":"1",
        "hw_music_thin_client":"1",
        "k_schastiyu_dlya_companii":"1",
        "weather_precipitation_type":"1",
        "new_fairytale_quasar":"1",
        "vins_e2e_partials":"1",
        "uniproxy_vins_sessions":"1",
        "quasar_tv":"1",
        "bg_enable_player_next_track_v2":"1",
        "fairytale_search_text_noprefix":"1",
        "change_alarm_sound_radio":"1",
        "quasar":"1",
        "play_radio_if_no_plus":"1",
        "weather_precipitation":"1",
        "podcasts":"1",
        "mm_enable_player_features":"1",
        "medium_ru_explicit_content":"1",
        "tv_stream":"1",
        "tv_without_channel_status_check":"1",
        "ether":"https://yandex.ru/portal/station/main",
        "mordovia_support_channels":"1",
        "mm_partial_preclassifier_threshold=0.0322":"1",
        "change_alarm_with_sound":"1",
        "tv":"1",
        "hw_enable_morning_show_good_morning":"1",
        "tv_vod_translation":"1",
        "music_show_first_track":"1",
        "personal_tv_channel":"1",
        "tts_domain_music":"1",
        "pure_general_conversation":"1",
        "market_disable":"1",
        "hw_enable_evening_show":"1",
        "enable_timers_alarms":"1",
        "quasar_gc_instead_of_search":"1",
        "mordovia_long_listening":"1",
        "market_orders_status_disable":"1",
        "video_qproxy_players":"1",
        "username_auto_insert":"1",
        "music_personalization":"1",
        "music_partials":"1",
        "dialog_4178_newcards":"1",
        "disable_interruption_spotter":"1",
        "drm_tv_stream":"1",
        "market_beru_disable":"1",
        "ambient_sound":"1",
        "kv_saas_activation_experiment":"1",
        "dj_service_for_games_onboarding":"1",
        "bg_fresh_granet_experiment=bg_enable_player_next_track_v2":"1",
        "weather_precipitation_starts_ends":"1",
        "music_use_websearch":"1",
        "change_alarm_sound":"1",
        "biometry_like":"1",
        "music_check_plus_promo":"1",
        "how_much":"1",
        "iot":"1",
        "radio_play_onboarding":"1",
        "general_conversation":"1",
        "ugc_enabled":"1",
        "sleep_timers":"1",
        "radio_fixes":"1",
        "music_recognizer":"1",
        "biometry_remove":"1",
        "supress_multi_activation":"1",
        "new_music_radio_nlg":"1",
        "alarm_how_long":"1"
    },
    "biometry_classification":{
        "scores":[
            {
                "classname":"adult",
                "tag":"children",
                "confidence":0.98682326
            },
            {
                "classname":"child",
                "tag":"children",
                "confidence":0.01317676
            },
            {
                "classname":"female",
                "tag":"gender",
                "confidence":0.053047426
            },
            {
                "classname":"male",
                "tag":"gender",
                "confidence":0.9469526
            }
        ],
        "status":"ok",
        "simple":[
            {
                "classname":"adult",
                "tag":"children"
            },
            {
                "classname":"male",
                "tag":"gender"
            }
        ]
    },
    "response":{
        "directives":[
            
        ],
        "experiments":{
            
        },
        "force_voice_answer":false,
        "quality_storage":{
            "post_win_reason":"WR_PRIORITY",
            "scenarios_information":{
                "IoTGranet":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Dialogovo":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "PhoneNotification":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Bugreport":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Transcription":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ExternalSkillFlashBriefing":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "News":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Covid19":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ZapravkiB2B":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Repeat":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ZenSearch":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "VideoPlayConcert":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Food":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "MessengerCall":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "MovieSuggest":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "GeneralConversationHeavy":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "SwitchLayerTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ZeroTesting":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "SimSim":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "GetWeatherTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "LinkARemote":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Vins":{
                    "reason":"LR_PRIORITY"
                },
                "VideoMusicalClips":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Bluetooth":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "IoTVoiceDiscovery":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "PhotoFrame":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "IoTScenarios":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "RandomNumber":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "VideoTrailer":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "DriveOrder":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Alarm":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Miles":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "SideSpeech":{
                    "reason":"LR_PRIORITY"
                },
                "SubscriptionsManager":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "NotificationsManager":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Search":{
                    "reason":"LR_PRIORITY"
                },
                "ImageWhatIsThis":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "GameSuggest":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "CecCommands":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "DrawPicture":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "SkillsDiscovery":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HappyNewYear":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HollywoodHardcodedMusic":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "VideoRater":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "MordoviaVideoSelection":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "DoNothing":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "OpenAppsFixlist":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "AutomotiveRadio":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ShowTrafficBass":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Theremin":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Commands":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Taximeter":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Reask":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "FindPoiTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Reminders":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "GetMyLocationTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "NaviExternalConfirmationTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HandcraftedTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "MapsDownloadOffline":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HollywoodAlarm":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HollywoodMusic":{
                    "reason":"LR_PRIORITY"
                },
                "Centaur":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ShowTraffic":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "GeneralConversationTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ShowRouteTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ShowTvChannelsGallery":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Route":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "MarketOrdersStatus":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "VideoCommand":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "TransformFace":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "GeneralConversation":{
                    "reason":"LR_PRIORITY"
                },
                "AliceShow":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Wizard":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Alice4Business":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "DiskMyPhotos":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "OnboardingCriticalUpdate":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "MarketHowMuch":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Video":{
                    "reason":"LR_PRIORITY"
                },
                "DialogovoB2b":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "Weather":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HollywoodZeroTesting":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "HardcodedResponse":{
                    "reason":"LR_IRRELEVANT"
                },
                "AddPointTr":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "TvChannels":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ShowGif":{
                    "reason":"LR_NOT_ALLOWED"
                },
                "ExternalSkillRecipes":{
                    "reason":"LR_NOT_ALLOWED"
                }
            },
            "pre_predicts":{
                "SideSpeech":0,
                "IoT":0,
                "SkillDiscoveryGc":0,
                "HardcodedResponse":0,
                "Translation":0,
                "Video":-5.4341712,
                "HollywoodMusic":-5.47678423,
                "Vins":-0.787874043,
                "GeneralConversation":0,
                "Search":1.15632915
            },
            "post_predicts":{
                "GeneralConversation":0,
                "Video":-4.38396311,
                "Vins":0.0404732749,
                "Translation":0,
                "Search":0.460568786,
                "SideSpeech":-2.4131763,
                "HollywoodMusic":-3.54384041
            }
        },
        "templates":{
            
        },
        "suggests":[
            {
                "type":"action",
                "title":"Повтори",
                "directives":[
                    {
                        "name":"type",
                        "payload":{
                            "text":"Повтори"
                        },
                        "sub_name":"render_buttons_type",
                        "type":"client_action"
                    },
                    {
                        "name":"on_suggest",
                        "payload":{
                            "button_id":"a40b02f-6e1c4901-c986dc3f-3b0215f4",
                            "@request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "caption":"Повтори",
                            "request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "@scenario_name":"Vins",
                            "scenario_name":"Translation"
                        },
                        "ignore_answer":true,
                        "type":"server_action",
                        "is_led_silent":true
                    }
                ]
            },
            {
                "type":"action",
                "title":"А на хорватском?",
                "directives":[
                    {
                        "name":"type",
                        "payload":{
                            "text":"А на хорватском?"
                        },
                        "sub_name":"render_buttons_type",
                        "type":"client_action"
                    },
                    {
                        "name":"on_suggest",
                        "payload":{
                            "@request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "button_id":"2e213e69-6c2beab9-d1d55084-dc2e8c6e",
                            "caption":"А на хорватском?",
                            "request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "scenario_name":"Translation",
                            "@scenario_name":"Vins"
                        },
                        "ignore_answer":true,
                        "type":"server_action",
                        "is_led_silent":true
                    }
                ]
            },
            {
                "type":"action",
                "title":"А на хинди?",
                "directives":[
                    {
                        "name":"type",
                        "payload":{
                            "text":"А на хинди?"
                        },
                        "sub_name":"render_buttons_type",
                        "type":"client_action"
                    },
                    {
                        "name":"on_suggest",
                        "payload":{
                            "button_id":"70605933-a3ee2c65-7b5bfa10-39a50e4a",
                            "@request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "caption":"А на хинди?",
                            "request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "scenario_name":"Translation",
                            "@scenario_name":"Vins"
                        },
                        "ignore_answer":true,
                        "type":"server_action",
                        "is_led_silent":true
                    }
                ]
            },
            {
                "type":"action",
                "title":"А на арабском?",
                "directives":[
                    {
                        "name":"type",
                        "payload":{
                            "text":"А на арабском?"
                        },
                        "sub_name":"render_buttons_type",
                        "type":"client_action"
                    },
                    {
                        "name":"on_suggest",
                        "payload":{
                            "button_id":"74a63a26-665f6365-4227206d-38470285",
                            "@request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "caption":"А на арабском?",
                            "request_id":"8b464f68-df7b-485c-a81b-0640051c9f49",
                            "scenario_name":"Translation",
                            "@scenario_name":"Vins"
                        },
                        "ignore_answer":true,
                        "type":"server_action",
                        "is_led_silent":true
                    }
                ]
            }
        ],
        "special_buttons":[
            
        ],
        "should_listen":false,
        "voice_text":"Подозреваю, что гагач полдер <speaker voice=\"shitova\"> - это полдер гагач",
        "directives_execution_policy":"BeforeSpeech",
        "cards":[
            {
                "type":"simple_text",
                "text":"Подозреваю, что гагач полдер - это полдер гагач"
            }
        ]
    },
    "client_time":1630184395,
    "uuid":"58341b91-fe76-e4b6-4fa9-604a2d2893f0",
    "callback_name":null,
    "biometry_scoring":{
        "status":"ok",
        "group_id":"aef372b7d1ae0d50ab9f60542314d414",
        "request_id":"8b464f68-df7b-485c-a81b-0640051c9f49"
    },
    "client_tz":"Europe/Moscow",
    "type":"UTTERANCE",
    "location_lat":56.12589264,
    "utterance_text":"что такое гагач полдер по чувашски",
    "provider":"megamind",
    "analytics_info":{
        "modifiers_info":{
            "proactivity":{
                "semantic_frames_info":{
                    "semantic_frames":[
                        {
                            "name":"personal_assistant.scenarios.translate.translation",
                            "slots":[
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"phrase",
                                    "value":"гагач полдер",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"from_language",
                                    "value":"чувашски",
                                    "type":"string"
                                }
                            ]
                        }
                    ],
                    "source":"Begemot"
                },
                "source":"Translation"
            }
        },
        "modifiers_analytics_info":{
            "proactivity":{
                "semantic_frames_info":{
                    "semantic_frames":[
                        {
                            "name":"personal_assistant.scenarios.translate.translation",
                            "slots":[
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"phrase",
                                    "value":"гагач полдер",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"from_language",
                                    "value":"чувашски",
                                    "type":"string"
                                }
                            ]
                        }
                    ],
                    "source":"Begemot"
                },
                "source":"Translation"
            }
        },
        "location":{
            "speed":0,
            "lat":56.12589264,
            "lon":47.39093781,
            "recency":0,
            "accuracy":140
        },
        "scenario_timings":{
            "SideSpeech":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399598758"
                    }
                }
            },
            "HardcodedResponse":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399597204"
                    }
                }
            },
            "Translation":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399599427"
                    }
                }
            },
            "Video":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399600396"
                    }
                }
            },
            "HollywoodMusic":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399597982"
                    }
                }
            },
            "Vins":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399601309"
                    }
                }
            },
            "GeneralConversation":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399596471"
                    }
                }
            },
            "Search":{
                "timings":{
                    "run":{
                        "start_timestamp":"1630184399923778"
                    }
                }
            }
        },
        "analytics_info":{
            "Translation":{
                "matched_semantic_frames":[
                    {
                        "name":"personal_assistant.scenarios.translate.translation",
                        "slots":[
                            {
                                "accepted_types":[
                                    "string"
                                ],
                                "name":"phrase",
                                "value":"гагач полдер",
                                "type":"string"
                            },
                            {
                                "accepted_types":[
                                    "string"
                                ],
                                "name":"from_language",
                                "value":"чувашски",
                                "type":"string"
                            }
                        ]
                    }
                ],
                "version":"vins/stable-159-4@8565965",
                "scenario_analytics_info":{
                    "stage_timings":{
                        "timings":{
                            "run":{
                                "start_timestamp":"1630184399599427"
                            }
                        }
                    },
                    "product_scenario_name":"translate",
                    "intent":"translate"
                },
                "frame_actions":{
                    "1":{
                        "directives":{
                            "list":[
                                {
                                    "type_text_directive":{
                                        "name":"render_buttons_type",
                                        "text":"Повтори"
                                    }
                                }
                            ]
                        },
                        "nlu_hint":{
                            "frame_name":"1"
                        }
                    },
                    "3":{
                        "directives":{
                            "list":[
                                {
                                    "type_text_directive":{
                                        "name":"render_buttons_type",
                                        "text":"А на хинди?"
                                    }
                                }
                            ]
                        },
                        "nlu_hint":{
                            "frame_name":"3"
                        }
                    },
                    "ellipsis":{
                        "callback":{
                            "name":"translation_ellipsis",
                            "payload":{
                                "phrase":"гагач полдер",
                                "from_language":"чувашский"
                            }
                        },
                        "nlu_hint":{
                            "frame_name":"personal_assistant.scenarios.translate.translation__ellipsis"
                        }
                    },
                    "quicker":{
                        "frame":{
                            "name":"personal_assistant.scenarios.translate.translation",
                            "slots":[
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"phrase",
                                    "value":"гагач полдер",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"from_language",
                                    "value":"чувашский",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"to_language",
                                    "value":"русский",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "num"
                                    ],
                                    "name":"speed",
                                    "value":"0.9",
                                    "type":"num"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"settings",
                                    "value":"quicker",
                                    "type":"string"
                                }
                            ]
                        },
                        "nlu_hint":{
                            "frame_name":"personal_assistant.scenarios.translate.translation__quicker"
                        }
                    },
                    "slower":{
                        "frame":{
                            "name":"personal_assistant.scenarios.translate.translation",
                            "slots":[
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"phrase",
                                    "value":"гагач полдер",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"from_language",
                                    "value":"чувашский",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"to_language",
                                    "value":"русский",
                                    "type":"string"
                                },
                                {
                                    "accepted_types":[
                                        "num"
                                    ],
                                    "name":"speed",
                                    "value":"0.9",
                                    "type":"num"
                                },
                                {
                                    "accepted_types":[
                                        "string"
                                    ],
                                    "name":"settings",
                                    "value":"slower",
                                    "type":"string"
                                }
                            ]
                        },
                        "nlu_hint":{
                            "frame_name":"personal_assistant.scenarios.translate.translation__slower"
                        }
                    },
                    "2":{
                        "directives":{
                            "list":[
                                {
                                    "type_text_directive":{
                                        "name":"render_buttons_type",
                                        "text":"А на хорватском?"
                                    }
                                }
                            ]
                        },
                        "nlu_hint":{
                            "frame_name":"2"
                        }
                    },
                    "repeat":{
                        "callback":{
                            "name":"repeat",
                            "payload":{
                                "voice":"полдер гагач"
                            }
                        },
                        "nlu_hint":{
                            "frame_name":"repeat"
                        }
                    },
                    "4":{
                        "directives":{
                            "list":[
                                {
                                    "type_text_directive":{
                                        "name":"render_buttons_type",
                                        "text":"А на арабском?"
                                    }
                                }
                            ]
                        },
                        "nlu_hint":{
                            "frame_name":"4"
                        }
                    }
                },
                "semantic_frame":{
                    "name":"translate",
                    "slots":[
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"чувашски"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"input_lang_src",
                            "value":"чувашски",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"русский"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"lang_dst",
                            "value":"русский",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"чувашский"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"lang_src",
                            "value":"чувашский",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"полдер гагач"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"result",
                            "value":"полдер гагач",
                            "type":"string"
                        },
                        {
                            "accepted_types":[
                                "num"
                            ],
                            "name":"speed",
                            "type":"num"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"хорватский хинди арабский"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"suggest_langs",
                            "value":"хорватский хинди арабский",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"гагач полдер"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"text",
                            "value":"гагач полдер",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"гагач полдер"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"text_to_translate",
                            "value":"гагач полдер",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"гагач полдер"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"text_to_translate_voice",
                            "value":"гагач полдер",
                            "type":"string"
                        },
                        {
                            "accepted_types":[
                                "string"
                            ],
                            "name":"translate_service",
                            "type":"string"
                        },
                        {
                            "typed_value":{
                                "type":"string",
                                "string":"полдер гагач"
                            },
                            "accepted_types":[
                                "string"
                            ],
                            "name":"voice",
                            "value":"полдер гагач",
                            "type":"string"
                        }
                    ]
                }
            }
        },
        "winner_scenario":{
            "name":"Translation"
        },
        "user_profile":{
            "subscriptions":[
                "basic-kinopoisk",
                "basic-music",
                "basic-plus",
                "station-lease-plus"
            ],
            "has_yandex_plus":true
        },
        "iot_user_info":{
            "households":[
                {
                    "name":"ДОМ",
                    "latitude":56.126339,
                    "longitude":47.390522,
                    "address":"Россия, Чувашская Республика, Чебоксары, микрорайон Новый Город, Новогородская улица, 38",
                    "id":"4696a330-2212-4829-8975-a4bf7be03cd3"
                },
                {
                    "name":"Мой дом",
                    "id":"882102ad-e30f-40c1-9014-fc867f01f3f7"
                }
            ],
            "devices":[
                {
                    "name":"Яндекс Мини",
                    "updated":1628455445,
                    "quasar_info":{
                        "device_id":"FF98F029669E41603B6B4993",
                        "platform":"yandexmini"
                    },
                    "analytics_type":"devices.types.smart_speaker.yandex.station.mini",
                    "status_updated":1629453046,
                    "analytics_name":"Умное устройство",
                    "id":"40e00ad5-98ec-4911-8226-c7e4b8651a1f",
                    "custom_data":"eyJkZXZpY2VfaWQiOiJGRjk4RjAyOTY2OUU0MTYwM0I2QjQ5OTMiLCJwbGF0Zm9ybSI6InlhbmRleG1pbmkifQ==",
                    "household_id":"4696a330-2212-4829-8975-a4bf7be03cd3",
                    "device_info":{
                        "manufacturer":"Yandex Services AG",
                        "model":"YNDX-0004"
                    },
                    "icon_url":"http://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.smart_speaker.yandex.station.mini.png",
                    "status":"OnlineDeviceState",
                    "type":"YandexStationMiniDeviceType",
                    "external_name":"Яндекс Мини",
                    "external_id":"FF98F029669E41603B6B4993.yandexmini",
                    "created":1628432547,
                    "original_type":"YandexStationMiniDeviceType",
                    "skill_id":"Q"
                }
            ],
            "current_household_id":"4696a330-2212-4829-8975-a4bf7be03cd3"
        },
        "post_classify_duration":"7137",
        "original_utterance":"что такое гагач полдер по чувашски",
        "shown_utterance":"Что такое гагач полдер по-чувашски?",
        "chosen_utterance":"что такое гагач полдер по чувашски"
    }
}
)*";

} // namespace

Y_UNIT_TEST_SUITE(SpeechkitUtils) {
    Y_UNIT_TEST(CreateVinsLikeRequestTextEvent) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_TEXT_EVENT, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_TEXT_EVENT, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeRequestSuggestEvent) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_SUGGESTED_EVENT, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_SUGGESTED_EVENT, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeRequestVoiceEvent1) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_VOICE_EVENT1, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_VOICE_EVENT1, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeRequestServerEvent) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_SERVER_EVENT, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_SERVER_EVENT, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeRequestMusicEvent) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_MUSIC_EVENT, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_MUSIC_EVENT, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeRequestImageEvent) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_IMAGE_EVENT, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_IMAGE_EVENT, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeRequestVoiceEvent2) {
        NAlice::TSpeechKitRequestProto skRequestProto;
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST_VOICE_EVENT2, &skRequestProto));

        NJson::TJsonValue actualVinsLikeRequestJson, expectedVinsLikeRequestJson;
        NJson::ReadJsonTree(JSON_VINS_LIKE_REQUEST_VOICE_EVENT2, &expectedVinsLikeRequestJson);
        TVinsLikeRequest vinsLikeRequestStruct(skRequestProto);
        actualVinsLikeRequestJson = vinsLikeRequestStruct.DumpJson();

        UNIT_ASSERT_VALUES_EQUAL(expectedVinsLikeRequestJson, actualVinsLikeRequestJson);
    }

    Y_UNIT_TEST(CreateVinsLikeResponse1) {
        NJson::TJsonValue responseJson, expectedVinsLikeResponseJson, actualVinsLikeResponseJson;
        NJson::ReadJsonTree(JSON_RESPONSE1, &responseJson);
        NJson::ReadJsonTree(JSON_VINS_LIKE_RESPONSE1, &expectedVinsLikeResponseJson);
        actualVinsLikeResponseJson = TVinsLikeResponse(responseJson).DumpJson();
        UNIT_ASSERT_EQUAL(expectedVinsLikeResponseJson, actualVinsLikeResponseJson);
    }

    Y_UNIT_TEST(CreateVinsLikeResponse2) {
        NJson::TJsonValue responseJson, expectedVinsLikeResponseJson, actualVinsLikeResponseJson;
        NJson::ReadJsonTree(JSON_RESPONSE2, &responseJson);
        NJson::ReadJsonTree(JSON_VINS_LIKE_RESPONSE2, &expectedVinsLikeResponseJson);
        actualVinsLikeResponseJson = TVinsLikeResponse(responseJson).DumpJson();
        UNIT_ASSERT_EQUAL(expectedVinsLikeResponseJson, actualVinsLikeResponseJson);
    }

    Y_UNIT_TEST(CreateVinsLikeEmptyResponse1) {
        NJson::TJsonValue responseEmptyJson;
        NJson::ReadJsonTree(JSON_EMPTY_RESPONSE1, &responseEmptyJson);
        UNIT_ASSERT_EQUAL(NJson::JSON_NULL, TVinsLikeResponse(responseEmptyJson).DumpJson());
    }

    Y_UNIT_TEST(CreateVinsLikeEmptyResponse2) {
        NJson::TJsonValue responseEmptyJson;
        NJson::ReadJsonTree(JSON_EMPTY_RESPONSE2, &responseEmptyJson);
        UNIT_ASSERT_EQUAL(NJson::JSON_NULL, TVinsLikeResponse(responseEmptyJson).DumpJson());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessage) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        auto actual = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();
        actual.GetMapSafe().erase("server_time");
        actual.GetMapSafe().erase("server_time_ms");

        NJson::TJsonValue expected;
        NJson::ReadJsonTree(JSON_VINS_LIKE_LOG, &expected);

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);

        UNIT_ASSERT_EQUAL("UTTERANCE", actual["type"].GetString());
        UNIT_ASSERT_EQUAL("pa", actual["app_id"].GetString());
        UNIT_ASSERT_EQUAL("megamind", actual["provider"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageClientTime) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitRequest.MutableApplication()->SetEpoch("1337");
        speechkitRequest.MutableApplication()->SetClientTime("69");
        speechkitRequest.MutableApplication()->SetTimezone("flex");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL(1337, vinsLikeLog["client_time"].GetUInteger());
        UNIT_ASSERT_EQUAL("flex", vinsLikeLog["client_tz"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageResponseId) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitResponse.MutableHeader()->SetResponseId("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("lolkek", vinsLikeLog["response_id"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageUtteranceVoice) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        {
            speechkitRequest.MutableRequest()->MutableEvent()->MutableAsrResult()->Clear();
            auto& asrResult = *speechkitRequest.MutableRequest()->MutableEvent()->MutableAsrResult()->Add();
            asrResult.MutableWords()->Add()->SetValue("lol");
            asrResult.MutableWords()->Add()->SetValue("kek");
        }
        speechkitRequest.MutableRequest()->MutableEvent()->SetType(NAlice::EEventType::voice_input);

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("voice", vinsLikeLog["utterance_source"].GetString());
        UNIT_ASSERT_VALUES_EQUAL("lol kek", vinsLikeLog["utterance_text"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageUtteranceText) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitRequest.MutableRequest()->MutableEvent()->SetType(NAlice::EEventType::text_input);
        speechkitRequest.MutableRequest()->MutableEvent()->SetText("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("text", vinsLikeLog["utterance_source"].GetString());
        UNIT_ASSERT_VALUES_EQUAL("lolkek", vinsLikeLog["utterance_text"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageUtteranceSuggested) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitRequest.MutableRequest()->MutableEvent()->SetType(NAlice::EEventType::suggested_input);
        speechkitRequest.MutableRequest()->MutableEvent()->SetText("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("suggested", vinsLikeLog["utterance_source"].GetString());
        UNIT_ASSERT_VALUES_EQUAL("lolkek", vinsLikeLog["utterance_text"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageUtteranceImage) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitRequest.MutableRequest()->MutableEvent()->SetType(NAlice::EEventType::image_input);

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("image", vinsLikeLog["utterance_source"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageUtteranceMusic) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitRequest.MutableRequest()->MutableEvent()->SetType(NAlice::EEventType::music_input);

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("music", vinsLikeLog["utterance_source"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageUuid) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        speechkitRequest.MutableApplication()->SetUuid("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("lolkek", vinsLikeLog["uuid"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageLocation) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        speechkitRequest.MutableRequest()->MutableLocation()->SetLon(1337);
        speechkitRequest.MutableRequest()->MutableLocation()->SetLat(69);

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        const double EPS = 1e-7;
        UNIT_ASSERT_DOUBLES_EQUAL(69, vinsLikeLog["location_lat"].GetDouble(), EPS);
        UNIT_ASSERT_DOUBLES_EQUAL(1337, vinsLikeLog["location_lon"].GetDouble(), EPS);
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageLang) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        speechkitRequest.MutableApplication()->SetLang("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("lolkek", vinsLikeLog["lang"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageDeviceId) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        speechkitRequest.MutableApplication()->SetDeviceId("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("lolkek", vinsLikeLog["device_id"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageExperiments) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        {
            auto& storage = *speechkitRequest.MutableRequest()->MutableExperiments()->MutableStorage();
            storage.clear();
            {
                NAlice::TExperimentsProto::TValue value;
                value.SetString("kek");
                storage["lol"] = value;
            }
            {
                NAlice::TExperimentsProto::TValue value;
                value.SetInteger(69);
                storage["1337"] = value;
            }
        }

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        const auto expectedExps = NJson::ReadJsonFastTree(R"({"lol": "kek", "1337": 69})");
        UNIT_ASSERT_EQUAL(expectedExps, vinsLikeLog["experiments"]);
        UNIT_ASSERT(!vinsLikeLog.Has("session"));
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageHasSessionExperiments) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        {
            auto& storage = *speechkitRequest.MutableRequest()->MutableExperiments()->MutableStorage();
            storage.clear();
            {
                NAlice::TExperimentsProto::TValue value;
                value.SetString("kek");
                storage["lol"] = value;
            }
            {
                NAlice::TExperimentsProto::TValue value;
                value.SetInteger(69);
                storage["1337"] = value;
            }
            {
                NAlice::TExperimentsProto::TValue value;
                value.SetBoolean(true);
                storage["dump_sessions_to_logs"] = value;
            }
        }
        speechkitRequest.SetSession("lolkek");

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        const auto expectedExps =
            NJson::ReadJsonFastTree(R"({"lol": "kek", "1337": 69, "dump_sessions_to_logs": true})");
        UNIT_ASSERT_EQUAL(expectedExps, vinsLikeLog["experiments"]);
        UNIT_ASSERT_EQUAL("lolkek", vinsLikeLog["session"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageType) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        NAlice::TEvent event;
        event.SetType(NAlice::EEventType::server_action);
        (*event.MutablePayload()->mutable_fields())["lol"].set_string_value("kek");
        event.SetName("keklol");

        *speechkitRequest.MutableRequest()->MutableEvent() = event;

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("CALLBACK", vinsLikeLog["type"].GetString());
        UNIT_ASSERT_EQUAL("keklol", vinsLikeLog["callback_name"].GetString());

        const auto expectedArgs = NJson::ReadJsonFastTree(R"({"lol": "kek"})");

        UNIT_ASSERT_EQUAL(expectedArgs, vinsLikeLog["callback_args"]);
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageBiometry) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));
        {
            NAlice::TBiometryScoring biometryScoring;
            biometryScoring.SetRequestId("lolkek");
            *speechkitRequest.MutableRequest()->MutableEvent()->MutableBiometryScoring() = biometryScoring;
        }

        {
            NAlice::TBiometryClassification biometryClassification;
            biometryClassification.SetStatus("keklol");
            *speechkitRequest.MutableRequest()->MutableEvent()->MutableBiometryClassification() =
                biometryClassification;
        }

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT_EQUAL("lolkek", vinsLikeLog["biometry_scoring"]["request_id"].GetString());
        UNIT_ASSERT_EQUAL("keklol", vinsLikeLog["biometry_classification"]["status"].GetString());
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageDoesntContainSensitiveData) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT(!vinsLikeLog.Has("contains_sensitive_data"));
    }

    Y_UNIT_TEST(CreateVinsLikeLogMessageContainsSensitiveData) {
        NAlice::TSpeechKitRequestProto speechkitRequest;
        NAlice::TSpeechKitResponseProto speechkitResponse;

        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_REQUEST, &speechkitRequest));
        UNIT_ASSERT(TextFormat::ParseFromString(PROTO_SPEECHKIT_RESPONSE, &speechkitResponse));

        speechkitResponse.SetContainsSensitiveData(true);

        const auto vinsLikeLog = TVinsLikeLog(speechkitRequest, speechkitResponse).DumpJson();

        UNIT_ASSERT(vinsLikeLog.Has("contains_sensitive_data"));
        UNIT_ASSERT(vinsLikeLog["contains_sensitive_data"].GetBoolean());
    }
}
