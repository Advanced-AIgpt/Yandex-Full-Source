#include <alice/wonderlogs/library/parsers/uniproxy.h>

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice::NWonderlogs;

const TString PROTO_MEGAMIND_TIMINGS = R"(
ResultVinsRunResponseIsReadySec: 0.190095739
VinsRunDelayAfterEouDurationSec: 0
UsefulVinsRequestDurationSec: 0.06756541785
MeanVinsPreparingRequestDurationSec: 0.0007872690912
MeanVinsRequestDurationSec: 0.06756541785
VinsRequestCount: 1
HasApplyVinsRequest: true
LastVinsPreparingRequestDurationSec: 0.0007872690912
LastVinsRunRequestIntentName: ""
LastVinsRunRequestDurationSec: 0.06756541785
UsefulVinsRequestEvage: 0.1246239601
LastVinsFullRequestDurationSec: 0.1023750887
VinsPersonalDataStartEvage: 0.00218536309
UsefulVinsPrepareRequestPersonalData: 0
Epoch: 1630184400
VinsResponseSec: 0.2284180522
VinsPersonalDataEndEvage: 0.1212845731
VinsSessionLoadEndEvage: 0.121431171
HasVinsFullResultOnEou: false
UsefulVinsPrepareRequestSession: 0.0004335390404
LastVinsApplyRequestDurationSec: 0.03480967088
VinsWaitAfterEouDurationSec: 0.227667816
StartVinsApplyRequestSec: 0.19286156
FinishVinsRequestEou: 0.190091273
StartVinsRequestEou: 0.122535401
UsefulVinsPrepareRequestContacts: 0.0006765970029
UsefulVinsPrepareRequestMemento: 0.0007807579823
UsefulVinsPrepareRequestNotificationState: 0.0005707200617)";

constexpr TStringBuf JSON_MEGAMIND_TIMINGS = R"(
{
    "Directive":{
        "header":{
            "name":"UniproxyVinsTimings",
            "namespace":"System",
            "refMessageId":"18f95cf7-12c3-495c-ac46-6e4ab619a8ac"
        },
        "payload":{
            "epoch":1630184400,
            "finish_vins_request_eou":0.19009127304889262,
            "has_apply_vins_request":true,
            "has_vins_full_result_on_eou":false,
            "last_vins_apply_request_duration_sec":0.03480967087671161,
            "last_vins_full_request_duration_sec":0.10237508872523904,
            "last_vins_preparing_request_duration_sec":0.0007872690912336111,
            "last_vins_run_request_duration_sec":0.06756541784852743,
            "last_vins_run_request_intent_name":"",
            "mean_vins_preparing_request_duration_sec":0.0007872690912336111,
            "mean_vins_request_duration_sec":0.06756541784852743,
            "result_vins_run_response_is_ready_sec":0.19009573897346854,
            "start_vins_apply_request_sec":0.1928615600336343,
            "start_vins_request_eou":0.12253540102392435,
            "useful_vins_prepare_request_contacts":0.0006765970028936863,
            "useful_vins_prepare_request_memento":0.0007807579822838306,
            "useful_vins_prepare_request_notification_state":0.0005707200616598129,
            "useful_vins_prepare_request_personal_data":0,
            "useful_vins_prepare_request_session":0.0004335390403866768,
            "useful_vins_request_duration_sec":0.06756541784852743,
            "useful_vins_request_evage":0.12462396011687815,
            "vins_personal_data_end_evage":0.1212845731060952,
            "vins_personal_data_start_evage":0.0021853630896657705,
            "vins_request_count":1,
            "vins_response_sec":0.22841805219650269,
            "vins_run_delay_after_eou_duration_sec":0,
            "vins_session_load_end_evage":0.12143117096275091,
            "vins_wait_after_eou_duration_sec":0.22766781598329544
        }
    },
    "Session":{
        "Action":"response",
        "AppId":"aliced",
        "AppType":"aliced",
        "DoNotUseUserLogs":true,
        "IpAddr":"5.165.136.164",
        "SessionId":"7699ce1f-d78f-4bbd-8135-1ede8fdee2ab",
        "Timestamp":"2021-08-28T21:00:00.715900",
        "Uid":"",
        "Uuid":"641983abdb7a0c60b7a15397debb652a"
    }
})";

constexpr TStringBuf JSON_VOICE_INPUT = R"(
{
    "Session":{
        "AppId":"ru.yandex.searchplugin",
        "Timestamp":"2021-08-04T21:48:59.828845Z",
        "Uid":"",
        "AppType":"pp",
        "IpAddr":"185.41.184.34",
        "SessionId":"8f46be26-63f4-46b6-8443-d828905e806d",
        "Action":"request",
        "RetryNumber":0,
        "DoNotUseUserLogs":false,
        "Uuid":"facc3167774b4120b0d967dfe963d6a2"
    },
    "Event":{
        "event":{
            "payload":{
                "advancedASROptions":{
                    "partial_results":true,
                    "manual_punctuation":false,
                    "capitalize":true
                },
                "lang":"ru-RU",
                "firmware":"exynos990",
                "tags":"PASS_AUDIO;exp_330999,0,9;exp_384138,0,64;exp_387160,0,13;exp_390353,0,76;exp_395779,0,82;exp_356539,0,70;exp_357768,0,78;exp_357771,0,80;exp_378360,0,58;exp_381880,0,64;exp_385537,0,73;exp_389759,0,42;exp_397583,0,64;exp_398226,0,89",
                "oauth_token":"1.322618115.163361.1657214252.1625678252242.38503.fE5x9qUYRXRKHMVN.2F1DxiG3U7ls7s_0_0WzpEq1eGU-3d7***************************************************************************************************",
                "punctuation":true,
                "application":{
                    "timezone":"Europe/Moscow",
                    "timestamp":"1628113737",
                    "device_id":"8df159253705ccc704ec182c5143b2b3",
                    "lang":"ru-RU",
                    "client_time":"20210805T004857"
                },
                "header":{
                    "prev_req_id":"c9bda9e7-5eca-4cc6-83e3-dca4a14f7412",
                    "request_id":"d544f7ab-5e22-4901-bdc6-f57086512caa",
                    "sequence_number":138
                },
                "request":{
                    "additional_options":{
                        "oauth_token":"1.322618115.163361.1657214252.1625678252242.38503.fE5x9qUYRXRKHMVN.2F1DxiG3U7ls7s_0_0WzpEq1eGU-3d7***************************************************************************************************",
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
                        "supported_features":[
                            "whocalls",
                            "div_cards",
                            "open_link_search_viewport",
                            "open_link_yellowskin",
                            "image_recognizer",
                            "music_recognizer",
                            "quasar_screen",
                            "reader_app",
                            "open_link_intent",
                            "div2_cards",
                            "keyboard",
                            "server_action",
                            "open_dialogs_in_tabs",
                            "set_alarm",
                            "set_timer",
                            "cloud_ui",
                            "open_link",
                            "cloud_push_implementation",
                            "open_link_turbo_app",
                            "open_yandex_auth",
                            "music_sdk_client"
                        ],
                        "bass_options":{
                            "filtration_level":1,
                            "screen_scale_factor":2.625,
                            "user_agent":"Mozilla/5.0 (Linux; arm_64; Android 11; SM-G988B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.164 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.61.1",
                            "cookies":"..."
                        },
                        "unsupported_features":[
                            "covid_qr"
                        ],
                        "divkit_version":"2.3"
                    },
                    "location":{
                        "lat":55.9504109,
                        "lon":37.9747743,
                        "recency":517242,
                        "accuracy":24.89999962
                    },
                    "experiments":[
                        "image_recognizer",
                        "music_recognizer",
                        "multi_tabs"
                    ],
                    "voice_session":true,
                    "activation_type":"superapp_bottom_bar",
                    "event":{
                        "name":"",
                        "type":"voice_input"
                    },
                    "reset_session":true,
                    "device_state":{
                        "sound_level":1,
                        "sound_muted":false,
                        "is_default_assistant":false
                    }
                },
                "recognize_music_only":false,
                "format":"audio/opus",
                "topic":"dialog-general"
            },
            "header":{
                "name":"VoiceInput",
                "messageId":"cfb2e597-e9c1-4c2f-83d4-87942e28357b",
                "rtLogToken":"1628113739828699$cfb2e597-e9c1-4c2f-83d4-87942e28357b$91e67220-73ae4bc6-30cebd41-8d13dcf4",
                "streamId":1,
                "namespace":"Vins"
            }
        }
    }
})";

const TString PROTO_VOICE_INPUT = R"(
ActivationType: "superapp_bottom_bar"
Topic: "dialog-general"
SpeechKitRequest {
    Header {
        RequestId: "d544f7ab-5e22-4901-bdc6-f57086512caa"
        PrevReqId: "c9bda9e7-5eca-4cc6-83e3-dca4a14f7412"
        SequenceNumber: 138
    }
    Application {
        DeviceId: "8df159253705ccc704ec182c5143b2b3"
        Lang: "ru-RU"
        ClientTime: "20210805T004857"
        Timezone: "Europe/Moscow"
        Epoch: "1628113737"
    }
    Request {
        Event {
            Type: voice_input
            Name: ""
        }
        Location {
            Lat: 55.9504109
            Lon: 37.9747743
            Accuracy: 24.89999962
            Recency: 517242
        }
        Experiments {
            Storage {
                key: "image_recognizer"
                value {
                    String: "1"
                }
            }
            Storage {
                key: "multi_tabs"
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
        }
        DeviceState {
            SoundLevel: 1
            SoundMuted: false
            IsDefaultAssistant: false
        }
        AdditionalOptions {
            OAuthToken: "1.322618115.163361.1657214252.1625678252242.38503.fE5x9qUYRXRKHMVN.2F1DxiG3U7ls7s_0_0WzpEq1eGU-3d7***************************************************************************************************"
            BassOptions {
                UserAgent: "Mozilla/5.0 (Linux; arm_64; Android 11; SM-G988B) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.164 BroPP/1.0 SA/3 Mobile Safari/537.36 YandexSearch/21.61.1"
                FiltrationLevel: 1
                Cookies: "..."
                ScreenScaleFactor: 2.625
            }
            SupportedFeatures: "whocalls"
            SupportedFeatures: "div_cards"
            SupportedFeatures: "open_link_search_viewport"
            SupportedFeatures: "open_link_yellowskin"
            SupportedFeatures: "image_recognizer"
            SupportedFeatures: "music_recognizer"
            SupportedFeatures: "quasar_screen"
            SupportedFeatures: "reader_app"
            SupportedFeatures: "open_link_intent"
            SupportedFeatures: "div2_cards"
            SupportedFeatures: "keyboard"
            SupportedFeatures: "server_action"
            SupportedFeatures: "open_dialogs_in_tabs"
            SupportedFeatures: "set_alarm"
            SupportedFeatures: "set_timer"
            SupportedFeatures: "cloud_ui"
            SupportedFeatures: "open_link"
            SupportedFeatures: "cloud_push_implementation"
            SupportedFeatures: "open_link_turbo_app"
            SupportedFeatures: "open_yandex_auth"
            SupportedFeatures: "music_sdk_client"
            UnsupportedFeatures: "covid_qr"
            DivKitVersion: "2.3"
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
        }
        VoiceSession: true
        ResetSession: true
        ActivationType: "superapp_bottom_bar"
    }
})";

