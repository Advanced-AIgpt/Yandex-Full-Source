#include <alice/wonderlogs/daily/lib/ttls.h>
#include <alice/wonderlogs/daily/lib/wonderlogs.h>

#include <alice/wonderlogs/library/common/utils.h>
#include <alice/wonderlogs/library/yt/utils.h>
#include <alice/wonderlogs/protos/asr_prepared.pb.h>
#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/private_user.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include <library/cpp/resource/registry.h>
#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/tests/yt_unittest_lib/yt_unittest_lib.h>

#include <util/string/split.h>

using namespace NAlice::NWonderlogs;

const TString WONDERLOG = R"(
Uuid: "00000000-something"
MessageId: "58f677d4-d42a-454c-a8f6-d216b00fe998"
MegamindRequestId: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
SpeechkitRequest {
    Header {
        RequestId: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
        RefMessageId: "58f677d4-d42a-454c-a8f6-d216b00fe998"
        SessionId: "eb786b54-e8f9a63-8318967-b2d59dd2"
    }
    Application {
        AppId: "ru.yandex.searchplugin"
        AppVersion: "7.52"
        OsVersion: "8.1.0"
        Platform: "android"
        Uuid: "00000000-something"
        Lang: "ru-RU"
        ClientTime: "20191001T072031"
        Timezone: "Asia/Krasnoyarsk"
        Epoch: "1569889231"
        DeviceModel: "TM-5084"
        DeviceManufacturer: "Texet"
    }
    Request {
        Event {
            Type: voice_input
            HypothesisNumber: 32
            EndOfUtterance: true
            AsrResult {
                Confidence: 1
                Words {
                    Value: "something"
                }
                Normalized: "something"
            }
            AsrResult {
                Confidence: 1
                Words {
                    Value: "something"
                }
                Words {
                    Value: "something"
                }
                Words {
                    Value: "something"
                }
            }
            Name: ""
            AsrCoreDebug: "{\"HypoFeats\":[{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-6.980543613,\"lm_eos_score\":-0.2635586858,\"normalized_lm_score\":-1.745135903,\"acoustic_score\":-0.005813543219}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-11.36527157,\"lm_eos_score\":-0.2462264597,\"normalized_lm_score\":-2.273054361,\"acoustic_score\":-2.17504096}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-10.89077377,\"lm_eos_score\":-0.1910991818,\"normalized_lm_score\":-1.815128922,\"acoustic_score\":-2.581600189}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-13.04352951,\"lm_eos_score\":-0.1856827885,\"normalized_lm_score\":-2.173921585,\"acoustic_score\":-3.433162689}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-13.01620483,\"lm_eos_score\":-0.2318917215,\"normalized_lm_score\":-2.169367552,\"acoustic_score\":-3.395662785}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-13.66531849,\"lm_eos_score\":-0.3017379045,\"normalized_lm_score\":-2.277553082,\"acoustic_score\":-3.345662594}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-9.838981628,\"lm_eos_score\":-0.3628558815,\"normalized_lm_score\":-2.459745407,\"acoustic_score\":-3.303388596}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-6.199226856,\"lm_eos_score\":-0.2192509323,\"normalized_lm_score\":-2.066408873,\"acoustic_score\":-3.78557682}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-10.6074934,\"lm_eos_score\":-0.402305454,\"normalized_lm_score\":-2.65187335,\"acoustic_score\":-3.524370909}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-16.20116425,\"lm_eos_score\":-0.4030920863,\"normalized_lm_score\":-4.050291061,\"acoustic_score\":-3.050598383}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-9.985442162,\"lm_eos_score\":-0.3852453828,\"normalized_lm_score\":-2.49636054,\"acoustic_score\":-4.005037785}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-14.27510452,\"lm_eos_score\":-0.2462264597,\"normalized_lm_score\":-2.855021,\"acoustic_score\":-4.18062067}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-17.70622444,\"lm_eos_score\":-0.4030920863,\"normalized_lm_score\":-4.42655611,\"acoustic_score\":-3.373787642}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-11.4213953,\"lm_eos_score\":-0.5284162164,\"normalized_lm_score\":-2.855348825,\"acoustic_score\":-4.642831802}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-7.58921957,\"lm_eos_score\":-1.937794328,\"normalized_lm_score\":-2.529739857,\"acoustic_score\":-5.590480328}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-9.791824341,\"lm_eos_score\":-1.798438668,\"normalized_lm_score\":-2.447956085,\"acoustic_score\":-6.274723053}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-5.067346096,\"lm_eos_score\":-0.6556411386,\"normalized_lm_score\":-2.533673048,\"acoustic_score\":-8.580151558}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-3.377486229,\"lm_eos_score\":-3.377486229,\"normalized_lm_score\":-3.377486229,\"acoustic_score\":-10.5535717}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-109.0112305,\"lm_eos_score\":-1.593458176,\"normalized_lm_score\":-27.25280762,\"acoustic_score\":-4.588311195}},{\"seq2seq_info\":{\"unfinished\":false,\"lm_score\":-109.0112305,\"lm_eos_score\":-1.593458176,\"normalized_lm_score\":-27.25280762,\"acoustic_score\":-4.653648853}}],\"ClientChunksSamples\":[0,0,3200,3200,0,3200,3200,3200,0,3200,3200,3200,0,3200,3200,3200,0,3200,3200,3200,0,3200,3200,3200,0,3200,3200,3200,0,3200,3200,3200,0,3200],\"AsrTimeSpent\":0.118128,\"HostBSConfiguration\":\"a_geo_sas a_dc_sas a_itype_asrgpu a_ctype_prestable a_prj_asr-dialogeneral-gpu a_metaprj_unknown a_tier_none use_hq_spec enable_hq_report enable_hq_poll\",\"CpuUsage\":6.602330685,\"PreEOU\":null,\"WhisperData\":{\"WhisperScore\":2.752701391e-07},\"AsrAudioDuration\":4.8,\"SpotterEOUIsAlwaysFalse\":true,\"InDegradationMode\":false,\"ClientChunksBytes\":[841,358,532,461,490,451,506,453,606,488,513,489,533,473,516,454,558,426,521,461,496,463,522,418,460,383,461,475,584,470,529,381,495,475],\"PartialsInterval\":12800,\"AsrAfterLastChunk\":0.041917,\"EOU\":{\"TrueEOUFrame\":479,\"L2EOU\":{\"NAsr::TMaxSilenceDurationEOUDetector\":{\"last_partial_dur\":0.8,\"last_update_time\":3.98,\"last_partial_text\":\"\320\274\320\265\320\275\321\216 \320\264\320\273\321\217 \320\277\320\276\321\205\321\203\320\264\320\265\320\275\320\270\321\217\",\"is_eou\":false},\"NAsr::TOldCatboostEOUDetector\":{\"cur_score\":0.9253289104,\"last_update_time\":4.78,\"last_partial_text\":\"\320\274\320\265\320\275\321\216 \320\264\320\273\321\217 \320\277\320\276\321\205\321\203\320\264\320\265\320\275\320\270\321\217\",\"state\":\"TriggeredByScore\"}},\"SamplesConsumed\":76800,\"FinalEOUFrame\":479,\"FramesConsumed\":479},\"RecognizedAudioDur\":0}"
            OriginalZeroAsrHypothesisIndex: 0
        }
        Location {
            Lat: 53.69345856
            Lon: 88.04698181
            Accuracy: 140
            Recency: 0
        }
        Experiments {
            Storage {
                key: "afisha_poi_events"
                value {
                    String: "1"
                }
            }
        }
        AdditionalOptions {
            BassOptions {
                UserAgent: "Mozilla/5.0 (Linux; Android 8.1.0; TM-5084) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.5 BroPP/1.0 Mobile Safari/537.36 YandexSearch/7.52"
                FiltrationLevel: 1
                ClientIP: "31.173.240.63"
                ScreenScaleFactor: 1.5
            }
            YandexUID: "8229842011627181586"
            AppInfo: "eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0="
            DoNotUseUserLogs: false
            ICookie: "4443374540000509505"
            Expboxes: "341893,0,77;341896,0,16;341903,0,58;341910,0,28;341915,0,85;341922,0,56;341933,0,7;341938,0,40;341944,0,23;341926,0,31;432015,0,39;315614,0,16;330999,0,41;411791,0,58;436513,0,90"
        }
        VoiceSession: true
        ResetSession: true
        LaasRegion {
            fields {
                key: "city_id"
                value {
                    number_value: 11287
                }
            }
            fields {
                key: "country_id_by_ip"
                value {
                    number_value: 225
                }
            }
        }
    }
}
SpeechkitResponse {
    Header {
        RequestId: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
        ResponseId: "a26bacd8-8d6841b2-6afbe7b7-fdbb0909"
        RefMessageId: "58f677d4-d42a-454c-a8f6-d216b00fe998"
        SessionId: "eb786b54-e8f9a63-8318967-b2d59dd2"
    }
    VoiceResponse {
        OutputSpeech {
            Type: "simple"
            Text: "something"
        }
        ShouldListen: false
    }
    Response {
        Cards {
            Type: "text_with_button"
            Text: "something"
            Buttons {
                Type: "action"
                Title: "something"
                Directives {
                    Type: "client_action"
                    Name: "open_uri"
                    AnalyticsType: "open_uri"
                    Payload {
                        fields {
                            key: "uri"
                            value {
                                string_value: "viewport://?l10n=ru-RU&lr=11287&noreask=1&query_source=alice&text=%D0%BC%D0%B5%D0%BD%D1%8E%20%D0%B4%D0%BB%D1%8F%20%D0%BF%D0%BE%D1%85%D1%83%D0%B4%D0%B5%D0%BD%D0%B8%D1%8F&viewport_id=serp"
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
                                string_value: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
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
                                string_value: "934568c0-84d1b57b-73cbd86d-42780c5a"
                            }
                        }
                        fields {
                            key: "caption"
                            value {
                                string_value: "something"
                            }
                        }
                        fields {
                            key: "request_id"
                            value {
                                string_value: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
                            }
                        }
                        fields {
                            key: "scenario_name"
                            value {
                                string_value: "Search"
                            }
                        }
                    }
                    IsLedSilent: true
                }
            }
        }
        Directives {
            Type: "client_action"
            Name: "open_uri"
            AnalyticsType: "open_uri"
            Payload {
                fields {
                    key: "uri"
                    value {
                        string_value: "viewport://?l10n=ru-RU&lr=11287&noreask=1&query_source=alice&text=%D0%BC%D0%B5%D0%BD%D1%8E%20%D0%B4%D0%BB%D1%8F%20%D0%BF%D0%BE%D1%85%D1%83%D0%B4%D0%B5%D0%BD%D0%B8%D1%8F&viewport_id=serp"
                    }
                }
            }
        }
        QualityStorage {
            PreclassifierPredicts {
                key: "GeneralConversation"
                value: 0
            }
            PreclassifierPredicts {
                key: "HollywoodMusic"
                value: -6.18725395
            }
        }
        Experiments {
        }
        Templates {
        }
        DirectivesExecutionPolicy: BeforeSpeech
    }
    Version: "vins/stable-174-1@8718054"
    MegamindAnalyticsInfo {
        AnalyticsInfo {
            key: "Search"
            value {
                ScenarioAnalyticsInfo {
                    Intent: "serp"
                    Objects {
                        Id: "tagger_query"
                        Name: "tagger query"
                        HumanReadable: "something"
                    }
                    Objects {
                        Id: "selected_fact"
                        Name: "{\"url\":\"viewport://?l10n=ru-RU&lr=11287&noreask=1&query_source=alice&text=%D0%BC%D0%B5%D0%BD%D1%8E%20%D0%B4%D0%BB%D1%8F%20%D0%BF%D0%BE%D1%85%D1%83%D0%B4%D0%B5%D0%BD%D0%B8%D1%8F&viewport_id=serp\"}"
                        HumanReadable: "something"
                    }
                    ProductScenarioName: "serp"
                }
                Version: "vins/stable-174-1@8718054"
                FrameActions {
                    key: "2"
                    value {
                        NluHint {
                            FrameName: "2"
                        }
                        Directives {
                            List {
                                OpenUriDirective {
                                    Name: "open_uri"
                                    Uri: "viewport://?l10n=ru-RU&lr=11287&noreask=1&query_source=alice&text=%D0%BC%D0%B5%D0%BD%D1%8E%20%D0%B4%D0%BB%D1%8F%20%D0%BF%D0%BE%D1%85%D1%83%D0%B4%D0%B5%D0%BD%D0%B8%D1%8F&viewport_id=serp"
                                }
                            }
                        }
                    }
                }
                MatchedSemanticFrames {
                    Name: "personal_assistant.scenarios.search"
                    Slots {
                        Name: "query"
                        Type: "string"
                        Value: "something"
                        AcceptedTypes: "string"
                    }
                    TypedSemanticFrame {
                        SearchSemanticFrame {
                            Query {
                                StringValue: "something"
                            }
                        }
                    }
                }
            }
        }
        OriginalUtterance: "something"
        ModifiersInfo {
            Proactivity {
                SemanticFramesInfo {
                    Source: Begemot
                    SemanticFrames {
                        Name: "personal_assistant.scenarios.search"
                        Slots {
                            Name: "query"
                            Type: "string"
                            Value: "something"
                            AcceptedTypes: "string"
                        }
                        TypedSemanticFrame {
                            SearchSemanticFrame {
                                Query {
                                    StringValue: "something"
                                }
                            }
                        }
                    }
                }
                Source: "serp"
            }
        }
        ScenarioTimings {
            key: "GeneralConversation"
            value {
                Timings {
                    key: "run"
                    value {
                        StartTimestamp: 1634556943615290
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
                        StartTimestamp: 1634556943615372
                    }
                }
            }
        }
        WinnerScenario {
            Name: "Search"
        }
        PostClassifyDuration: 5136
        ShownUtterance: "something"
        Location {
            Lat: 53.69345856
            Lon: 88.04698181
            Accuracy: 140
            Recency: 0
            Speed: 0
        }
        ChosenUtterance: "something"
        ModifiersAnalyticsInfo {
            Proactivity {
                SemanticFramesInfo {
                    Source: Begemot
                    SemanticFrames {
                        Name: "personal_assistant.scenarios.search"
                        Slots {
                            Name: "query"
                            Type: "string"
                            Value: "something"
                            AcceptedTypes: "string"
                        }
                        TypedSemanticFrame {
                            SearchSemanticFrame {
                                Query {
                                    StringValue: "something"
                                }
                            }
                        }
                    }
                }
                Source: "serp"
            }
        }
    }
}
RequestStat {
    Timestamps {
        OnSoundPlayerEndTime: "7324"
        OnVinsResponseTime: "6015"
        OnFirstSynthesisChunkTime: "6086"
        OnRecognitionBeginTime: "143"
        OnRecognitionEndTime: "5644"
    }
}
ServerTimeMs: 0
Presence {
    Uniproxy: true
    Megamind: true
    Asr: true
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
    }
}
Action: VOICE
Environment {
    MegamindEnvironment {
        Environment: "megamind_standalone_sas"
        Provider: "megamind"
    }
    UniproxyEnvironment {
        QloudProject: "alice"
        QloudApplication: "uniproxy"
    }
}
DownloadingInfo {
    UniproxyClientIp: "31.173.240.63"
    UniproxyYandexIp: false
    MegamindClientIp: "31.173.240.63"
    MegamindYandexIp: false
    Uniproxy {
        ClientIp: "31.173.240.63"
        YandexNet: false
        StaffNet: false
    }
    Megamind {
        ClientIp: "31.173.240.63"
        YandexNet: false
        StaffNet: false
    }
}
Asr {
    Data {
        MdsKey: "asr-logs/2021-10-18/4cc2814-6d9dec-d78ea162-177771d5"
        Recognition {
            Normalized: "something"
            Words: "something"
            Words: "something"
            Words: "something"
        }
        Trash: false
        Hypotheses {
            Words: "something"
            Words: "something"
            Words: "something"
            Words: "something"
        }
    }
    VoiceByUniproxy {
        Mds: "something.opus"
        Format: "audio/opus"
    }
    TrashOrEmpty: false
    Topics {
        Model: "dialogeneral-gpu"
    }
}
Spotter {
    FalseActivation: false
}
RealMessageId: true
Privacy {
    GeoRestrictions {
        ProhibitedByRegion: false
        Region: R_COUNTRY_RUSSIA
    }
}
Client {
    Application {
        AppId: "ru.yandex.searchplugin"
        AppVersion: "7.52"
        OsVersion: "8.1.0"
        Platform: "android"
        Uuid: "00000000-something"
        Lang: "ru-RU"
        ClientTime: "20191001T072031"
        Timezone: "Asia/Krasnoyarsk"
        Epoch: "1569889231"
        DeviceModel: "TM-5084"
        DeviceManufacturer: "Texet"
    }
    Type: AT_HUMAN
}
Timings {
    Uniproxy {
        Megamind {
            VinsRunDelayAfterEouDurationSec: 0.549
            UsefulVinsRequestDurationSec: 0.549
            MeanVinsRequestDurationSec: 0.549
            VinsRequestCount: 1
            LastPartialSec: 27.614
            LastVinsRunRequestDurationSec: 0.549
            UsefulVinsRequestEvage: 27.776
            Epoch: 1634556915
            VinsResponseSec: 28.33
            FirstAsrResultSec: 22.934
            VinsSessionLoadEndEvage: 22.576
            EndOfUtteranceSec: 27.776
            VinsWaitAfterEouDurationSec: 0.549
            FinishVinsRequestEou: 28.325
            StartVinsRequestEou: 27.776
        }
        Tts {
            FirstTtsChunkSec: 28.335
            UsefulResponseForUserEvage: 28.335
        }
    }
}
)";

