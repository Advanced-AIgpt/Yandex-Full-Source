#include "converter.h"

#include <alice/library/unittest/message_diff.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(ConverterTest) {

Y_UNIT_TEST(General) {
    NSc::TValue requestBody = NSc::TValue::FromJson(R"(
    {
        "clientIp": "46.32.67.228",
        "contentSettings": "safe",
        "currentlyPlaying": "",
        "deviceId": "FFB8F0066C13DBA0D33DEE40",
        "experiments": {
            "aliceExperiments": "music_exp__control2=1;music_exp__dj_testid@491965=1;music_exp__dj_testid@497397=1;music_exp__control=1;",
            "isSmartSpeaker": true,
            "mediumRuExplicitContent": true,
            "morningShowConfigId": 1000,
            "sessions": true,
            "ugcEnabled": true
        },
        "filters": null,
        "isGeneral": true,
        "location": {
            "lat": 59.89962387,
            "lon": 29.81730461
        },
        "requestId": "dfbc3eeb-4376-43bf-b61d-150189a0ce1d",
        "sessionId": "",
        "supportedMusicTypes": [],
        "timestamp": 1643696376922,
        "userClassificationAge": "child",
        "uuid": "9fbbeb8367830fe4b7105f8d595edf2c"
    }
    )");
    QuasarRequestConverter converter(requestBody);
    UNIT_ASSERT(converter.Convert());

    NAlice::TTypedSemanticFrame expectedTsf;
    auto& musicPlayTsf = *expectedTsf.MutableMusicPlaySemanticFrame();
    musicPlayTsf.MutableDisableNlg()->SetBoolValue(true);

    UNIT_ASSERT_MESSAGES_EQUAL(converter.GetTypedSemanticFrame(), expectedTsf);
}

Y_UNIT_TEST(Promo) {
    NSc::TValue requestBody = NSc::TValue::FromJson(R"(
    {
        "clientIp": "188.170.74.193",
        "contentSettings": "children",
        "currentlyPlaying": "",
        "deviceId": "FF98F0297565C8A910CD8590",
        "experiments": {
            "aliceExperiments": "music_exp__dj_testid@497399=1;music_exp__dj_mixing_catboost_version@pairlogit=1;",
            "isSmartSpeaker": true,
            "mediumRuExplicitContent": true,
            "morningShowConfigId": 1000,
            "sessions": true,
            "ugcEnabled": true
        },
        "filters": {
            "object": {
                "id": "4924870",
                "type": "album"
            }
        },
        "location": {
            "lat": 54.562996,
            "lon": 33.13221
        },
        "modifiers": {
            "shuffle": true
        },
        "requestId": "ef907167-ccb6-4415-8c78-d81a4a725750",
        "sessionId": "",
        "supportedMusicTypes": [],
        "textQuery": "Yet Another New Year",
        "timestamp": 1643703636727,
        "userClassificationAge": "child",
        "uuid": "ca2b4a940606bad43361f029b544e406"
    }
    )");

    QuasarRequestConverter converter(requestBody);
    UNIT_ASSERT(converter.Convert());

    NAlice::TTypedSemanticFrame expectedTsf;
    auto& musicPlayTsf = *expectedTsf.MutableMusicPlaySemanticFrame();
    musicPlayTsf.MutableDisableNlg()->SetBoolValue(true);
    musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Album);
    musicPlayTsf.MutableObjectId()->SetStringValue("4924870");
    musicPlayTsf.MutableOrder()->SetOrderValue("shuffle");

    UNIT_ASSERT_MESSAGES_EQUAL(converter.GetTypedSemanticFrame(), expectedTsf);
}

Y_UNIT_TEST(BassAction) {
    NSc::TValue requestBody = NSc::TValue::FromJson(R"(
    {
        "clientIp": "95.221.18.17",
        "contentSettings": "without",
        "currentlyPlaying": "",
        "deviceId": "FF98F029D98C9ADF75AD24BF",
        "experiments": {
            "aliceExperiments": "music_exp__dj_program@alice_reverse_experiment=1;music_exp__dj_testid@490652=1;",
            "isSmartSpeaker": true,
            "mediumRuExplicitContent": true,
            "sessions": true,
            "ugcEnabled": true
        },
        "filters": {
            "isPopular": false,
            "object": {
            "id": "39257277",
            "type": "track"
            }
        },
        "requestId": "0e05d56d-4a54-43e8-8a0c-913bd969dd42",
        "sessionId": "",
        "showFirstTrack": true,
        "supportedMusicTypes": [],
        "timestamp": 1643707804546,
        "uuid": "4e6ddb75ca4ada3d9bb535616e5ed34d"
    }
    )");
    QuasarRequestConverter converter(requestBody);

    UNIT_ASSERT(converter.Convert());

    const TString sampleAlarmId = "sample_alarm_id";
    converter.SetAlarmId(sampleAlarmId);

    NAlice::TTypedSemanticFrame expectedTsf;
    auto& musicPlayTsf = *expectedTsf.MutableMusicPlaySemanticFrame();
    musicPlayTsf.MutableDisableNlg()->SetBoolValue(true);
    musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Track);
    musicPlayTsf.MutableObjectId()->SetStringValue("39257277");
    musicPlayTsf.MutableAlarmId()->SetStringValue(sampleAlarmId);

    UNIT_ASSERT_MESSAGES_EQUAL(converter.GetTypedSemanticFrame(), expectedTsf);
}