constexpr TStringBuf JSON_TTS_TIMINGS = R"(
{
    "Directive":{
        "header":{
            "name":"UniproxyTTSTimings",
            "namespace":"System",
            "refMessageId":"bbef8771-d031-4320-b000-0f42a2772b16"
        },
        "payload":{
            "first_tts_chunk_sec":0.3670448012948036,
            "useful_response_for_user_evage":0.25740448012948036
        }
    }
})";

const TString PROTO_TTS_TIMINGS = R"(
FirstTtsChunkSec: 0.3670448013
UsefulResponseForUserEvage: 0.2574044801)";

constexpr TStringBuf JSON_TTS_GENERATE = R"(
{
    "Session":{
        "AppId":"ru.yandex.yandexmaps",
        "Timestamp":"2021-07-11T10:05:53.961410Z",
        "Uid":"",
        "AppType":"other_apps",
        "IpAddr":"2a00:1fa3:420b:4a55:1:1:6c3b:fb75",
        "SessionId":"ad0a4737-dd46-427a-9b4c-a20f8b98df66",
        "Action":"request",
        "RetryNumber":0,
        "DoNotUseUserLogs":false,
        "Uuid":"34929ca7be114d23a0b0aa70c612d619"
    },
    "Event":{
        "event":{
            "payload":{
                "voice":"shitova.us",
                "lang":"ru-RU",
                "text":"поверните налево",
                "format":"audio/opus",
                "emotion":"neutral",
                "quality":"UltraHigh"
            },
            "header":{
                "name":"Generate",
                "messageId":"161fba92-433d-48d2-904e-7455391c88f4",
                "rtLogToken":"1625997953961344$161fba92-433d-48d2-904e-7455391c88f4$bb942a34-5130308d-5f04b55c-b166e59e",
                "namespace":"TTS"
            }
        }
    }
})";

constexpr TStringBuf JSON_LOG_SPOTTER = R"(
{
    "Session":{
        "AppId":"aliced",
        "Timestamp":"2021-11-02T14:00:48.319305Z",
        "Uid":"",
        "AppType":"aliced",
        "IpAddr":"178.214.254.72",
        "SessionId":"64178dae-433c-4ad3-8abc-f32adcc50968",
        "Action":"request",
        "RetryNumber":0,
        "DoNotUseUserLogs":false,
        "Uuid":"555f7038a97c06cdb967a445f232abf1"
    },
    "Event":{
        "event":{
            "payload":{
                "vinsMessageId":"177b4c3f-d296-4756-af9f-627c083a883b",
                "lang":"ru-RU",
                "firmware":"1.86.4.38.1095637818.20211008",
                "source":"ru-RU-yandexmini-alisa-28Jul21-x3-retune-4",
                "oauth_token":"",
                "extra":{
                    "metainfo":"{\"frameno\": 25989861, \"decoder_state\": \"hit\", \"freq_filter_state\": \"passed\", \"freq_filter_confidence\": 11.164299, \"tts_blocker_info\": {\"frame_blocked\": 0, \"hit_blocked\": 0, \"ruleno\": -1, \"is_alive\": 1, \"shift_bytes\": 8216869376}, \"api_calls\": \"\", \"regular_logs\": {\"version\": \"ru-RU-yandexmini-alisa-28Jul21-x3-retune-4\", \"revision\": 8716277, \"received_frames_from_reset\": 12126, \"received_frames_from_start\": 25989866, \"hits\": [1], \"blocked_hits_by_freq_filter\": 0, \"blocked_hits_by_tts_blocker\": 0, \"blocked_frames_by_tts_blocker\": 0, \"tts_blocker_is_dead_hits\": 0, \"tts_blocker_is_dead_frames\": 0, \"subhits\": [0], \"name\": \"default\", \"spotter_conf_id\": 7024832355968179883, \"spotter_id\": 7024832355968242689, \"skip_by_vad_frames\": 0, \"confidences_histogram\": [[238, 0, 0, 0, 0, 0, 0, 0, 0, 0]], \"logits_histogram\": [[3, 0, 0, 1, 0, 0, 0, 0, 0, 12122], [12122, 0, 0, 0, 0, 0, 1, 0, 0, 3]], \"external_logs\": {\"chen14\": {\"after_smoothing_probs_total_count\": 24252, \"after_smoothing_probs_negative_count\": 6392, \"after_smoothing_probs_nan_count\": 0, \"after_smoothing_probs_negative_min\": -0.000000, \"after_smoothing_probs_max_drift_from_one\": 0.008813}}}, \"confidences\": [0.657701]}",
                    "subThresholdSendRateMs":3600000,
                    "subThresholdDelayMs":90000,
                    "context":"activation",
                    "device_state":{
                        "device_id":"FF98F0290E1724935CC54205",
                        "timers":{
                            "active_timers":[
                                
                            ]
                        },
                        "dnd_enabled":false,
                        "sound_level":5,
                        "is_tv_plugged_in":false,
                        "clocks_us":{
                            "ChNCwRzj":260189034672
                        },
                        "smart_activation":null,
                        "alarm_state":{
                            "sound_alarm_setting":{
                                "type":"music",
                                "info":{
                                    "uri":"https://music.yandex.ru/users/477737898/playlists/1000/?from=alice&mob=0",
                                    "coverUri":"https://music.yandex.ru/blocks/common/default.200x200.png",
                                    "type":"playlist",
                                    "title":"громкая музыка",
                                    "id":"477737898:1000",
                                    "first_track_uri":"http://music.yandex.ru/track/33311009",
                                    "session_id":"60i3F2Ix",
                                    "first_track":{
                                        "uri":"https://music.yandex.ru/album/5568718/track/33311009/?from=alice&mob=0",
                                        "album":{
                                            "genre":"rock",
                                            "title":"Evolve",
                                            "id":"5568718"
                                        },
                                        "coverUri":"https://avatars.yandex.net/get-music-content/98892/a6be0789.a.5568718-1/200x200",
                                        "title":"Believer",
                                        "type":"track",
                                        "id":"33311009",
                                        "artists":[
                                            {
                                                "name":"Imagine Dragons",
                                                "is_various":false,
                                                "composer":false,
                                                "id":"675068"
                                            }
                                        ],
                                        "subtype":"music"
                                    },
                                    "subtype":null
                                }
                            },
                            "icalendar":"BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20211016T040000Z\r\nDTEND:20211016T040000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20211016T040000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n",
                            "max_sound_level":7,
                            "currently_playing":false,
                            "can_be_postponed":false
                        },
                        "tof":true,
                        "sound_level_step":1000,
                        "alarms_state":"BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Yandex LTD//NONSGML Quasar//EN\r\nBEGIN:VEVENT\r\nDTSTART:20211016T040000Z\r\nDTEND:20211016T040000Z\r\nBEGIN:VALARM\r\nTRIGGER;VALUE=DATE-TIME:20211016T040000Z\r\nACTION:AUDIO\r\nEND:VALARM\r\nEND:VEVENT\r\nEND:VCALENDAR\r\n",
                        "mics_muted":false,
                        "sound_muted":false,
                        "sound_max_level":10,
                        "active_actions":{
                            "semantic_frames":null
                        },
                        "bluetooth":{
                            
                        },
                        "audio_player":{
                            "offset_ms":5000,
                            "played_ms":5000,
                            "duration_ms":106000,
                            "last_stop_timestamp":1635695288355,
                            "scenario_meta":{
                                "owner":"music",
                                "what_is_playing_answer":"Младшая группа хора ИХВ АПН СССР, песня \"В лесу родилась ёлочка\"",
                                "@scenario_name":"HollywoodMusic"
                            },
                            "last_play_timestamp":1635695281787,
                            "player_state":"Stopped",
                            "current_stream":{
                                "stream_type":"Track",
                                "subtitle":"Младшая группа хора ИХВ АПН СССР",
                                "stream_id":"6098515",
                                "title":"В лесу родилась ёлочка",
                                "last_play_timestamp":1635695281787
                            }
                        },
                        "device_config":{
                            "content_settings":"children",
                            "child_content_settings":"safe",
                            "spotter":"alisa"
                        },
                        "sound_min_level":0
                    },
                    "testids":"443673",
                    "experiments":{
                        "mordovia_long_listening":"1",
                        "video_disable_webview_searchscreen":"1",
                        "ether":"https://yandex.ru/portal/station/main",
                        "mordovia_support_channels":"1",
                        "recurring_purchase":"1",
                        "mordovia":"1",
                        "video_disable_films_webview_searchscreen":"1",
                        "mm_disable_protocol_scenario=MordoviaVideoSelection":"1",
                        "mm_enable_session_reset":"1",
                        "enable_multiroom":"1"
                    },
                    "VQEType":"yandex",
                    "requestSoundBeforeTriggerMs":1500,
                    "globalStreamId":"e9a63c6f-4be7-4553-bff9-2cdaa7f7307f",
                    "phraseId":"1",
                    "onlineValidationInfo":"{\"version\": \"ru-RU-yandexmini-alisa-28Jul21-x3-retune-4\", \"override_ov_threshold\": -0.175000, \"ov_version\": \"0\"}",
                    "quasmodrom_group":"production",
                    "streamType":"vqe_0",
                    "standalone":"false",
                    "requestSoundAfterTriggerMs":500,
                    "durationSubmitted":"3122528",
                    "actualSoundBeforeTriggerMs":1500,
                    "actualSoundAfterTriggerMs":500,
                    "unhandledBytes":"5856",
                    "isSpotterSound":true,
                    "quasmodrom_subgroup":"production",
                    "VQEPreset":"yandexmini_postOneCh",
                    "test_buckets":"443673,0,56;329370,0,85;329376,0,49;329383,0,60;336917,0,44;336932,0,33;336936,0,55;336940,0,76;336945,0,16;336952,0,28;336960,0,36;336965,0,56;336967,0,51;336973,0,85;336979,0,8;336981,0,18;336984,0,31;336989,0,38;336993,0,81;336997,0,14;337004,0,65"
                },
                "transcript":"алиса",
                "format":"audio/opus"
            },
            "header":{
                "name":"Spotter",
                "messageId":"c3321d7b-0498-452c-b0d5-a9374bd93d49",
                "refStreamId":4,
                "rtLogToken":"1635861648319081$c3321d7b-0498-452c-b0d5-a9374bd93d49$1ac3e4fb-ad7cd391-ce57ffe8-e8a1b826",
                "streamId":1,
                "namespace":"Log"
            }
        }
    }
})";

