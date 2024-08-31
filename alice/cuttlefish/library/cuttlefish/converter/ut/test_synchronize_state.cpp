#include <alice/cuttlefish/library/cuttlefish/converter/ut/common.h>
#include <alice/cuttlefish/library/cuttlefish/converter/converters.h>
#include <google/protobuf/util/json_util.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices::NConverter;
using namespace NAliceProtocol;


Y_UNIT_TEST_SUITE(ParseSynchronizeState) {

Y_UNIT_TEST(Basic) {
    const TStringBuf rawJson = R"__({"event": {
        "header": {
        },
        "payload": {
            "uuid": "aaaaaaaA-Aaaa-aAAa-AaaA-AAAAAAAAAAAA",
            "device_manufacturer": "some-device-manufacturer",
            "yandexuid": "some-yandexuid",
            "device": "some-device",
            "auth_token": "some-auth-token",
            "oauth_token": "some-oauth-token",
            "vins": {
                "application": {
                    "device_manufacturer": "Some Device-Manufacturer",
                    "device_model": "Some-device MODEL",
                    "device_id": "some-device-id",
                    "os_version": "some-os-version",
                    "app_version": "some-app-version",
                    "app_id": "some-app-id",
                    "platform": "some-platform-info",
                    "uuid": "0123456789ABCDEF0123456789ABCDEF"
                }
            },
            "network_type": "some-network-type",
            "speechkitVersion": "some-speechkit-version",
            "volume": 95,
            "format": "some-voice-format",
            "speed": "1.5",
            "lang": "some-language",
            "emotion": "evil",
            "quality": "UltraHigh",
            "voice": "some-voice",
            "request": {
                "experiments": [
                    "first", "second", "third"
                ]
            },
            "service_name": "zazaza",
            "disable_local_experiments": true,
            "disable_utterance_logging": false,
            "user_agent": "my-lovely-user-agent",
            "uaas_tests": [
                1, "another", "one more"
            ],
            "accept_invalid_auth": true,
            "icookie": "the-most-encrypted-icookie",
            "wifi_networks": [
                {"mac":"24:7e:51:f2:41:44","signal_strength":-61},
                {"mac":"98:13:33:f9:04:da","signal_strength":-65},
                {"mac":"98:13:33:f9:59:40","signal_strength":-72}
            ],
            "supported_features": [
                "some_feature1",
                "some_feature2"
            ]
        }
    }})__";

    const TSynchronizeStateEvent event = JsonToProtobuf(GetSynchronizeStateEventConverter(), ReadJsonValue(rawJson));

    const TString expected = AsSortedJsonString(R"__({
        "AppToken": "some-auth-token",
        "UserAgent": "my-lovely-user-agent",
        "UaasTests": ["1", "another", "one more"],
        "ICookie": "the-most-encrypted-icookie",
        "ServiceName": "zazaza",
        "UserInfo": {
            "Uuid": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa",
            "Yuid": "some-yandexuid",
            "AuthToken":"some-oauth-token",
            "AuthTokenType":"OAUTH",
            "VinsApplicationUuid": "0123456789ABCDEF0123456789ABCDEF"
        },
        "UserOptions": {
            "DisableLocalExperiments": true,
            "DisableUtteranceLogging": false,
            "AcceptInvalidAuth": true
        },
        "DeviceInfo": {
            "Device":"some-device",
            "DeviceManufacturer": "some_device_manufacturer",
            "DeviceModel": "some_device_model",
            "DeviceId": "some-device-id",
            "Platform": "some-platform-info",
            "OsVersion": "some-os-version",
            "NetworkType": "some-network-type",
            "WifiNetworks": [
                {"Mac":"24:7e:51:f2:41:44","SignalStrength":-61},
                {"Mac":"98:13:33:f9:04:da","SignalStrength":-65},
                {"Mac":"98:13:33:f9:59:40","SignalStrength":-72}
            ],
            "SupportedFeatures": [
                "some_feature1",
                "some_feature2"
            ]
        },
        "ApplicationInfo": {
            "Id": "some-app-id",
            "Version": "some-app-version",
            "SpeechkitVersion": "some-speechkit-version"
        },
        "AudioOptions": {
            "Format": "some-voice-format"
        },
        "VoiceOptions": {
            "Volume": 95,
            "Speed": 1.5,
            "Lang": "some-language",
            "UnrestrictedEmotion": "evil",
            "Quality": "ULTRAHIGH",
            "Voice": "some-voice"
        },
        "Experiments": { "Storage": {
            "first": {"String": "1"},
            "second": {"String": "1"},
            "third": {"String": "1"}
        } }
    })__");
    const TString actual = AsSortedJsonString(event);

    Cerr << "EXPECTED: " << expected << "\n"
            "ACTUAL:   " << actual << Endl;
    UNIT_ASSERT_EQUAL(expected, actual);
}


Y_UNIT_TEST(InvalidFieldType) {
    const TStringBuf rawJson = R"__({"event": {
        "header": {
        },
        "payload": {
            "uuid": 12345
        }
    }})__";

    try {
        const TSynchronizeStateEvent event = JsonToProtobuf(GetSynchronizeStateEventConverter(), ReadJsonValue(rawJson));
    } catch (const yexception& err) {
        Cerr << "FAILED: " << err.what() << Endl;
    }
}

}  // Y_UNIT_TEST_SUITE(ParseSynchronizeState)