Y_UNIT_TEST(FairyTale) {
    NSc::TValue requestBody = NSc::TValue::FromJson(R"(
    {
        "clientIp": "178.218.98.177",
        "contentSettings": "medium",
        "currentlyPlaying": "67782334",
        "deviceId": "FF98F0294B94087F9F452408",
        "experiments": {
            "aliceExperiments": "music_exp__dj_testid@497398=1;music_exp__dj_merlin_boost@0=1;music_exp__dj_replace_catboost_90_seconds_alice.bin@catboost_90_seconds_new_features.bin=1;music_exp__dj_testid@491966=1;",
            "isSmartSpeaker": true,
            "mediumRuExplicitContent": true,
            "sessions": true,
            "ugcEnabled": true
        },
        "filters": {
            "isNew": null,
            "isPopular": null,
            "object": {
                "desiredAnswerType": null,
                "id": "2557930",
                "isSimilar": null,
                "type": "artist"
            }
        },
        "location": {
            "lat": 50.54515076,
            "lon": 136.9691772
        },
        "requestId": "20270b2e-fe11-4286-a3d9-815fb5e5b79e",
        "sessionId": "F8ZxJAQS",
        "showFirstTrack": true,
        "supportedMusicTypes": [],
        "textQuery": "сказка чуковского",
        "timestamp": 1643701290853,
        "userClassificationAge": "adult",
        "uuid": "6957269e3e0cd1e5b3333cfdf2248abd"
    }
    )");
    QuasarRequestConverter converter(requestBody);
    UNIT_ASSERT(converter.Convert());

    NAlice::TTypedSemanticFrame expectedTsf;
    auto& musicPlayTsf = *expectedTsf.MutableMusicPlaySemanticFrame();
    musicPlayTsf.MutableDisableNlg()->SetBoolValue(true);
    musicPlayTsf.MutableObjectType()->SetEnumValue(NAlice::TMusicPlayObjectTypeSlot::Artist);
    musicPlayTsf.MutableObjectId()->SetStringValue("2557930");

    UNIT_ASSERT_MESSAGES_EQUAL(converter.GetTypedSemanticFrame(), expectedTsf);
}

Y_UNIT_TEST(Genre) {
    NSc::TValue requestBody = NSc::TValue::FromJson(R"(
    {
        "clientIp": "95.221.18.17",
        "contentSettings": "without",
        "currentlyPlaying": "",
        "deviceId": "FF98F029D98C9ADF75AD24BF",
        "experiments": {
            "aliceExperiments": "music_exp__dj_program@alice_reverse_experiment=1;music_exp__dj_testid@490652=1;",
            "isSmartSpeaker": true,
            "mediumRuExplicitContent": true,
            "sessions": true,
            "ugcEnabled": true
        },
        "filters": {
            "genre": [
                "rock"
                ]
            }
        },
        "requestId": "0e05d56d-4a54-43e8-8a0c-913bd969dd42",
        "sessionId": "",
        "showFirstTrack": true,
        "supportedMusicTypes": [],
        "timestamp": 1643707804546,
        "uuid": "4e6ddb75ca4ada3d9bb535616e5ed34d"
    }
    )");
    QuasarRequestConverter converter(requestBody);
    UNIT_ASSERT(!converter.Convert());
}

Y_UNIT_TEST(Nothing) {
    NSc::TValue requestBody = NSc::TValue::FromJson(R"(
    {
        "clientIp": "95.221.18.17",
        "contentSettings": "without",
        "currentlyPlaying": "",
        "deviceId": "FF98F029D98C9ADF75AD24BF",
        "experiments": {
            "aliceExperiments": "music_exp__dj_program@alice_reverse_experiment=1;music_exp__dj_testid@490652=1;",
            "isSmartSpeaker": true,
            "mediumRuExplicitContent": true,
            "sessions": true,
            "ugcEnabled": true
        },
        "requestId": "0e05d56d-4a54-43e8-8a0c-913bd969dd42",
        "sessionId": "",
        "showFirstTrack": true,
        "supportedMusicTypes": [],
        "timestamp": 1643707804546,
        "uuid": "4e6ddb75ca4ada3d9bb535616e5ed34d"
    }
    )");
    QuasarRequestConverter converter(requestBody);
    UNIT_ASSERT(!converter.Convert());
}

}