constexpr TStringBuf JSON_TEST_IDS = R"(
{
    "Directive":{
        "Body":{
            "test_ids":[
                "412802",
                "20194",
            ]
        },
        "ForEvent":"55abc467-6187-4759-b423-10d30d482507",
        "type":"FlagsJson"
    },
    "Session":{
        "AppId":"ru.yandex.yandexnavi",
        "AppType":"navi",
        "Action":"log_flags_json",
        "DoNotUseUserLogs":false,
        "IpAddr":"176.59.174.42",
        "RetryNumber":0,
        "SessionId":"77c7c258-e8ca-44db-8e4c-5714df1a8b76",
        "Timestamp":"2021-12-14T22:54:06.777472Z",
        "Uid":"",
        "Uuid":"79a1450fd8f64d1a8d9921149c368ff0"
    }
})";

constexpr TStringBuf JSON_TEST_IDS_INVALID1 = R"(
{
    "Directive":{
        "Body":{
            "test_ids":[
                412802,
                20194,
            ]
        },
        "ForEvent":"55abc467-6187-4759-b423-10d30d482507",
        "type":"FlagsJson"
    },
    "Session":{
        "AppId":"ru.yandex.yandexnavi",
        "AppType":"navi",
        "Action":"log_flags_json",
        "DoNotUseUserLogs":false,
        "IpAddr":"176.59.174.42",
        "RetryNumber":0,
        "SessionId":"77c7c258-e8ca-44db-8e4c-5714df1a8b76",
        "Timestamp":"2021-12-14T22:54:06.777472Z",
        "Uid":"",
        "Uuid":"79a1450fd8f64d1a8d9921149c368ff0"
    }
})";

constexpr TStringBuf JSON_TEST_IDS_INVALID2 = R"(
{
    "Directive":{
        "Body":{
            "test_ids":[
                "KFU WAS",
                "IN THE FINAL",
                "ACM ICPC",
            ]
        },
        "ForEvent":"55abc467-6187-4759-b423-10d30d482507",
        "type":"FlagsJson"
    },
    "Session":{
        "AppId":"ru.yandex.yandexnavi",
        "AppType":"navi",
        "Action":"log_flags_json",
        "DoNotUseUserLogs":false,
        "IpAddr":"176.59.174.42",
        "RetryNumber":0,
        "SessionId":"77c7c258-e8ca-44db-8e4c-5714df1a8b76",
        "Timestamp":"2021-12-14T22:54:06.777472Z",
        "Uid":"",
        "Uuid":"79a1450fd8f64d1a8d9921149c368ff0"
    }
})";

const TString PROTO_SPOTTER_STATS = R"(
Confidences: 0.657701
FreqFilterState: "passed"
FreqFilterConfidence: 11.164299)";

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

constexpr TStringBuf JSON_ASR_DEBUG = R"(
{
    "Directive":{
        "ForEvent":"d3e53d6e-4315-4cdf-ba82-994f2db9eaae",
        "type":"AsrCoreDebug",
        "backend":"spotter",
        "debug":"{\"ClientChunksSamples\":[0,0,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200,0,0,0,0,0,0,0,0,0,3200],\"ClientChunksBytes\":[841,79,96,91,83,83,91,84,88,77,84,79,76,82,83,80,75,84,84,88,79,84,78,90,80,74,82,88,82,91,85,81,81,78,83,97,87,86,95,93,96,91,79,90,87,86,76,82,80,76,75,69,80,85,95,90,96,92,85,90,90,90,80,80,80,84,95,95,96,95,88,84,92,86,85,91,95,88,88,80,82,86,76,80,86,92,104,105,88,94,100,101,92,90,92,89,94,90,112,89,94,841,89,94,88,88,90,85,76,82,81,79,80,79,82,79,80,79,77,91,95,95,93,73,81,98,93,91,93,97,90,84,78,82,94,84,74,78,82,82,76,90,103,99,100,93,85,97,87,88,78,93,86],\"SpotterAudioDuration\":1.999,\"SpotterTimeSpent\":0.017651,\"StreamValidation\":{\"burst_detector\":{\"timestamp\":4200590,\"status\":\"Accepted\",\"wrapper_id\":\"1635410383906888\",\"task_count\":8,\"idx\":1911950,\"confidence\":0.570281148},\"online_validation\":{\"version\":\"0\",\"decoder_threshold\":-0.73,\"type\":\"custom_seq2seq\",\"confidence\":-0.1241455078},\"context\":{\"submitted_asr_front_ms\":1000,\"max_asr_front_ms\":1000,\"spotter_back_ms\":2000},\"meta_info\":{\"embedded_spotter_info\":\"{\\\"version\\\": \\\"ru-RU-yandexmini-alisa-02Apr21-retune3\\\", \\\"override_ov_threshold\\\": null, \\\"ov_version\\\": null}\",\"activation_phrase\":\"алиса\",\"device\":\"Yandex yandexmini\"}}}"
    },
    "Session":{
        "AppId":"aliced",
        "Timestamp":"2021-11-02T10:49:51.872039Z",
        "Uid":"",
        "AppType":"aliced",
        "IpAddr":"37.122.124.56",
        "SessionId":"60530b25-07e8-4dc2-b4d3-c65457fa2518",
        "Action":"response",
        "RetryNumber":0,
        "Uuid":"549890e5b195fb37db65bdd64083e2a5",
        "DoNotUseUserLogs":false
    }
})";

const TString PROTO_ASR_DEBUG = R"(
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
})";

constexpr TStringBuf JSON_REQUEST_STAT = R"(
{
    "Session":{
        "AppId":"aliced",
        "Timestamp":"2021-12-18T16:36:32.957909Z",
        "Uid":"",
        "AppType":"aliced",
        "IpAddr":"94.19.5.54",
        "SessionId":"a0b7d2af-0131-440b-9257-49b828682859",
        "Action":"request",
        "RetryNumber":0,
        "DoNotUseUserLogs":false,
        "Uuid":"5a550e77b3c6f3171f5c31b51edcfca9"
    },
    "Event":{
        "event":{
            "payload":{
                "isSeamlessActivation":true,
                "durations":{
                    "onRecognitionEndTime-onVinsResponseTime":"17",
                    "onRecognitionBeginTime-onFirstMessageMergedTime":"218",
                    "onVinsResponseTime-onFirstSynthesisChunkTime":"94",
                    "onStartVoiceInputTime-onRecognitionBeginTime":"79"
                },
                "oauth_token":"AQAAAAATGwIeAAQXFb7********************",
                "reconnectionCount":0,
                "ack":1639845392,
                "isSpotterActivated":true,
                "timestamps":{
                    "onRecognitionBeginTime":"104",
                    "maxAsrRtf":11.25,
                    "onStartVoiceInputTime":"25",
                    "onFirstNonEmptyPartialTime":"322",
                    "onLastCompletedPartialTime":"1418",
                    "requestDurationTime":"4636",
                    "onFirstSocketActivityTime":"2286",
                    "onFirstSynthesisChunkTime":"2373",
                    "onFirstMessageMergedTime":"322",
                    "onPhraseSpottedTime":"0",
                    "averageAsrRtf":6.213445378,
                    "onSoundPlayerBeginTime":"2425",
                    "onRecognitionEndTime":"2262",
                    "onVinsResponseTime":"2279",
                    "onSoundPlayerEndTime":"4661",
                    "onLastSynthesisChunkTime":"2414",
                    "medianAsrRtf":5.4,
                    "spotterConfirmationTime":"869",
                    "minAsrRtf":2.7,
                    "prevSoundPlayerEndTime":"0",
                    "onStartVoiceInputTime":"0",
                    "onStartVinsRequestTime":"0",
                    "onFirstSocketActivityTime":"208",
                    "onLastSynthesisChunkTime":"4115",
                    "StartPlayer":"-9",
                    "OnPlayerBegin":"4",
                    "OnPlayerEnd":"630"
                },
                "refMessageId":"479c8d54-ed14-42ab-9b79-8823e1efd5b8",
                "audioProcessingMode":"PASS_AUDIO",
                "cancelled":false
            },
            "header":{
                "name":"RequestStat",
                "messageId":"a9d7753e-d256-418b-aaa5-af5285785158",
                "ack":1639845392,
                "refStreamId":300,
                "rtLogToken":"1639845392957789$a9d7753e-d256-418b-aaa5-af5285785158$3d466e69-2259a0b0-380236bd-255ab271",
                "namespace":"Log"
            }
        }
    }
})";

const TString PROTO_PARSED_REQUEST_STAT = R"(
Uuid: "5a550e77b3c6f3171f5c31b51edcfca9"
MessageId: "479c8d54-ed14-42ab-9b79-8823e1efd5b8"
RequestStat {
    Timestamps {
        OnSoundPlayerEndTime: "4661"
        OnVinsResponseTime: "2279"
        OnFirstSynthesisChunkTime: "2373"
        OnRecognitionBeginTime: "104"
        OnRecognitionEndTime: "2262"
        RequestDurationTime: "4636"
        SpotterConfirmationTime: "869"
        OnPhraseSpottedTime: "0",
        PrevSoundPlayerEndTime: "0",
        OnStartVoiceInputTime: "0",
        OnStartVinsRequestTime: "0",
        OnSoundPlayerBeginTime: "2425",
        OnFirstSocketActivityTime: "208",
        OnLastSynthesisChunkTime: "4115",
        StartPlayer: "-9",
        OnPlayerBegin: "4",
        OnPlayerEnd: "630"
    }
}
TimestampLogMs: 1639845392957
)";

const TString PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_ENVIRONMENT = R"(
Uuid: "5a550e77b3c6f3171f5c31b51edcfca9"
MessageId: "479c8d54-ed14-42ab-9b79-8823e1efd5b8"
TimestampLogMs: 1639845392957
Environment {
    QloudProject: "alice"
    QloudApplication: "uniproxy"
}
)";

const TString PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_CLIENT_IP = R"(
Uuid: "5a550e77b3c6f3171f5c31b51edcfca9"
MessageId: "479c8d54-ed14-42ab-9b79-8823e1efd5b8"
TimestampLogMs: 1639845392957
ClientIp: "94.19.5.54"
)";

const TString PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_CONNECT_SESSION_ID = R"(
Uuid: "5a550e77b3c6f3171f5c31b51edcfca9"
MessageId: "479c8d54-ed14-42ab-9b79-8823e1efd5b8"
TimestampLogMs: 1639845392957
ConnectSessionId: "a0b7d2af-0131-440b-9257-49b828682859"
)";

const TString PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS = R"(
Uuid: "5a550e77b3c6f3171f5c31b51edcfca9"
MessageId: "479c8d54-ed14-42ab-9b79-8823e1efd5b8"
TimestampLogMs: 1639845392957
DoNotUseUserLogs: false
)";