const TString CENSORED_WONDERLOG = R"(
Uuid: "00000000-something"
MessageId: "58f677d4-d42a-454c-a8f6-d216b00fe998"
MegamindRequestId: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
SpeechkitRequest {
    Header {
        RequestId: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
        RefMessageId: "58f677d4-d42a-454c-a8f6-d216b00fe998"
        SessionId: "eb786b54-e8f9a63-8318967-b2d59dd2"
    }
    Application {
        AppId: "ru.yandex.searchplugin"
        AppVersion: "7.52"
        OsVersion: "8.1.0"
        Platform: "android"
        Uuid: "00000000-something"
        Lang: "ru-RU"
        ClientTime: "20191001T072031"
        Timezone: "Asia/Krasnoyarsk"
        Epoch: "1569889231"
        DeviceModel: "TM-5084"
        DeviceManufacturer: "Texet"
    }
    Request {
        Event {
            Type: voice_input
            HypothesisNumber: 32
            EndOfUtterance: true
            AsrResult {
                Confidence: 1
                Words {
                    Value: "***"
                }
                Normalized: "***"
            }
            AsrResult {
                Confidence: 1
                Words {
                    Value: "***"
                }
                Words {
                    Value: "***"
                }
                Words {
                    Value: "***"
                }
            }
            Name: ""
            AsrCoreDebug: "***"
            OriginalZeroAsrHypothesisIndex: 0
        }
        Location {
            Lat: 53.69345856
            Lon: 88.04698181
            Accuracy: 140
            Recency: 0
        }
        Experiments {
            Storage {
                key: "afisha_poi_events"
                value {
                    String: "1"
                }
            }
        }
        AdditionalOptions {
            BassOptions {
                UserAgent: "Mozilla/5.0 (Linux; Android 8.1.0; TM-5084) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.5 BroPP/1.0 Mobile Safari/537.36 YandexSearch/7.52"
                FiltrationLevel: 1
                ClientIP: "31.173.240.63"
                ScreenScaleFactor: 1.5
            }
            YandexUID: "8229842011627181586"
            AppInfo: "eyJicm93c2VyTmFtZSI6IllhbmRleFNlYXJjaCIsImRldmljZVR5cGUiOiJ0b3VjaCIsImRldmljZU1vZGVsIjoiIiwibW9iaWxlUGxhdGZvcm0iOiJhbmRyb2lkIn0="
            DoNotUseUserLogs: false
            ICookie: "4443374540000509505"
            Expboxes: "341893,0,77;341896,0,16;341903,0,58;341910,0,28;341915,0,85;341922,0,56;341933,0,7;341938,0,40;341944,0,23;341926,0,31;432015,0,39;315614,0,16;330999,0,41;411791,0,58;436513,0,90"
        }
        VoiceSession: true
        ResetSession: true
        LaasRegion {
            fields {
                key: "city_id"
                value {
                    number_value: 11287
                }
            }
            fields {
                key: "country_id_by_ip"
                value {
                    number_value: 225
                }
            }
        }
    }
}
SpeechkitResponse {
    Header {
        RequestId: "5829fc22-9abe-4356-8573-ef58f9c1ef99"
        ResponseId: "a26bacd8-8d6841b2-6afbe7b7-fdbb0909"
        RefMessageId: "58f677d4-d42a-454c-a8f6-d216b00fe998"
        SessionId: "eb786b54-e8f9a63-8318967-b2d59dd2"
    }
    VoiceResponse {
        OutputSpeech {
            Type: "simple"
            Text: "***"
        }
        ShouldListen: false
    }
    Response {
        Cards {
            Type: "text_with_button"
            Text: "***"
            Buttons {
            }
        }
        Directives {
            Type: "client_action"
            Name: "open_uri"
            AnalyticsType: "open_uri"
            Payload {}
        }
        QualityStorage {
            PreclassifierPredicts {
                key: "GeneralConversation"
                value: 0
            }
            PreclassifierPredicts {
                key: "HollywoodMusic"
                value: -6.18725395
            }
        }
        Experiments {
        }
        Templates {
        }
        DirectivesExecutionPolicy: BeforeSpeech
    }
    Version: "vins/stable-174-1@8718054"
    MegamindAnalyticsInfo {
        AnalyticsInfo {
            key: "Search"
            value {
                ScenarioAnalyticsInfo {
                    Intent: "serp"
                    Objects {
                    }
                    ProductScenarioName: "serp"
                }
                Version: "vins/stable-174-1@8718054"
                FrameActions {
                    key: ""
                    value {
                    }
                }
                MatchedSemanticFrames {}
            }
        }
        OriginalUtterance: "***"
        ModifiersInfo {
            Proactivity {
                SemanticFramesInfo {
                    Source: Begemot
                    SemanticFrames {}
                }
                Source: "serp"
            }
        }
        ScenarioTimings {
            key: "GeneralConversation"
            value {
                Timings {
                    key: "run"
                    value {
                        StartTimestamp: 1634556943615290
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
                        StartTimestamp: 1634556943615372
                    }
                }
            }
        }
        WinnerScenario {
            Name: "Search"
        }
        PostClassifyDuration: 5136
        ShownUtterance: "***"
        Location {
            Lat: 53.69345856
            Lon: 88.04698181
            Accuracy: 140
            Recency: 0
            Speed: 0
        }
        ChosenUtterance: "***"
        ModifiersAnalyticsInfo {
            Proactivity {
                SemanticFramesInfo {
                    Source: Begemot
                    SemanticFrames {}
                }
                Source: "serp"
            }
        }
    }
}
RequestStat {
    Timestamps {
        OnSoundPlayerEndTime: "7324"
        OnVinsResponseTime: "6015"
        OnFirstSynthesisChunkTime: "6086"
        OnRecognitionBeginTime: "143"
        OnRecognitionEndTime: "5644"
    }
}
ServerTimeMs: 0
Presence {
    Uniproxy: true
    Megamind: true
    Asr: true
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
    }
}
Action: VOICE
Environment {
    MegamindEnvironment {
        Environment: "megamind_standalone_sas"
        Provider: "megamind"
    }
    UniproxyEnvironment {
        QloudProject: "alice"
        QloudApplication: "uniproxy"
    }
}
DownloadingInfo {
    UniproxyClientIp: "31.173.240.63"
    UniproxyYandexIp: false
    MegamindClientIp: "31.173.240.63"
    MegamindYandexIp: false
    Uniproxy {
        ClientIp: "31.173.240.63"
        YandexNet: false
        StaffNet: false
    }
    Megamind {
        ClientIp: "31.173.240.63"
        YandexNet: false
        StaffNet: false
    }
}
Asr {
    Data {
        MdsKey: "***"
        Recognition {
            Normalized: "***"
            Words: "***"
        }
        Trash: false
        Hypotheses {
        }
    }
    VoiceByUniproxy {
    }
    TrashOrEmpty: false
    Topics {
        Model: "dialogeneral-gpu"
    }
}
Spotter {
    FalseActivation: false
}
RealMessageId: true
Privacy {
    GeoRestrictions {
        ProhibitedByRegion: false
        Region: R_COUNTRY_RUSSIA
    }
}
Client {
    Application {
        AppId: "ru.yandex.searchplugin"
        AppVersion: "7.52"
        OsVersion: "8.1.0"
        Platform: "android"
        Uuid: "00000000-something"
        Lang: "ru-RU"
        ClientTime: "20191001T072031"
        Timezone: "Asia/Krasnoyarsk"
        Epoch: "1569889231"
        DeviceModel: "TM-5084"
        DeviceManufacturer: "Texet"
    }
    Type: AT_HUMAN
}
Timings {
    Uniproxy {
        Megamind {
            VinsRunDelayAfterEouDurationSec: 0.549
            UsefulVinsRequestDurationSec: 0.549
            MeanVinsRequestDurationSec: 0.549
            VinsRequestCount: 1
            LastPartialSec: 27.614
            LastVinsRunRequestDurationSec: 0.549
            UsefulVinsRequestEvage: 27.776
            Epoch: 1634556915
            VinsResponseSec: 28.33
            FirstAsrResultSec: 22.934
            VinsSessionLoadEndEvage: 22.576
            EndOfUtteranceSec: 27.776
            VinsWaitAfterEouDurationSec: 0.549
            FinishVinsRequestEou: 28.325
            StartVinsRequestEou: 27.776
        }
        Tts {
            FirstTtsChunkSec: 28.335
            UsefulResponseForUserEvage: 28.335
        }
    }
}
)";