const TString PROTO_PARSED_REQUEST_STAT_INVALID_UUID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid uuid from RequestStat event: \n{\n    \"Session\":{\n        \"AppId\":\"aliced\",\n        \"Timestamp\":\"2021-12-18T16:36:32.957909Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"aliced\",\n        \"IpAddr\":\"94.19.5.54\",\n        \"SessionId\":\"a0b7d2af-0131-440b-9257-49b828682859\",\n        \"Action\":\"request\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"5a550e77b3c6f3171f5c31b51edcfca9\"\n    },\n    \"Event\":{\n        \"event\":{\n            \"payload\":{\n                \"isSeamlessActivation\":true,\n                \"durations\":{\n                    \"onRecognitionEndTime-onVinsResponseTime\":\"17\",\n                    \"onRecognitionBeginTime-onFirstMessageMergedTime\":\"218\",\n                    \"onVinsResponseTime-onFirstSynthesisChunkTime\":\"94\",\n                    \"onStartVoiceInputTime-onRecognitionBeginTime\":\"79\"\n                },\n                \"oauth_token\":\"AQAAAAATGwIeAAQXFb7********************\",\n                \"reconnectionCount\":0,\n                \"ack\":1639845392,\n                \"isSpotterActivated\":true,\n                \"timestamps\":{\n                    \"onRecognitionBeginTime\":\"104\",\n                    \"maxAsrRtf\":11.25,\n                    \"onStartVoiceInputTime\":\"25\",\n                    \"onFirstNonEmptyPartialTime\":\"322\",\n                    \"onLastCompletedPartialTime\":\"1418\",\n                    \"requestDurationTime\":\"4636\",\n                    \"onFirstSocketActivityTime\":\"2286\",\n                    \"onFirstSynthesisChunkTime\":\"2373\",\n                    \"onFirstMessageMergedTime\":\"322\",\n                    \"onPhraseSpottedTime\":\"0\",\n                    \"averageAsrRtf\":6.213445378,\n                    \"onSoundPlayerBeginTime\":\"2425\",\n                    \"onRecognitionEndTime\":\"2262\",\n                    \"onVinsResponseTime\":\"2279\",\n                    \"onSoundPlayerEndTime\":\"4661\",\n                    \"onLastSynthesisChunkTime\":\"2414\",\n                    \"medianAsrRtf\":5.4,\n                    \"spotterConfirmationTime\":\"869\",\n                    \"minAsrRtf\":2.7,\n                    \"prevSoundPlayerEndTime\":\"0\",\n                    \"onStartVoiceInputTime\":\"0\",\n                    \"onStartVinsRequestTime\":\"0\",\n                    \"onFirstSocketActivityTime\":\"208\",\n                    \"onLastSynthesisChunkTime\":\"4115\",\n                    \"StartPlayer\":\"-9\",\n                    \"OnPlayerBegin\":\"4\",\n                    \"OnPlayerEnd\":\"630\"\n                },\n                \"refMessageId\":\"479c8d54-ed14-42ab-9b79-8823e1efd5b8\",\n                \"audioProcessingMode\":\"PASS_AUDIO\",\n                \"cancelled\":false\n            },\n            \"header\":{\n                \"name\":\"RequestStat\",\n                \"messageId\":\"a9d7753e-d256-418b-aaa5-af5285785158\",\n                \"ack\":1639845392,\n                \"refStreamId\":300,\n                \"rtLogToken\":\"1639845392957789$a9d7753e-d256-418b-aaa5-af5285785158$3d466e69-2259a0b0-380236bd-255ab271\",\n                \"namespace\":\"Log\"\n            }\n        }\n    }\n} timestamp: 2021-12-18T16:36:32.000000Z"
Uuid: ""
MessageId: "479c8d54-ed14-42ab-9b79-8823e1efd5b8"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=479c8d54-ed14-42ab-9b79-8823e1efd5b8"
)";

const TString PROTO_PARSED_REQUEST_STAT_INVALID_MESSAGE_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid message_id from RequestStat event: \n{\n    \"Session\":{\n        \"AppId\":\"aliced\",\n        \"Timestamp\":\"2021-12-18T16:36:32.957909Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"aliced\",\n        \"IpAddr\":\"94.19.5.54\",\n        \"SessionId\":\"a0b7d2af-0131-440b-9257-49b828682859\",\n        \"Action\":\"request\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"5a550e77b3c6f3171f5c31b51edcfca9\"\n    },\n    \"Event\":{\n        \"event\":{\n            \"payload\":{\n                \"isSeamlessActivation\":true,\n                \"durations\":{\n                    \"onRecognitionEndTime-onVinsResponseTime\":\"17\",\n                    \"onRecognitionBeginTime-onFirstMessageMergedTime\":\"218\",\n                    \"onVinsResponseTime-onFirstSynthesisChunkTime\":\"94\",\n                    \"onStartVoiceInputTime-onRecognitionBeginTime\":\"79\"\n                },\n                \"oauth_token\":\"AQAAAAATGwIeAAQXFb7********************\",\n                \"reconnectionCount\":0,\n                \"ack\":1639845392,\n                \"isSpotterActivated\":true,\n                \"timestamps\":{\n                    \"onRecognitionBeginTime\":\"104\",\n                    \"maxAsrRtf\":11.25,\n                    \"onStartVoiceInputTime\":\"25\",\n                    \"onFirstNonEmptyPartialTime\":\"322\",\n                    \"onLastCompletedPartialTime\":\"1418\",\n                    \"requestDurationTime\":\"4636\",\n                    \"onFirstSocketActivityTime\":\"2286\",\n                    \"onFirstSynthesisChunkTime\":\"2373\",\n                    \"onFirstMessageMergedTime\":\"322\",\n                    \"onPhraseSpottedTime\":\"0\",\n                    \"averageAsrRtf\":6.213445378,\n                    \"onSoundPlayerBeginTime\":\"2425\",\n                    \"onRecognitionEndTime\":\"2262\",\n                    \"onVinsResponseTime\":\"2279\",\n                    \"onSoundPlayerEndTime\":\"4661\",\n                    \"onLastSynthesisChunkTime\":\"2414\",\n                    \"medianAsrRtf\":5.4,\n                    \"spotterConfirmationTime\":\"869\",\n                    \"minAsrRtf\":2.7,\n                    \"prevSoundPlayerEndTime\":\"0\",\n                    \"onStartVoiceInputTime\":\"0\",\n                    \"onStartVinsRequestTime\":\"0\",\n                    \"onFirstSocketActivityTime\":\"208\",\n                    \"onLastSynthesisChunkTime\":\"4115\",\n                    \"StartPlayer\":\"-9\",\n                    \"OnPlayerBegin\":\"4\",\n                    \"OnPlayerEnd\":\"630\"\n                },\n                \"refMessageId\":\"479c8d54-ed14-42ab-9b79-8823e1efd5b8\",\n                \"audioProcessingMode\":\"PASS_AUDIO\",\n                \"cancelled\":false\n            },\n            \"header\":{\n                \"name\":\"RequestStat\",\n                \"messageId\":\"a9d7753e-d256-418b-aaa5-af5285785158\",\n                \"ack\":1639845392,\n                \"refStreamId\":300,\n                \"rtLogToken\":\"1639845392957789$a9d7753e-d256-418b-aaa5-af5285785158$3d466e69-2259a0b0-380236bd-255ab271\",\n                \"namespace\":\"Log\"\n            }\n        }\n    }\n} timestamp: 2021-12-18T16:36:32.000000Z"
Uuid: "5a550e77b3c6f3171f5c31b51edcfca9"
MessageId: ""
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=5a550e77b3c6f3171f5c31b51edcfca9"
)";

constexpr TStringBuf JSON_VINS_REQUEST = R"(
{
    "Session":{
        "AppId":"aliced",
        "Timestamp":"2021-12-18T19:31:16.612268Z",
        "Uid":"",
        "AppType":"aliced",
        "IpAddr":"62.221.90.49",
        "SessionId":"2c978b31-27a6-4412-b26d-6ed4c677f17b",
        "Action":"response",
        "RetryNumber":0,
        "DoNotUseUserLogs":false,
        "Uuid":"c56b9edd0bc70f3aa871bbc05cc178ca"
    },
    "Directive":{
        "ForEvent":"862db07c-f6f5-4077-b29a-1caa63c94f0c",
        "type":"VinsRequest",
        "EffectiveVinsUrl":"http://vins.alice.yandex.net/speechkit/apply/",
        "Body":{
            "header":{
                "prev_req_id":"c98af04b-0261-4493-bd89-bed5f7103b94",
                "request_id":"862db07c-f6f5-4077-b29a-1caa63c94f0c",
                "ref_message_id":"862db07c-f6f5-4077-b29a-1caa63c94f0c",
                "sequence_number":776,
                "session_id":"2c978b31-27a6-4412-b26d-6ed4c677f17b"
            },
        }
    }
}
)";

const TString PROTO_PARSED_VINS_REQUEST = R"(
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MegamindRequestId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
TimestampLogMs: 1639855876612
)";

const TString PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_ENVIRONMENT = R"(
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
TimestampLogMs: 1639855876612
Environment {
    QloudProject: "alice"
    QloudApplication: "uniproxy"
}
)";

const TString PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_CLIENT_IP = R"(
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
TimestampLogMs: 1639855876612
ClientIp: "62.221.90.49"
)";

const TString PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_CONNECT_SESSION_ID = R"(
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
TimestampLogMs: 1639855876612
ConnectSessionId: "2c978b31-27a6-4412-b26d-6ed4c677f17b"
)";

const TString PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS = R"(
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
TimestampLogMs: 1639855876612
DoNotUseUserLogs: false
)";

const TString PROTO_PARSED_VINS_REQUEST_INVALID_UUID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid uuid from VinsRequest directive: \n{\n    \"Session\":{\n        \"AppId\":\"aliced\",\n        \"Timestamp\":\"2021-12-18T19:31:16.612268Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"aliced\",\n        \"IpAddr\":\"62.221.90.49\",\n        \"SessionId\":\"2c978b31-27a6-4412-b26d-6ed4c677f17b\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c56b9edd0bc70f3aa871bbc05cc178ca\"\n    },\n    \"Directive\":{\n        \"ForEvent\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n        \"type\":\"VinsRequest\",\n        \"EffectiveVinsUrl\":\"http://vins.alice.yandex.net/speechkit/apply/\",\n        \"Body\":{\n            \"header\":{\n                \"prev_req_id\":\"c98af04b-0261-4493-bd89-bed5f7103b94\",\n                \"request_id\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n                \"ref_message_id\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n                \"sequence_number\":776,\n                \"session_id\":\"2c978b31-27a6-4412-b26d-6ed4c677f17b\"\n            },\n        }\n    }\n}\n timestamp: 2021-12-18T19:31:16.000000Z"
Uuid: ""
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=862db07c-f6f5-4077-b29a-1caa63c94f0c"
)";

const TString PROTO_PARSED_VINS_REQUEST_INVALID_MESSAGE_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid message_id from VinsRequest directive: \n{\n    \"Session\":{\n        \"AppId\":\"aliced\",\n        \"Timestamp\":\"2021-12-18T19:31:16.612268Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"aliced\",\n        \"IpAddr\":\"62.221.90.49\",\n        \"SessionId\":\"2c978b31-27a6-4412-b26d-6ed4c677f17b\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c56b9edd0bc70f3aa871bbc05cc178ca\"\n    },\n    \"Directive\":{\n        \"ForEvent\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n        \"type\":\"VinsRequest\",\n        \"EffectiveVinsUrl\":\"http://vins.alice.yandex.net/speechkit/apply/\",\n        \"Body\":{\n            \"header\":{\n                \"prev_req_id\":\"c98af04b-0261-4493-bd89-bed5f7103b94\",\n                \"request_id\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n                \"ref_message_id\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n                \"sequence_number\":776,\n                \"session_id\":\"2c978b31-27a6-4412-b26d-6ed4c677f17b\"\n            },\n        }\n    }\n}\n timestamp: 2021-12-18T19:31:16.000000Z"
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MessageId: ""
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=c56b9edd0bc70f3aa871bbc05cc178ca"
)";

const TString PROTO_PARSED_VINS_REQUEST_INVALID_REQUEST_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid request_id from VinsRequest directive: \n{\n    \"Session\":{\n        \"AppId\":\"aliced\",\n        \"Timestamp\":\"2021-12-18T19:31:16.612268Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"aliced\",\n        \"IpAddr\":\"62.221.90.49\",\n        \"SessionId\":\"2c978b31-27a6-4412-b26d-6ed4c677f17b\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c56b9edd0bc70f3aa871bbc05cc178ca\"\n    },\n    \"Directive\":{\n        \"ForEvent\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n        \"type\":\"VinsRequest\",\n        \"EffectiveVinsUrl\":\"http://vins.alice.yandex.net/speechkit/apply/\",\n        \"Body\":{\n            \"header\":{\n                \"prev_req_id\":\"c98af04b-0261-4493-bd89-bed5f7103b94\",\n                \"request_id\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n                \"ref_message_id\":\"862db07c-f6f5-4077-b29a-1caa63c94f0c\",\n                \"sequence_number\":776,\n                \"session_id\":\"2c978b31-27a6-4412-b26d-6ed4c677f17b\"\n            },\n        }\n    }\n}\n timestamp: 2021-12-18T19:31:16.000000Z"
Uuid: "c56b9edd0bc70f3aa871bbc05cc178ca"
MessageId: "862db07c-f6f5-4077-b29a-1caa63c94f0c"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=862db07c-f6f5-4077-b29a-1caa63c94f0c"
)";

constexpr TStringBuf JSON_VINS_RESPONSE = R"(
{
    "Session":{
        "AppId":"ru.yandex.quasar.app",
        "Timestamp":"2021-12-18T14:39:49.287558Z",
        "Uid":"",
        "AppType":"quasar",
        "IpAddr":"178.173.118.213",
        "SessionId":"22a888c6-7858-4dcf-a69d-22868fb3947e",
        "Action":"response",
        "RetryNumber":0,
        "DoNotUseUserLogs":false,
        "Uuid":"c79bf50cc73fe0a203e417be1714f517"
    },
    "Directive":{
        "directive":{
            "payload":{
                "version":"vins/stable-192-4@8952380",
                "header":{
                    "response_id":"5e9ee0f8-5827bc71-5d573d9f-46c60936",
                    "asr_partial_number":null,
                    "request_id":"e4da3954-4b28-4d9b-a64a-7b29e0e410c6",
                    "ref_message_id":"fa01944b-53f8-409d-8574-0c4cd40916ab",
                    "session_id":"22a888c6-7858-4dcf-a69d-22868fb3947e",
                    "dialog_id":null
                },
            },
            "header":{
                "name":"VinsResponse",
                "refMessageId":"fa01944b-53f8-409d-8574-0c4cd40916ab",
                "messageId":"e110af0d-9850-4b8c-8c23-38588684828b",
                "namespace":"Vins"
            }
        }
    }
}
)";

const TString PROTO_PARSED_VINS_RESPONSE = R"(
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MegamindRequestId: "e4da3954-4b28-4d9b-a64a-7b29e0e410c6"
MegamindResponseId: "5e9ee0f8-5827bc71-5d573d9f-46c60936"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
TimestampLogMs: 1639838389287
)";

const TString PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_ENVIRONMENT = R"(
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
TimestampLogMs: 1639838389287
Environment {
    QloudProject: "alice"
    QloudApplication: "uniproxy"
}
)";

const TString PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_CLIENT_IP = R"(
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
TimestampLogMs: 1639838389287
ClientIp: "178.173.118.213"
)";

const TString PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_CONNECT_SESSION_ID = R"(
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
TimestampLogMs: 1639838389287
ConnectSessionId: "22a888c6-7858-4dcf-a69d-22868fb3947e"
)";

const TString PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS = R"(
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
TimestampLogMs: 1639838389287
DoNotUseUserLogs: false
)";

const TString PROTO_PARSED_VINS_RESPONSE_INVALID_UUID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid uuid from VinsResponse directive: \n{\n    \"Session\":{\n        \"AppId\":\"ru.yandex.quasar.app\",\n        \"Timestamp\":\"2021-12-18T14:39:49.287558Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"quasar\",\n        \"IpAddr\":\"178.173.118.213\",\n        \"SessionId\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c79bf50cc73fe0a203e417be1714f517\"\n    },\n    \"Directive\":{\n        \"directive\":{\n            \"payload\":{\n                \"version\":\"vins/stable-192-4@8952380\",\n                \"header\":{\n                    \"response_id\":\"5e9ee0f8-5827bc71-5d573d9f-46c60936\",\n                    \"asr_partial_number\":null,\n                    \"request_id\":\"e4da3954-4b28-4d9b-a64a-7b29e0e410c6\",\n                    \"ref_message_id\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                    \"session_id\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n                    \"dialog_id\":null\n                },\n            },\n            \"header\":{\n                \"name\":\"VinsResponse\",\n                \"refMessageId\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                \"messageId\":\"e110af0d-9850-4b8c-8c23-38588684828b\",\n                \"namespace\":\"Vins\"\n            }\n        }\n    }\n}\n timestamp: 2021-12-18T14:39:49.000000Z"
Uuid: ""
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=fa01944b-53f8-409d-8574-0c4cd40916ab"
)";

const TString PROTO_PARSED_VINS_RESPONSE_INVALID_MESSAGE_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid message_id from VinsResponse directive: \n{\n    \"Session\":{\n        \"AppId\":\"ru.yandex.quasar.app\",\n        \"Timestamp\":\"2021-12-18T14:39:49.287558Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"quasar\",\n        \"IpAddr\":\"178.173.118.213\",\n        \"SessionId\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c79bf50cc73fe0a203e417be1714f517\"\n    },\n    \"Directive\":{\n        \"directive\":{\n            \"payload\":{\n                \"version\":\"vins/stable-192-4@8952380\",\n                \"header\":{\n                    \"response_id\":\"5e9ee0f8-5827bc71-5d573d9f-46c60936\",\n                    \"asr_partial_number\":null,\n                    \"request_id\":\"e4da3954-4b28-4d9b-a64a-7b29e0e410c6\",\n                    \"ref_message_id\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                    \"session_id\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n                    \"dialog_id\":null\n                },\n            },\n            \"header\":{\n                \"name\":\"VinsResponse\",\n                \"refMessageId\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                \"messageId\":\"e110af0d-9850-4b8c-8c23-38588684828b\",\n                \"namespace\":\"Vins\"\n            }\n        }\n    }\n}\n timestamp: 2021-12-18T14:39:49.000000Z"
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: ""
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=c79bf50cc73fe0a203e417be1714f517"
)";

const TString PROTO_PARSED_VINS_RESPONSE_INVALID_REQUEST_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid request_id from VinsResponse directive: \n{\n    \"Session\":{\n        \"AppId\":\"ru.yandex.quasar.app\",\n        \"Timestamp\":\"2021-12-18T14:39:49.287558Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"quasar\",\n        \"IpAddr\":\"178.173.118.213\",\n        \"SessionId\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c79bf50cc73fe0a203e417be1714f517\"\n    },\n    \"Directive\":{\n        \"directive\":{\n            \"payload\":{\n                \"version\":\"vins/stable-192-4@8952380\",\n                \"header\":{\n                    \"response_id\":\"5e9ee0f8-5827bc71-5d573d9f-46c60936\",\n                    \"asr_partial_number\":null,\n                    \"request_id\":\"e4da3954-4b28-4d9b-a64a-7b29e0e410c6\",\n                    \"ref_message_id\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                    \"session_id\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n                    \"dialog_id\":null\n                },\n            },\n            \"header\":{\n                \"name\":\"VinsResponse\",\n                \"refMessageId\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                \"messageId\":\"e110af0d-9850-4b8c-8c23-38588684828b\",\n                \"namespace\":\"Vins\"\n            }\n        }\n    }\n}\n timestamp: 2021-12-18T14:39:49.000000Z"
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=fa01944b-53f8-409d-8574-0c4cd40916ab"
)";

const TString PROTO_PARSED_VINS_RESPONSE_INVALID_RESPONSE_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid response_id from VinsResponse directive: \n{\n    \"Session\":{\n        \"AppId\":\"ru.yandex.quasar.app\",\n        \"Timestamp\":\"2021-12-18T14:39:49.287558Z\",\n        \"Uid\":\"\",\n        \"AppType\":\"quasar\",\n        \"IpAddr\":\"178.173.118.213\",\n        \"SessionId\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n        \"Action\":\"response\",\n        \"RetryNumber\":0,\n        \"DoNotUseUserLogs\":false,\n        \"Uuid\":\"c79bf50cc73fe0a203e417be1714f517\"\n    },\n    \"Directive\":{\n        \"directive\":{\n            \"payload\":{\n                \"version\":\"vins/stable-192-4@8952380\",\n                \"header\":{\n                    \"response_id\":\"5e9ee0f8-5827bc71-5d573d9f-46c60936\",\n                    \"asr_partial_number\":null,\n                    \"request_id\":\"e4da3954-4b28-4d9b-a64a-7b29e0e410c6\",\n                    \"ref_message_id\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                    \"session_id\":\"22a888c6-7858-4dcf-a69d-22868fb3947e\",\n                    \"dialog_id\":null\n                },\n            },\n            \"header\":{\n                \"name\":\"VinsResponse\",\n                \"refMessageId\":\"fa01944b-53f8-409d-8574-0c4cd40916ab\",\n                \"messageId\":\"e110af0d-9850-4b8c-8c23-38588684828b\",\n                \"namespace\":\"Vins\"\n            }\n        }\n    }\n}\n timestamp: 2021-12-18T14:39:49.000000Z"
Uuid: "c79bf50cc73fe0a203e417be1714f517"
MessageId: "fa01944b-53f8-409d-8574-0c4cd40916ab"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=fa01944b-53f8-409d-8574-0c4cd40916ab"
)";