Y_UNIT_TEST_SUITE(Wonderlogs) {
    Y_UNIT_TEST(WonderlogsMaker) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto uniproxyPrepared = CreateRandomTable(client, directory, "uniproxy-prepared");
        const auto megamindPrepared = CreateRandomTable(client, directory, "megamind-prepared");
        const auto asrPrepared = CreateRandomTable(client, directory, "asr-prepared");
        const auto wonderlogsActual = CreateRandomTable(client, directory, "wonderlogs");
        const auto privateWonderlogsActual = CreateRandomTable(client, directory, "private-wonderlogs");
        const auto robotWonderlogsActual = CreateRandomTable(client, directory, "robot-wonderlogs");
        const auto wonderlogsErrorActual = CreateRandomTable(client, directory, "wonderlogs-error");

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
            for (auto it = rows.begin(); it != rows.end() && std::next(it) != rows.end(); ++it) {
                const auto nextIt = std::next(it);
                UNIT_ASSERT(!(it->GetUuid() == nextIt->GetUuid() && it->GetMessageId() == nextIt->GetMessageId()));
            }
            auto writer = client->CreateTableWriter<TUniproxyPrepared>(
                NYT::TRichYPath(uniproxyPrepared)
                    .Schema(NYT::CreateTableSchema<TUniproxyPrepared>({"uuid", "message_id"})));

            for (const auto& row : rows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        {
            TVector<TMegamindPrepared> rows;
            const auto rowsData = NResource::Find("megamind_prepared.jsonlines");
            for (const TStringBuf megamindPreparedJson : StringSplitter(rowsData).Split('\n')) {
                TMegamindPrepared megamindPreparedRow;
                // TODO(ran1s) MEGAMIND-3467
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = true;
                UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString(megamindPreparedJson),
                                                                        &megamindPreparedRow, options)
                                .ok());
                rows.push_back(megamindPreparedRow);
            }
            Sort(rows, comparator);
            for (auto it = rows.begin(); it != rows.end() && std::next(it) != rows.end(); ++it) {
                const auto nextIt = std::next(it);
                UNIT_ASSERT(!(it->GetUuid() == nextIt->GetUuid() && it->GetMessageId() == nextIt->GetMessageId()));
            }
            auto writer = client->CreateTableWriter<TMegamindPrepared>(
                NYT::TRichYPath(megamindPrepared)
                    .Schema(NYT::CreateTableSchema<TMegamindPrepared>({"uuid", "message_id"})));

            for (const auto& row : rows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        {
            TVector<TAsrPrepared> rows;
            const auto rowsData = NResource::Find("asr_prepared.jsonlines");
            for (const TStringBuf asrPreparedJson : StringSplitter(rowsData).Split('\n')) {
                TAsrPrepared asrPreparedRow;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(asrPreparedJson), &asrPreparedRow).ok());
                rows.push_back(asrPreparedRow);
            }
            Sort(rows, comparator);
            for (auto it = rows.begin(); it != rows.end() && std::next(it) != rows.end(); ++it) {
                const auto nextIt = std::next(it);
                UNIT_ASSERT(!(it->GetUuid() == nextIt->GetUuid() && it->GetMessageId() == nextIt->GetMessageId()));
            }
            auto writer = client->CreateTableWriter<TAsrPrepared>(
                NYT::TRichYPath(asrPrepared).Schema(NYT::CreateTableSchema<TAsrPrepared>({"uuid", "message_id"})));

            for (const auto& row : rows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }

        {
            const auto timestampFrom = ParseDatetime("2021-08-29T00:00:00+03:00");
            const auto timestampTo = ParseDatetime("2021-08-30T00:00:00+03:00");
            TEnvironment environment{.UniproxyQloudProjects{"voice-ext", "alice", "unknown"},
                                     .UniproxyQloudApplications{"uniproxy"},
                                     .MegamindEnvironments{"stable", "megamind_standalone_man",
                                                           "megamind_standalone_vla", "megamind_standalone_sas"}};
            MakeWonderlogs(client, directory + "/tmp/", uniproxyPrepared, megamindPrepared, asrPrepared,
                           wonderlogsActual, privateWonderlogsActual, robotWonderlogsActual, wonderlogsErrorActual,
                           *timestampFrom, *timestampTo, /* requestsShift= */ TDuration::Minutes(10), environment,
                           JoinFsPaths(GetWorkPath(), "geodata6.bin"));
        }

        for (const auto& table : {wonderlogsActual, privateWonderlogsActual, robotWonderlogsActual}) {
            UNIT_ASSERT_EQUAL("brotli_8", client->Get(table + "/@compression_codec").AsString());
            UNIT_ASSERT_EQUAL("lrc_12_2_2", client->Get(table + "/@erasure_codec").AsString());
            UNIT_ASSERT_EQUAL("scan", client->Get(table + "/@optimize_for").AsString());

            const THashMap<TString, int> sortColumns{{"_uuid", 0}, {"_message_id", 1}};
            int order = 0;
            NYT::TTableSchema schema;
            NYT::Deserialize(schema, client->Get(table + "/@schema"));
            TStringBuilder sortColumnsActual;
            for (const auto& col : schema.Columns()) {
                sortColumnsActual << col.Name() << ", ";
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

            UNIT_ASSERT_EQUAL_C(2, order, sortColumnsActual);
        }

        UNIT_ASSERT(!client->Exists(wonderlogsActual + "/@expiration_time"));
        UNIT_ASSERT(!client->Exists(privateWonderlogsActual + "/@expiration_time"));

        {
            const auto expirationTime =
                ParseDatetime(client->Get(wonderlogsErrorActual + "/@expiration_time").AsString());
            UNIT_ASSERT(expirationTime);
            UNIT_ASSERT(expirationTime->MilliSeconds() >
                        (TInstant::Now() + MONTH_TTL - TDuration::Days(1)).MilliSeconds());
            UNIT_ASSERT(expirationTime->MilliSeconds() <
                        (TInstant::Now() + MONTH_TTL + TDuration::Days(1)).MilliSeconds());
        }

        for (const auto& [resourceName, wonderlogsTable] :
             {std::make_pair("wonderlogs.jsonlines", wonderlogsActual),
              std::make_pair("private_wonderlogs.jsonlines", privateWonderlogsActual),
              std::make_pair("robot_wonderlogs.jsonlines", robotWonderlogsActual)}) {
            TVector<TWonderlog> wonderlogsExpectedRows;
            {
                const auto WonderlogRows = NResource::Find(resourceName);
                TVector<TStringBuf> wonderlogsExpectedStr = StringSplitter(WonderlogRows).Split('\n');
                for (const auto wonderlogExpectedStr : wonderlogsExpectedStr) {
                    TWonderlog wonderlog;
                    // TODO(ran1s) MEGAMIND-3467
                    google::protobuf::util::JsonParseOptions options;
                    options.ignore_unknown_fields = true;
                    UNIT_ASSERT(
                        google::protobuf::util::JsonStringToMessage(TString(wonderlogExpectedStr), &wonderlog, options)
                            .ok());
                    wonderlogsExpectedRows.push_back(std::move(wonderlog));
                }
            }
            TVector<TWonderlog> wonderlogsActualRows;
            {
                for (auto readerActual = client->CreateTableReader<TWonderlog>(wonderlogsTable);
                     readerActual->IsValid(); readerActual->Next()) {
                    wonderlogsActualRows.push_back(readerActual->GetRow());
                }
            }

            UNIT_ASSERT_EQUAL(wonderlogsExpectedRows.size(), wonderlogsActualRows.size());

            Sort(wonderlogsExpectedRows, comparator);
            Sort(wonderlogsActualRows, comparator);

            for (const auto& rows : {wonderlogsExpectedRows, wonderlogsActualRows}) {
                for (auto it = rows.begin(); it != rows.end() && std::next(it) != rows.end(); ++it) {
                    const auto nextIt = std::next(it);
                    UNIT_ASSERT(!(it->GetUuid() == nextIt->GetUuid() && it->GetMessageId() == nextIt->GetMessageId()));
                }
            }

            {
                for (size_t i = 0; i < wonderlogsExpectedRows.size(); i++) {
                    auto expected = wonderlogsExpectedRows[i];
                    auto actual = wonderlogsActualRows[i];

                    UNIT_ASSERT_MESSAGES_EQUAL(expected.GetPresence(), actual.GetPresence());
                    if (!expected.GetPresence().GetUniproxy() && expected.GetPresence().GetMegamind()) {
                        expected.MutableSpeechkitRequest()->MutableHeader()->SetSessionId("");
                        actual.MutableSpeechkitRequest()->MutableHeader()->SetSessionId("");
                        expected.MutableSpeechkitResponse()->MutableHeader()->SetSessionId("");
                        actual.MutableSpeechkitResponse()->MutableHeader()->SetSessionId("");
                    }

                    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
                }
            }
        }
        const auto wonderlogsErrorRows = NResource::Find("wonderlogs_error.jsonlines");
        TVector<TStringBuf> wonderlogsErrorSortedExpected = StringSplitter(wonderlogsErrorRows).Split('\n');
        while (!wonderlogsErrorSortedExpected.empty() && wonderlogsErrorSortedExpected.back().empty()) {
            wonderlogsErrorSortedExpected.pop_back();
        }

        TVector<TWonderlog::TError> wonderlogsErrorSortedActual;
        for (auto readerActual = client->CreateTableReader<TWonderlog::TError>(wonderlogsErrorActual);
             readerActual->IsValid(); readerActual->Next()) {
            wonderlogsErrorSortedActual.push_back(readerActual->GetRow());
        }
        UNIT_ASSERT_EQUAL(wonderlogsErrorSortedExpected.size(), wonderlogsErrorSortedActual.size());
        Sort(wonderlogsErrorSortedActual, [](const TWonderlog::TError& lhs, const TWonderlog::TError& rhs) {
            if (lhs.GetUuid() != rhs.GetUuid()) {
                return lhs.GetUuid() < rhs.GetUuid();
            }
            return lhs.GetMessageId() < rhs.GetMessageId();
        });
        for (size_t i = 0; i < wonderlogsErrorSortedExpected.size(); i++) {
            TWonderlog::TError& actual = wonderlogsErrorSortedActual[i];
            TWonderlog::TError expected;
            UNIT_ASSERT(
                google::protobuf::util::JsonStringToMessage(TString(wonderlogsErrorSortedExpected[i]), &expected)
                    .ok());

            UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(PrivacyFlags) {
        {
            TWonderlog::TPrivacy privacy;
            UNIT_ASSERT(!NImpl::DoCensor(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInRequest(true);
            UNIT_ASSERT(NImpl::DoCensor(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInResponse(true);
            UNIT_ASSERT(NImpl::DoCensor(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.SetOriginalDoNotUseUserLogs(true);
            UNIT_ASSERT(NImpl::DoCensor(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableGeoRestrictions()->SetProhibitedByRegion(true);
            UNIT_ASSERT(NImpl::DoCensor(privacy));
        }
        const NAlice::TCensor::TFlags publicFlags;
        const auto privateRequestFlags = NAlice::TCensor::TFlags{NAlice::EAccess::A_PRIVATE_REQUEST};
        const auto privateResponseFlags = NAlice::TCensor::TFlags{NAlice::EAccess::A_PRIVATE_RESPONSE};
        const auto privateRequestResponseFlags =
            NAlice::TCensor::TFlags{NAlice::EAccess::A_PRIVATE_REQUEST} | NAlice::EAccess::A_PRIVATE_RESPONSE;
        {
            TWonderlog::TPrivacy privacy;
            UNIT_ASSERT_EQUAL(publicFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInRequest(true);
            UNIT_ASSERT_EQUAL(privateRequestFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInResponse(true);
            UNIT_ASSERT_EQUAL(privateResponseFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInRequest(true);
            privacy.MutableContentProperties()->SetContainsSensitiveDataInResponse(true);
            UNIT_ASSERT_EQUAL(privateRequestResponseFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.SetOriginalDoNotUseUserLogs(true);
            UNIT_ASSERT_EQUAL(privateRequestResponseFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInResponse(true);
            privacy.SetOriginalDoNotUseUserLogs(true);
            UNIT_ASSERT_EQUAL(privateRequestResponseFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInRequest(true);
            privacy.SetOriginalDoNotUseUserLogs(true);
            UNIT_ASSERT_EQUAL(privateRequestResponseFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInRequest(true);
            privacy.MutableContentProperties()->SetContainsSensitiveDataInResponse(true);
            privacy.SetOriginalDoNotUseUserLogs(true);
            UNIT_ASSERT_EQUAL(privateRequestResponseFlags, NImpl::CensorFlags(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableGeoRestrictions()->SetProhibitedByRegion(true);
            UNIT_ASSERT_EQUAL(privateRequestResponseFlags, NImpl::CensorFlags(privacy));
        }
    }

    Y_UNIT_TEST(DoNotUseUserLogs) {
        {
            TWonderlog::TPrivacy privacy;
            UNIT_ASSERT(!NImpl::GetDoNotUseUserLogs(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInRequest(true);
            UNIT_ASSERT(NImpl::GetDoNotUseUserLogs(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableContentProperties()->SetContainsSensitiveDataInResponse(true);
            UNIT_ASSERT(NImpl::GetDoNotUseUserLogs(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.SetOriginalDoNotUseUserLogs(true);
            UNIT_ASSERT(NImpl::GetDoNotUseUserLogs(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.MutableGeoRestrictions()->SetProhibitedByRegion(true);
            UNIT_ASSERT(NImpl::GetDoNotUseUserLogs(privacy));
        }
        {
            TWonderlog::TPrivacy privacy;
            privacy.SetProhibitedByGdpr(true);
            UNIT_ASSERT(NImpl::GetDoNotUseUserLogs(privacy));
        }
    }

    Y_UNIT_TEST(CensorWonderlogs) {
        auto client = NYT::NTesting::CreateTestClient();
        const auto directory = NYT::NTesting::CreateTestDirectory(client);
        const auto wonderlogs1 = CreateRandomTable(client, directory, "wonderlogs1");
        const auto wonderlogs2 = CreateRandomTable(client, directory, "wonderlogs2");
        const auto privateUsers = CreateRandomTable(client, directory, "private-users");
        const auto actualCensoredWonderlogs1 = CreateRandomTable(client, directory, "censored-wonderlogs1");
        const auto actualCensoredWonderlogs2 = CreateRandomTable(client, directory, "censored-wonderlogs2");

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

        for (const auto& [table, resource, comparator, sortColumns] :
             {std::tie(wonderlogs1, "wonderlogs1.jsonlines", comparator1, sortColumns1),
              std::tie(wonderlogs2, "wonderlogs2.jsonlines", comparator2, sortColumns2)}) {
            TVector<TWonderlog> rows;
            const auto rowsData = NResource::Find(resource);
            for (const TStringBuf wonderlogJson : StringSplitter(rowsData).Split('\n')) {
                TWonderlog wonderlogRow;
                // TODO(ran1s) MEGAMIND-3467
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = true;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(wonderlogJson), &wonderlogRow, options).ok());
                rows.push_back(wonderlogRow);
            }
            Sort(rows, comparator);
            auto writer = client->CreateTableWriter<TWonderlog>(
                NYT::TRichYPath(table).Schema(NYT::CreateTableSchema<TWonderlog>(sortColumns)));

            for (const auto& row : rows) {
                writer->AddRow(row);
            }
            writer->Finish();
        }
        {
            auto writer =
                client->CreateTableWriter<TPrivateUser>(privateUsers, NYT::TTableWriterOptions().InferSchema(true));
            const auto rows = NResource::Find("private_users.jsonlines");
            for (const TStringBuf privateUserJson : StringSplitter(rows).Split('\n')) {
                TPrivateUser privateUserRow;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(privateUserJson), &privateUserRow).ok());
                writer->AddRow(privateUserRow);
            }
            writer->Finish();
        }
        CensorWonderlogs(client, directory, {wonderlogs1, wonderlogs2},
                         {actualCensoredWonderlogs1, actualCensoredWonderlogs2}, privateUsers, /* threadCount= */ 2);

        for (const auto& [oldWonderlogs, newWonderlogs] :
             {std::tie(wonderlogs1, actualCensoredWonderlogs1), std::tie(wonderlogs2, actualCensoredWonderlogs2)}) {
            UNIT_ASSERT_EQUAL(client->Get(oldWonderlogs + "/@schema"), client->Get(newWonderlogs + "/@schema"));
        }

        for (const auto& [table, sortColumns] :
             {std::tie(actualCensoredWonderlogs1, sortColumns1), std::tie(actualCensoredWonderlogs2, sortColumns2)}) {
            UNIT_ASSERT_EQUAL("brotli_8", client->Get(table + "/@compression_codec").AsString());
            UNIT_ASSERT_EQUAL("lrc_12_2_2", client->Get(table + "/@erasure_codec").AsString());
            UNIT_ASSERT_EQUAL("scan", client->Get(table + "/@optimize_for").AsString());

            THashMap<TString, size_t> sortColumnsMap;
            for(size_t i = 0; i < sortColumns.Parts_.size(); ++i) {
                sortColumnsMap[sortColumns.Parts_[i].Name()] = i;
            }
            size_t order = 0;
            NYT::TTableSchema schema;
            NYT::Deserialize(schema, client->Get(table + "/@schema"));
            for (const auto& col : schema.Columns()) {
                const auto* expectedOrder = sortColumnsMap.FindPtr(col.Name());
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
            UNIT_ASSERT_EQUAL(sortColumnsMap.size(), order);
        }

        for (const auto& [actualCensoredWonderlogs, expectedCensoreWonderlogs] :
             {std::tie(actualCensoredWonderlogs1, "censored_wonderlogs1.jsonlines"),
              std::tie(actualCensoredWonderlogs2, "censored_wonderlogs2.jsonlines")}) {
            TVector<TWonderlog> wonderlogsActualRows, wonderlogsExpectedRows;
            for (auto readerActual = client->CreateTableReader<TWonderlog>(actualCensoredWonderlogs);
                 readerActual->IsValid(); readerActual->Next()) {
                wonderlogsActualRows.push_back(readerActual->GetRow());
            }

            for (const auto wonderlogExpectedStr :
                 StringSplitter(NResource::Find(expectedCensoreWonderlogs)).Split('\n')) {
                TWonderlog wonderlog;
                // TODO(ran1s) MEGAMIND-3467
                google::protobuf::util::JsonParseOptions options;
                options.ignore_unknown_fields = true;
                UNIT_ASSERT(
                    google::protobuf::util::JsonStringToMessage(TString(wonderlogExpectedStr), &wonderlog, options)
                        .ok());
                wonderlogsExpectedRows.push_back(std::move(wonderlog));
            }

            UNIT_ASSERT_EQUAL_C(wonderlogsExpectedRows.size(), wonderlogsActualRows.size(),
                                TStringBuilder{} << wonderlogsExpectedRows.size() << " "
                                                 << wonderlogsActualRows.size());

            const auto comparator = [](const TWonderlog& lhs, const TWonderlog& rhs) {
                if (lhs.GetUuid() != rhs.GetUuid()) {
                    return lhs.GetUuid() < rhs.GetUuid();
                }
                return lhs.GetMessageId() < rhs.GetMessageId();
            };

            Sort(wonderlogsExpectedRows, comparator);
            Sort(wonderlogsActualRows, comparator);

            for (const auto& rows : {wonderlogsExpectedRows, wonderlogsActualRows}) {
                for (auto it = rows.begin(); it != rows.end() && std::next(it) != rows.end(); ++it) {
                    const auto nextIt = std::next(it);
                    UNIT_ASSERT(!(it->GetUuid() == nextIt->GetUuid() && it->GetMessageId() == nextIt->GetMessageId()));
                }
            }

            {
                for (size_t i = 0; i < wonderlogsExpectedRows.size(); i++) {
                    auto expected = wonderlogsExpectedRows[i];
                    auto actual = wonderlogsActualRows[i];

                    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
                }
            }
        }
    }

    Y_UNIT_TEST(ChangeSchema) {
        const TString DEVICE_STATE = "device_state";
        const TVector<TString> PATH = {"speechkit_request", "request", DEVICE_STATE};
        const auto schema = NImpl::ChangeSchema(NYT::CreateTableSchema<TWonderlog>(), {PATH});
        bool containsDeviceState = false;
        NTi::TTypePtr type;
        for (const auto& col : schema.Columns()) {
            containsDeviceState |= DEVICE_STATE == col.Name();
            if (PATH[0] == col.Name()) {
                type = col.TypeV3();
            }
        }
        UNIT_ASSERT(containsDeviceState);
        UNIT_ASSERT(type);
        for (size_t i = 1; i + 1 < PATH.size(); ++i) {
            UNIT_ASSERT(type->IsOptional());
            UNIT_ASSERT(type->StripOptionals()->IsStruct());
            type = type->StripOptionals()->AsStruct()->GetMember(PATH[i]).GetType();
            UNIT_ASSERT(type);
        }
        UNIT_ASSERT(type->IsOptional());
        UNIT_ASSERT(type->StripOptionals()->IsStruct());
        UNIT_ASSERT(!type->StripOptionals()->AsStruct()->HasMember(DEVICE_STATE));
    }

    Y_UNIT_TEST(MoveToColumns) {
        NYT::TNode wonderlog;
        wonderlog["speechkit_request"]["request"]["device_state"] = 1337;
        const auto newWonderlog = NImpl::MoveToColumns(wonderlog, {{"speechkit_request", "request", "device_state"}});
        UNIT_ASSERT(newWonderlog.AsMap());
        UNIT_ASSERT(newWonderlog.HasKey("device_state"));
        UNIT_ASSERT(!newWonderlog["speechkit_request"]["request"].HasKey("device_state"));
        UNIT_ASSERT(newWonderlog["device_state"].IsInt64());
        UNIT_ASSERT_EQUAL(1337, newWonderlog["device_state"].AsInt64());
    }

    Y_UNIT_TEST(GetRestrictions) {
        const auto geobase = NGeobase::TLookup(JoinFsPaths(GetWorkPath(), "geodata6.bin"));
        {
            const auto region = NImpl::GetRegion(&geobase, "77.88.55.77");
            UNIT_ASSERT_EQUAL(TWonderlog::TPrivacy::TGeoRestrictions::R_COUNTRY_RUSSIA, region);
            UNIT_ASSERT(!NImpl::ProhibitedRegion(region));
        }
        {
            const auto region = NImpl::GetRegion(&geobase, "192.118.64.180");
            UNIT_ASSERT_EQUAL(TWonderlog::TPrivacy::TGeoRestrictions::R_COUNTRY_ISRAEL, region);
            UNIT_ASSERT(NImpl::ProhibitedRegion(region));
        }
    }

    Y_UNIT_TEST(Environment) {
        TEnvironment environment{.UniproxyQloudProjects{"voice-ext", "alice", "unknown"},
                                 .UniproxyQloudApplications{"uniproxy"},
                                 .MegamindEnvironments{"stable", "megamind_standalone_man", "megamind_standalone_vla",
                                                       "megamind_standalone_sas"}};
        UNIT_ASSERT(environment.SuitableEnvironment(/* uniproxyQloudProject= */ "alice",
                                                    /* uniproxyQloudApplication= */ "uniproxy",
                                                    /* megamindEnvironment= */ "megamind_standalone_man"));
        UNIT_ASSERT(environment.SuitableEnvironment(/* uniproxyQloudProject= */ "alice",
                                                    /* uniproxyQloudApplication= */ "uniproxy",
                                                    /* megamindEnvironment= */ {}));

        UNIT_ASSERT(environment.SuitableEnvironment(/* uniproxyQloudProject= */ {},
                                                    /* uniproxyQloudApplication= */ {},
                                                    /* megamindEnvironment= */ {}));

        UNIT_ASSERT(environment.SuitableEnvironment(/* uniproxyQloudProject= */ {},
                                                    /* uniproxyQloudApplication= */ {},
                                                    /* megamindEnvironment= */ "megamind_standalone_man"));

        UNIT_ASSERT(!environment.SuitableEnvironment(/* uniproxyQloudProject= */ "alice",
                                                     /* uniproxyQloudApplication= */ "uniproxy",
                                                     /* megamindEnvironment= */ "megamind_hamster"));

        UNIT_ASSERT(!environment.SuitableEnvironment(/* uniproxyQloudProject= */ "lol",
                                                     /* uniproxyQloudApplication= */ "uniproxy",
                                                     /* megamindEnvironment= */ "megamind_standalone_man"));

        UNIT_ASSERT(!environment.SuitableEnvironment(/* uniproxyQloudProject= */ "alice",
                                                     /* uniproxyQloudApplication= */ "kek",
                                                     /* megamindEnvironment= */ "megamind_standalone_man"));
    }

    Y_UNIT_TEST(PrivateWonderlog) {
        TWonderlog actual, expected;
        google::protobuf::TextFormat::ParseFromString(WONDERLOG, &actual);
        google::protobuf::TextFormat::ParseFromString(CENSORED_WONDERLOG, &expected);

        NAlice::TCensor censor;
        censor.ProcessMessage(
            NAlice::TCensor::TFlags{NAlice::EAccess::A_PRIVATE_REQUEST} | NAlice::EAccess::A_PRIVATE_RESPONSE, actual);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }
}