const TString PROTO_PARSED_TEST_IDS = R"(
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
TimestampLogMs: 1639522446777
TestIds: 412802
TestIds: 20194
)";

const TString PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CONNECT_SESSION_ID = R"(
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
TimestampLogMs: 1639522446777
ConnectSessionId: "77c7c258-e8ca-44db-8e4c-5714df1a8b76"
)";

const TString PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS = R"(
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
TimestampLogMs: 1639522446777
DoNotUseUserLogs: false
)";

const TString PROTO_PARSED_TEST_IDS_INVALID_UUID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid uuid from containing TestIds directive: \n{\n    \"Directive\":{\n        \"Body\":{\n            \"test_ids\":[\n                \"412802\",\n                \"20194\",\n            ]\n        },\n        \"ForEvent\":\"55abc467-6187-4759-b423-10d30d482507\",\n        \"type\":\"FlagsJson\"\n    },\n    \"Session\":{\n        \"AppId\":\"ru.yandex.yandexnavi\",\n        \"AppType\":\"navi\",\n        \"Action\":\"log_flags_json\",\n        \"DoNotUseUserLogs\":false,\n        \"IpAddr\":\"176.59.174.42\",\n        \"RetryNumber\":0,\n        \"SessionId\":\"77c7c258-e8ca-44db-8e4c-5714df1a8b76\",\n        \"Timestamp\":\"2021-12-14T22:54:06.777472Z\",\n        \"Uid\":\"\",\n        \"Uuid\":\"79a1450fd8f64d1a8d9921149c368ff0\"\n    }\n} timestamp: 2021-12-14T22:54:06.000000Z"
Uuid: ""
MessageId: "55abc467-6187-4759-b423-10d30d482507"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=55abc467-6187-4759-b423-10d30d482507"
)";

const TString PROTO_PARSED_TEST_IDS_INVALID_MESSAGE_ID = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Invalid message_id from containing TestIds directive: \n{\n    \"Directive\":{\n        \"Body\":{\n            \"test_ids\":[\n                \"412802\",\n                \"20194\",\n            ]\n        },\n        \"ForEvent\":\"55abc467-6187-4759-b423-10d30d482507\",\n        \"type\":\"FlagsJson\"\n    },\n    \"Session\":{\n        \"AppId\":\"ru.yandex.yandexnavi\",\n        \"AppType\":\"navi\",\n        \"Action\":\"log_flags_json\",\n        \"DoNotUseUserLogs\":false,\n        \"IpAddr\":\"176.59.174.42\",\n        \"RetryNumber\":0,\n        \"SessionId\":\"77c7c258-e8ca-44db-8e4c-5714df1a8b76\",\n        \"Timestamp\":\"2021-12-14T22:54:06.777472Z\",\n        \"Uid\":\"\",\n        \"Uuid\":\"79a1450fd8f64d1a8d9921149c368ff0\"\n    }\n} timestamp: 2021-12-14T22:54:06.000000Z"
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: ""
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=79a1450fd8f64d1a8d9921149c368ff0"
)";

const TString PROTO_PARSED_TEST_IDS_INVALID_TEST_IDS1 = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Can\'t parse testId from json: \n{\n    \"Directive\":{\n        \"Body\":{\n            \"test_ids\":[\n                412802,\n                20194,\n            ]\n        },\n        \"ForEvent\":\"55abc467-6187-4759-b423-10d30d482507\",\n        \"type\":\"FlagsJson\"\n    },\n    \"Session\":{\n        \"AppId\":\"ru.yandex.yandexnavi\",\n        \"AppType\":\"navi\",\n        \"Action\":\"log_flags_json\",\n        \"DoNotUseUserLogs\":false,\n        \"IpAddr\":\"176.59.174.42\",\n        \"RetryNumber\":0,\n        \"SessionId\":\"77c7c258-e8ca-44db-8e4c-5714df1a8b76\",\n        \"Timestamp\":\"2021-12-14T22:54:06.777472Z\",\n        \"Uid\":\"\",\n        \"Uuid\":\"79a1450fd8f64d1a8d9921149c368ff0\"\n    }\n} timestamp: 2021-12-14T22:54:06.000000Z"
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=55abc467-6187-4759-b423-10d30d482507"
)";

const TString PROTO_PARSED_TEST_IDS_INVALID_TEST_IDS2 = R"(
Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
Reason: R_INVALID_VALUE
Message: "Can\'t parse testId from json: \n{\n    \"Directive\":{\n        \"Body\":{\n            \"test_ids\":[\n                \"KFU WAS\",\n                \"IN THE FINAL\",\n                \"ACM ICPC\",\n            ]\n        },\n        \"ForEvent\":\"55abc467-6187-4759-b423-10d30d482507\",\n        \"type\":\"FlagsJson\"\n    },\n    \"Session\":{\n        \"AppId\":\"ru.yandex.yandexnavi\",\n        \"AppType\":\"navi\",\n        \"Action\":\"log_flags_json\",\n        \"DoNotUseUserLogs\":false,\n        \"IpAddr\":\"176.59.174.42\",\n        \"RetryNumber\":0,\n        \"SessionId\":\"77c7c258-e8ca-44db-8e4c-5714df1a8b76\",\n        \"Timestamp\":\"2021-12-14T22:54:06.777472Z\",\n        \"Uid\":\"\",\n        \"Uuid\":\"79a1450fd8f64d1a8d9921149c368ff0\"\n    }\n} timestamp: 2021-12-14T22:54:06.000000Z"
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
SetraceUrl: "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=55abc467-6187-4759-b423-10d30d482507"
)";

const TString PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_ENVIRONMENT = R"(
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
TimestampLogMs: 1639522446777
Environment {
    QloudProject: "alice"
    QloudApplication: "uniproxy"
}
)";

const TString PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CLIENT_IP = R"(
Uuid: "79a1450fd8f64d1a8d9921149c368ff0"
MessageId: "55abc467-6187-4759-b423-10d30d482507"
TimestampLogMs: 1639522446777
ClientIp: "176.59.174.42"
)";

TUniproxyPrepared::TEnvironment GenerateEnvironment() {
    TUniproxyPrepared::TEnvironment environment;
    environment.SetQloudApplication("uniproxy");
    environment.SetQloudProject("alice");
    return environment;
}

Y_UNIT_TEST_SUITE(Uniproxy) {
    Y_UNIT_TEST(ParseUuid) {
        {
            NJson::TJsonValue json;
            json["Session"]["Uuid"] = "qwe";
            const auto uuid = ParseUuid(json);
            UNIT_ASSERT(uuid);
            UNIT_ASSERT_EQUAL("qwe", *uuid);
        }
        {
            NJson::TJsonValue json;
            json["Session"]["Uuid1"] = "qwe";
            const auto uuid = ParseUuid(json);
            UNIT_ASSERT(!uuid);
        }
    }

    Y_UNIT_TEST(ParseConnectSessionId) {
        constexpr TStringBuf LOLKEK = "lolkek";
        {
            NJson::TJsonValue json;
            json["Session"]["SessionId"] = LOLKEK;
            const auto connectSessionId = NImpl::ParseConnectSessionId(json);
            UNIT_ASSERT(connectSessionId);
            UNIT_ASSERT_EQUAL(LOLKEK, *connectSessionId);
        }
        {
            NJson::TJsonValue json;
            json["Session"]["SessionId1337"] = LOLKEK;
            const auto connectSessionId = NImpl::ParseConnectSessionId(json);
            UNIT_ASSERT(!connectSessionId);
        }
        {
            NJson::TJsonValue json;
            json["Session"] = LOLKEK;
            const auto connectSessionId = NImpl::ParseConnectSessionId(json);
            UNIT_ASSERT(!connectSessionId);
        }
    }

    Y_UNIT_TEST(ParseClientIp) {
        constexpr TStringBuf CLIENT_IP = "420.69";
        {
            NJson::TJsonValue json;
            json["Session"]["IpAddr"] = CLIENT_IP;
            const auto clientIp = NImpl::ParseClientIp(json);
            UNIT_ASSERT(clientIp);
            UNIT_ASSERT_EQUAL(CLIENT_IP, *clientIp);
        }
        {
            NJson::TJsonValue json;
            json["Session"]["IpAddr1337"] = CLIENT_IP;
            const auto clientIp = NImpl::ParseClientIp(json);
            UNIT_ASSERT(!clientIp);
        }
        {
            NJson::TJsonValue json;
            json["Session"] = CLIENT_IP;
            const auto clientIp = NImpl::ParseClientIp(json);
            UNIT_ASSERT(!clientIp);
        }
    }

    Y_UNIT_TEST(ParseDoNotUseUserLogs) {
        {
            NJson::TJsonValue json;
            json["Session"]["DoNotUseUserLogs"] = true;
            const auto doNotUseUserLogs = NImpl::ParseDoNotUseUserLogs(json);
            UNIT_ASSERT(doNotUseUserLogs);
            UNIT_ASSERT(*doNotUseUserLogs);
        }
        {
            NJson::TJsonValue json;
            json["Session"]["DoNotUseUserLogs2"] = true;
            const auto doNotUseUserLogs = NImpl::ParseDoNotUseUserLogs(json);
            UNIT_ASSERT(!doNotUseUserLogs);
        }
        {
            NJson::TJsonValue json;
            json["Session"] = true;
            const auto doNotUseUserLogs = NImpl::ParseDoNotUseUserLogs(json);
            UNIT_ASSERT(!doNotUseUserLogs);
        }
    }

    Y_UNIT_TEST(GetEvent) {
        const NJson::TJsonValue expected = "lolkek";
        {
            NJson::TJsonValue json;
            json["Event"]["event"] = expected;
            const auto actual = NImpl::GetEvent(json);
            UNIT_ASSERT_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(GetPayload) {
        const NJson::TJsonValue expected = "lolkek";
        {
            NJson::TJsonValue json;
            json["payload"] = expected;
            const auto actual = NImpl::GetPayload(json);
            UNIT_ASSERT_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(GetDirective) {
        const NJson::TJsonValue expected = "lolkek";
        {
            NJson::TJsonValue json;
            json["Directive"] = expected;
            const auto actual = NImpl::GetDirective(json);
            UNIT_ASSERT_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(GetInternalDirective) {
        const NJson::TJsonValue expected = "lolkek";
        {
            NJson::TJsonValue json;
            json["Directive"]["directive"] = expected;
            const auto actual = NImpl::GetInternalDirective(json);
            UNIT_ASSERT_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(GetHeader) {
        const NJson::TJsonValue expected = "lolkek";
        {
            NJson::TJsonValue json;
            json["header"] = expected;
            const auto actual = NImpl::GetHeader(json);
            UNIT_ASSERT_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(ParseMegamindTimings) {
        const auto json = NJson::ReadJsonFastTree(JSON_MEGAMIND_TIMINGS);
        TMegamindTimings megamindTimings;
        UNIT_ASSERT(ParseMegamindTimings(json, megamindTimings));
        TMegamindTimings expected;
        google::protobuf::TextFormat::ParseFromString(PROTO_MEGAMIND_TIMINGS, &expected);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, megamindTimings);
    }

    Y_UNIT_TEST(ParseMegamindTimingsInvalidJson) {
        const auto json = NJson::ReadJsonFastTree(
            R"({"Directive":{"directive":{"payload":{"result_vins_run_response_is_ready_sec":"lol"}}}})");
        TMegamindTimings megamindTimings;
        UNIT_ASSERT(!ParseMegamindTimings(json, megamindTimings));
    }

    Y_UNIT_TEST(ParseTtsTimings) {
        const auto json = NJson::ReadJsonFastTree(JSON_TTS_TIMINGS);
        TTtsTimings ttsTimings;
        UNIT_ASSERT(ParseTtsTimings(json, ttsTimings));
        TTtsTimings expected;
        google::protobuf::TextFormat::ParseFromString(PROTO_TTS_TIMINGS, &expected);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, ttsTimings);
    }

    Y_UNIT_TEST(ParseTtsTimingsInvalidJson) {
        const auto json =
            NJson::ReadJsonFastTree(R"({"Directive":{"directive":{"payload":{"first_tts_chunk_sec":"kek"}}}})");
        TTtsTimings ttsTimings;
        UNIT_ASSERT(!ParseTtsTimings(json, ttsTimings));
    }

    Y_UNIT_TEST(ParseTtsGenerate) {
        const auto json = NJson::ReadJsonFastTree(JSON_TTS_GENERATE);

        TTtsGenerate ttsGenerate;
        ParseTtsGenerate(json["Event"]["event"]["payload"], ttsGenerate);
        UNIT_ASSERT_EQUAL("поверните налево", ttsGenerate.GetText());
    }

    Y_UNIT_TEST(ParseTtsGenerateInvalidJson) {
        const auto json = NJson::ReadJsonFastTree(R"({"Event":{"event":{"payload":1}}})");

        TTtsGenerate ttsGenerate;
        UNIT_ASSERT(!ParseTtsGenerate(json["Event"]["event"]["payload"], ttsGenerate));
    }

    Y_UNIT_TEST(ParseTtsGenerateInvalidJson2) {
        const auto json = NJson::ReadJsonFastTree(R"({"Event":{"event":{"payload2":1}}})");

        TTtsGenerate ttsGenerate;
        ParseTtsGenerate(json["Event"]["event"]["payload"], ttsGenerate);
        UNIT_ASSERT(!ParseTtsGenerate(json["Event"]["event"]["payload"], ttsGenerate));
    }

    Y_UNIT_TEST(ParseVoiceInput) {
        const auto json = NJson::ReadJsonFastTree(JSON_VOICE_INPUT)["Event"]["event"]["payload"];
        TVoiceInput voiceInput;
        UNIT_ASSERT(ParseVoiceInput(json, voiceInput));

        TVoiceInput expected;
        google::protobuf::TextFormat::ParseFromString(PROTO_VOICE_INPUT, &expected);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, voiceInput);
    }

    Y_UNIT_TEST(ParseSpotterStats) {
        TLogSpotter::TSpotterActivationInfo::TSpotterStats expected;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_SPOTTER_STATS, &expected));
        const auto json = NJson::ReadJsonFastTree(
            NJson::ReadJsonFastTree(JSON_LOG_SPOTTER)["Event"]["event"]["payload"]["extra"]["metainfo"].GetString());
        TLogSpotter::TSpotterActivationInfo::TSpotterStats actual;
        UNIT_ASSERT(ParseSpotterStats(json, actual));
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseSpotterActivationInfo) {
        TLogSpotter::TSpotterActivationInfo expected;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_SPOTTER_ACTIVATION_INFO, &expected));
        const auto json = NJson::ReadJsonFastTree(JSON_LOG_SPOTTER)["Event"]["event"]["payload"]["extra"];
        TLogSpotter::TSpotterActivationInfo actual;
        UNIT_ASSERT(ParseSpotterActivationInfo(json, actual));
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseAsrDebug) {
        TAsrDebug expected, actual;
        UNIT_ASSERT(google::protobuf::TextFormat::ParseFromString(PROTO_ASR_DEBUG, &expected));
        const auto json = NJson::ReadJsonFastTree(JSON_ASR_DEBUG)["Directive"];
        UNIT_ASSERT(ParseAsrDebug(json, actual));
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(ParseSpotterActivationInfoIvalidJson) {
        const auto json = NJson::ReadJsonFastTree(R"({"extra":"test"})");
        TLogSpotter::TSpotterActivationInfo proto;
        UNIT_ASSERT(!ParseSpotterActivationInfo(json, proto));
    }

    Y_UNIT_TEST(ParseSpotterActivationInfoIvalidMetaJson) {
        const auto json = NJson::ReadJsonFastTree(R"({"extra":{"metainfo":"test"}})");
        TLogSpotter::TSpotterActivationInfo proto;
        UNIT_ASSERT(!ParseSpotterActivationInfo(json, proto));
    }

    Y_UNIT_TEST(ParseAsrDebugInvalidJson) {
        const auto json = NJson::ReadJsonFastTree(R"({"debug":{"StreamValidation":{"burst_detector":1}}})");
        TAsrDebug proto;
        UNIT_ASSERT(!ParseAsrDebug(json, proto));
    }
}

Y_UNIT_TEST_SUITE(RequestStat) {
    Y_UNIT_TEST(ParseRequestStat) {
        const auto json = NJson::ReadJsonFastTree(JSON_REQUEST_STAT);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639845392), JSON_REQUEST_STAT);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::RequestStat, logsParser.GetType());

        const auto actual = logsParser.ParseRequestStat();

        UNIT_ASSERT(actual.Errors.empty());
        UNIT_ASSERT(actual.ParsedEvent);

        TUniproxyPrepared::TRequestStatWrapper expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(actual.MessageIdToEnvironment);
        TUniproxyPrepared::TMessageIdToEnvironment expectedMessageIdToEnvironment;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_ENVIRONMENT,
                                                      &expectedMessageIdToEnvironment);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToEnvironment, *actual.MessageIdToEnvironment);

        UNIT_ASSERT(actual.MessageIdToClientIp);
        TUniproxyPrepared::TMessageIdToClientIp expectedMessageIdToClientIp;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_CLIENT_IP,
                                                      &expectedMessageIdToClientIp);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToClientIp, *actual.MessageIdToClientIp);

        UNIT_ASSERT(actual.MessageIdToConnectSessionId);
        TUniproxyPrepared::TMessageIdToConnectSessionId expectedMessageIdToConnectSessionId;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_CONNECT_SESSION_ID,
                                                      &expectedMessageIdToConnectSessionId);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToConnectSessionId, *actual.MessageIdToConnectSessionId);

        UNIT_ASSERT(actual.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs expectedDoNotUseUserLogs;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS,
                                                      &expectedDoNotUseUserLogs);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedDoNotUseUserLogs, *actual.MessageIdToDoNotUseUserLogs);
    }

    Y_UNIT_TEST(ParseRequestStatBadUuid) {
        auto json = NJson::ReadJsonFastTree(JSON_REQUEST_STAT);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Session"]["Uuid"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639845392), JSON_REQUEST_STAT);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::RequestStat, logsParser.GetType());

        const auto actual = logsParser.ParseRequestStat();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT_INVALID_UUID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseRequestStatBadMessageId) {
        auto json = NJson::ReadJsonFastTree(JSON_REQUEST_STAT);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Event"]["event"]["payload"]["refMessageId"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639845392), JSON_REQUEST_STAT);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::RequestStat, logsParser.GetType());

        const auto actual = logsParser.ParseRequestStat();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT_INVALID_MESSAGE_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseRequestStatBadCommonParts) {
        auto json = NJson::ReadJsonFastTree(JSON_REQUEST_STAT);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        json["Session"].GetMapSafe().erase("IpAddr");
        json["Session"].GetMapSafe().erase("SessionId");
        json["Session"].GetMapSafe().erase("DoNotUseUserLogs");
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639845392), JSON_REQUEST_STAT);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::RequestStat, logsParser.GetType());

        const auto actual = logsParser.ParseRequestStat();

        UNIT_ASSERT(actual.Errors.empty());

        UNIT_ASSERT(actual.ParsedEvent);
        TUniproxyPrepared::TRequestStatWrapper expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_REQUEST_STAT, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);
    }
}

Y_UNIT_TEST_SUITE(VinsRequest) {
    Y_UNIT_TEST(ParseVinsRequest) {
        const auto json = NJson::ReadJsonFastTree(JSON_VINS_REQUEST);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639855876), JSON_VINS_REQUEST);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindRequest, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindRequest();

        UNIT_ASSERT(actual.Errors.empty());
        UNIT_ASSERT(actual.ParsedEvent);

        TUniproxyPrepared::TMegamindRequest expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(actual.MessageIdToEnvironment);
        TUniproxyPrepared::TMessageIdToEnvironment expectedMessageIdToEnvironment;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_ENVIRONMENT,
                                                      &expectedMessageIdToEnvironment);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToEnvironment, *actual.MessageIdToEnvironment);

        UNIT_ASSERT(actual.MessageIdToClientIp);
        TUniproxyPrepared::TMessageIdToClientIp expectedMessageIdToClientIp;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_CLIENT_IP,
                                                      &expectedMessageIdToClientIp);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToClientIp, *actual.MessageIdToClientIp);

        UNIT_ASSERT(actual.MessageIdToConnectSessionId);
        TUniproxyPrepared::TMessageIdToConnectSessionId expectedMessageIdToConnectSessionId;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_CONNECT_SESSION_ID,
                                                      &expectedMessageIdToConnectSessionId);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToConnectSessionId, *actual.MessageIdToConnectSessionId);

        UNIT_ASSERT(actual.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs expectedDoNotUseUserLogs;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS,
                                                      &expectedDoNotUseUserLogs);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedDoNotUseUserLogs, *actual.MessageIdToDoNotUseUserLogs);
    }

    Y_UNIT_TEST(ParseVinsRequestBadUuid) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_REQUEST);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Session"]["Uuid"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639855876), JSON_VINS_REQUEST);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindRequest, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindRequest();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_INVALID_UUID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsRequestBadMessageId) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_REQUEST);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Directive"]["ForEvent"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639855876), JSON_VINS_REQUEST);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindRequest, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindRequest();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_INVALID_MESSAGE_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsRequestBadRequestId) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_REQUEST);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Directive"]["Body"]["header"]["request_id"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639855876), JSON_VINS_REQUEST);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindRequest, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindRequest();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST_INVALID_REQUEST_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsRequestBadCommonParts) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_REQUEST);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        json["Session"].GetMapSafe().erase("IpAddr");
        json["Session"].GetMapSafe().erase("SessionId");
        json["Session"].GetMapSafe().erase("DoNotUseUserLogs");
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639855876), JSON_VINS_REQUEST);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindRequest, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindRequest();

        UNIT_ASSERT(actual.Errors.empty());

        UNIT_ASSERT(actual.ParsedEvent);
        TUniproxyPrepared::TMegamindRequest expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_REQUEST, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);
    }
}

Y_UNIT_TEST_SUITE(VinsResponse) {
    Y_UNIT_TEST(ParseVinsResponse) {
        const auto json = NJson::ReadJsonFastTree(JSON_VINS_RESPONSE);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639838389), JSON_VINS_RESPONSE);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindResponse, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindResponse();

        UNIT_ASSERT(actual.Errors.empty());
        UNIT_ASSERT(actual.ParsedEvent);

        TUniproxyPrepared::TMegamindResponse expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(actual.MessageIdToEnvironment);
        TUniproxyPrepared::TMessageIdToEnvironment expectedMessageIdToEnvironment;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_ENVIRONMENT,
                                                      &expectedMessageIdToEnvironment);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToEnvironment, *actual.MessageIdToEnvironment);

        UNIT_ASSERT(actual.MessageIdToClientIp);
        TUniproxyPrepared::TMessageIdToClientIp expectedMessageIdToClientIp;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_CLIENT_IP,
                                                      &expectedMessageIdToClientIp);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToClientIp, *actual.MessageIdToClientIp);

        UNIT_ASSERT(actual.MessageIdToConnectSessionId);
        TUniproxyPrepared::TMessageIdToConnectSessionId expectedMessageIdToConnectSessionId;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_CONNECT_SESSION_ID,
                                                      &expectedMessageIdToConnectSessionId);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToConnectSessionId, *actual.MessageIdToConnectSessionId);

        UNIT_ASSERT(actual.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs expectedDoNotUseUserLogs;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS,
                                                      &expectedDoNotUseUserLogs);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedDoNotUseUserLogs, *actual.MessageIdToDoNotUseUserLogs);
    }

    Y_UNIT_TEST(ParseVinsResponseBadUuid) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_RESPONSE);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Session"]["Uuid"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639838389), JSON_VINS_RESPONSE);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindResponse, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindResponse();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_INVALID_UUID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsResponseBadMessageId) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_RESPONSE);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Directive"]["directive"]["header"]["refMessageId"] = "";
        json["Directive"]["directive"]["payload"]["header"]["ref_message_id"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639838389), JSON_VINS_RESPONSE);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindResponse, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindResponse();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_INVALID_MESSAGE_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsResponseBadRequestId) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_RESPONSE);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Directive"]["directive"]["payload"]["header"]["request_id"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639838389), JSON_VINS_RESPONSE);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindResponse, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindResponse();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_INVALID_REQUEST_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsResponseBadResponseId) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_RESPONSE);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Directive"]["directive"]["payload"]["header"]["response_id"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639838389), JSON_VINS_RESPONSE);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindResponse, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindResponse();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE_INVALID_RESPONSE_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseVinsResponseBadCommonParts) {
        auto json = NJson::ReadJsonFastTree(JSON_VINS_RESPONSE);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        json["Session"].GetMapSafe().erase("IpAddr");
        json["Session"].GetMapSafe().erase("SessionId");
        json["Session"].GetMapSafe().erase("DoNotUseUserLogs");
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639838389), JSON_VINS_RESPONSE);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::MegamindResponse, logsParser.GetType());

        const auto actual = logsParser.ParseMegamindResponse();

        UNIT_ASSERT(actual.Errors.empty());

        UNIT_ASSERT(actual.ParsedEvent);
        TUniproxyPrepared::TMegamindResponse expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_VINS_RESPONSE, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);
    }
}

Y_UNIT_TEST_SUITE(TestIds) {
    Y_UNIT_TEST(ParseTestIds) {
        const auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::TestIds, logsParser.GetType());

        const auto actual = logsParser.ParseTestIds();

        UNIT_ASSERT(actual.Errors.empty());
        UNIT_ASSERT(actual.ParsedEvent);

        TUniproxyPrepared::TTestIds expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(actual.MessageIdToEnvironment);
        TUniproxyPrepared::TMessageIdToEnvironment expectedMessageIdToEnvironment;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_ENVIRONMENT,
                                                      &expectedMessageIdToEnvironment);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToEnvironment, *actual.MessageIdToEnvironment);

        UNIT_ASSERT(actual.MessageIdToClientIp);
        TUniproxyPrepared::TMessageIdToClientIp expectedMessageIdToClientIp;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CLIENT_IP,
                                                      &expectedMessageIdToClientIp);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToClientIp, *actual.MessageIdToClientIp);

        UNIT_ASSERT(actual.MessageIdToConnectSessionId);
        TUniproxyPrepared::TMessageIdToConnectSessionId expectedMessageIdToConnectSessionId;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CONNECT_SESSION_ID,
                                                      &expectedMessageIdToConnectSessionId);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToConnectSessionId, *actual.MessageIdToConnectSessionId);

        UNIT_ASSERT(actual.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs expectedDoNotUseUserLogs;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS,
                                                      &expectedDoNotUseUserLogs);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedDoNotUseUserLogs, *actual.MessageIdToDoNotUseUserLogs);
    }

    Y_UNIT_TEST(ParseTestIdWrongJson) {
        auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS);
        json["Session"].GetMapSafe().erase("Action");
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS);
        UNIT_ASSERT_EQUAL(EUniproxyEventType::Unknown, logsParser.GetType());
    }

    Y_UNIT_TEST(ParseTestIdsBadUuid) {
        auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Session"]["Uuid"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::TestIds, logsParser.GetType());

        const auto actual = logsParser.ParseTestIds();

        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_INVALID_UUID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseTestIdsBadMessageId) {
        auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment = GenerateEnvironment();
        json["Directive"]["ForEvent"] = "";
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::TestIds, logsParser.GetType());

        const auto actual = logsParser.ParseTestIds();
        UNIT_ASSERT(!actual.ParsedEvent);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_INVALID_MESSAGE_ID, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);
    }

    Y_UNIT_TEST(ParseTestIdsBadCommonParts) {
        auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        json["Session"].GetMapSafe().erase("SessionId");
        json["Session"].GetMapSafe().erase("DoNotUseUserLogs");
        json["Session"].GetMapSafe().erase("IpAddr");
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::TestIds, logsParser.GetType());

        const auto actual = logsParser.ParseTestIds();

        UNIT_ASSERT(actual.Errors.empty());

        UNIT_ASSERT(actual.ParsedEvent);
        TUniproxyPrepared::TTestIds expectedParsedEvent;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS, &expectedParsedEvent);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedParsedEvent, *actual.ParsedEvent);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
        UNIT_ASSERT(!actual.MessageIdToClientIp);
        UNIT_ASSERT(!actual.MessageIdToConnectSessionId);
        UNIT_ASSERT(!actual.MessageIdToDoNotUseUserLogs);
    }

    Y_UNIT_TEST(ParseTestIdsBadTestIdsPart1) {
        auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS_INVALID1);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS_INVALID1);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::TestIds, logsParser.GetType());

        const auto actual = logsParser.ParseTestIds();
        UNIT_ASSERT(!actual.ParsedEvent);
        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;

        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_INVALID_TEST_IDS1, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);

        UNIT_ASSERT(actual.MessageIdToClientIp);
        TUniproxyPrepared::TMessageIdToClientIp expectedMessageIdToClientIp;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CLIENT_IP,
                                                      &expectedMessageIdToClientIp);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToClientIp, *actual.MessageIdToClientIp);

        UNIT_ASSERT(actual.MessageIdToConnectSessionId);
        TUniproxyPrepared::TMessageIdToConnectSessionId expectedMessageIdToConnectSessionId;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CONNECT_SESSION_ID,
                                                      &expectedMessageIdToConnectSessionId);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToConnectSessionId, *actual.MessageIdToConnectSessionId);

        UNIT_ASSERT(actual.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs expectedDoNotUseUserLogs;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS,
                                                      &expectedDoNotUseUserLogs);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedDoNotUseUserLogs, *actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
    }

    Y_UNIT_TEST(ParseTestIdsBadTestIdsPart2) {
        auto json = NJson::ReadJsonFastTree(JSON_TEST_IDS_INVALID2);
        const TMaybe<TUniproxyPrepared::TEnvironment> environment;
        TUniproxyLogsParser logsParser(json, environment, TInstant::Seconds(1639522446), JSON_TEST_IDS_INVALID2);

        UNIT_ASSERT_EQUAL(EUniproxyEventType::TestIds, logsParser.GetType());

        const auto actual = logsParser.ParseTestIds();
        UNIT_ASSERT(!actual.ParsedEvent);

        UNIT_ASSERT_EQUAL(1, actual.Errors.size());
        TUniproxyPrepared::TError expectedError;

        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_INVALID_TEST_IDS2, &expectedError);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedError, actual.Errors[0]);

        UNIT_ASSERT(actual.MessageIdToClientIp);
        TUniproxyPrepared::TMessageIdToClientIp expectedMessageIdToClientIp;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CLIENT_IP,
                                                      &expectedMessageIdToClientIp);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToClientIp, *actual.MessageIdToClientIp);

        UNIT_ASSERT(actual.MessageIdToConnectSessionId);
        TUniproxyPrepared::TMessageIdToConnectSessionId expectedMessageIdToConnectSessionId;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_CONNECT_SESSION_ID,
                                                      &expectedMessageIdToConnectSessionId);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedMessageIdToConnectSessionId, *actual.MessageIdToConnectSessionId);

        UNIT_ASSERT(actual.MessageIdToDoNotUseUserLogs);
        TUniproxyPrepared::TMessageIdToDoNotUseUserLogs expectedDoNotUseUserLogs;
        google::protobuf::TextFormat::ParseFromString(PROTO_PARSED_TEST_IDS_MESSAGE_ID_TO_DO_NOT_USE_USER_LOGS,
                                                      &expectedDoNotUseUserLogs);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedDoNotUseUserLogs, *actual.MessageIdToDoNotUseUserLogs);

        UNIT_ASSERT(!actual.MessageIdToEnvironment);
    }
}

} // namespace
