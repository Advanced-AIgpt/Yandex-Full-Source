#include "bass_adapter.h"

#include <alice/hollywood/library/request/request.h>
#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood {

static const TString EPOCH = "1574930264";
static const TString TIMEZONE = "UTC+3";
static const TString UUID = "34D94983-65BD-49F2-B34E-BD123A7A9830";

template <typename TScenarioRequest>
TScenarioRequest CreateRequest() {
    TScenarioRequest request;

    auto& clientInfoProto = *request.MutableBaseRequest()->MutableClientInfo();
    clientInfoProto.SetEpoch(EPOCH);
    clientInfoProto.SetTimezone(TIMEZONE);
    clientInfoProto.SetUuid(UUID);

    return request;
}

Y_UNIT_TEST_SUITE(BassLikeMeta) {
    Y_UNIT_TEST(Smoke) {
        // constants
        const TString utterance = "Hello, my name is Inigo Montoya";
        const TString dialogId = "EACC0DF2-9078-4C45-BBBE-5A17EDF82F7C";
        const TString requestId = "E9EB0CC5-B5B0-49E7-8E0C-0C3A50361518";
        constexpr bool voiceSession = true;
        constexpr i64 rngSeed = 4;  // https://xkcd.com/221/
        constexpr i64 requestStartTimeSec = 1575317410;
        const TString userAgent = "YaBro";
        constexpr i64 filtrationLevel = 1;
        const TString clientIP = "::1";
        const TString cookie = "foo=bar";
        constexpr double screenScaleFactor = 1.0;
        constexpr i32 videoGalleryLimit = 42;
        // XXX(a-square): see MEGAMIND-816
        // const NSc::TValue personalData = NSc::TValue::FromJson("{\"hello\": \"there\"}");
        constexpr double lon = 55.7527;
        constexpr double lat = 37.6172;
        constexpr double accuracy = 5;
        constexpr i64 soundLevel = 6;
        const TString expKey = "hello";
        constexpr i64 expValue = 16373;
        const TString deviceId = "2A928510-2B41-4138-97B9-0C9636A44A08";
        const TString lang = "ru-RU";
        const TString appId = "Softinka";
        const TString appVersion = "66";
        const TString deviceManufacturer = "Yandex";
        const TString deviceModel = "Ushanka";
        const TString platform = "BolgenOS";
        const TString osVersion = "666";
        const TString clientId = "Softinka/66 (Yandex Ushanka; BolgenOS 666)";
        const TSet<TStringBuf> expectedFeatures = {
            "no_reliable_speakers",
            "no_bluetooth",
            "battery_power_state",
            "cec_available",
            "change_alarm_sound",
            "no_microphone",
            "music_player_allow_shots",
            "music_sdk_client",
        };
        const THashMap<TString, TString> expectedInstalledApps = {
            { "app1", "1" },
            { "app2", "qwerty" },
        };
        const auto deviceConfigContentSettings = NAlice::EContentSettings::children;
        const TString musicPlaylistOwner = "some_playlist_owner_string";
        const TString musicSessionId = "some_session_id";
        const TString musicCurrentlyPlayingTrackId = "some_track_id";
        const THashMap<TString, TString> musicCurrentlyPlayingTrackInfo = {
            {"title", "some_title"},
            {"type", "some_type"},
            // NOTE: TrackInfo may contain more complex values than just strings, e.g. arrays, objects etc.
        };
        constexpr bool forbidWebSearch = false;

        // fill the request
        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();

        request.MutableInput()->MutableVoice()->SetUtterance(utterance);

        auto& baseRequest = *request.MutableBaseRequest();
        baseRequest.SetDialogId(dialogId);
        baseRequest.SetRequestId(requestId);
        baseRequest.SetRandomSeed(rngSeed);
        baseRequest.SetServerTimeMs(requestStartTimeSec * 1000);

        auto& interfaces = *baseRequest.MutableInterfaces();
        interfaces.SetVoiceSession(voiceSession);
        interfaces.SetHasReliableSpeakers(false);
        interfaces.SetHasBluetooth(false);
        interfaces.SetHasAccessToBatteryPowerState(true);
        interfaces.SetHasCEC(true);
        interfaces.SetCanChangeAlarmSound(true);
        interfaces.SetHasMicrophone(false);
        interfaces.SetHasMusicPlayerShots(true);
        interfaces.SetHasMusicSdkClient(true);

        auto& location = *baseRequest.MutableLocation();
        location.SetLon(lon);
        location.SetLat(lat);
        location.SetAccuracy(accuracy);

        auto& deviceStateProto = *baseRequest.MutableDeviceState();
        deviceStateProto.SetSoundLevel(soundLevel);

        auto& installedApps = *deviceStateProto.MutableInstalledApps();
        for (const auto& appKeyVal : expectedInstalledApps) {
            installedApps[appKeyVal.first] = appKeyVal.second;
        }

        auto& deviceConfig = *deviceStateProto.MutableDeviceConfig();
        deviceConfig.SetContentSettings(deviceConfigContentSettings);

        auto& musicProto = *deviceStateProto.MutableMusic();
        musicProto.SetPlaylistOwner(musicPlaylistOwner);
        musicProto.SetSessionId(musicSessionId);

        auto& musicCurrentlyPlayingProto = *musicProto.MutableCurrentlyPlaying();
        musicCurrentlyPlayingProto.SetTrackId(musicCurrentlyPlayingTrackId);

        auto& trackInfoProto = *musicCurrentlyPlayingProto.MutableRawTrackInfo();
        for (const auto& kv : musicCurrentlyPlayingTrackInfo) {
            (*trackInfoProto.mutable_fields())[kv.first].set_string_value(kv.second);
        }

        auto& experimentsProto = *baseRequest.MutableExperiments();
        (*experimentsProto.mutable_fields())[expKey].set_number_value(expValue);

        auto& optionsProto = *baseRequest.MutableOptions();
        optionsProto.SetUserAgent(userAgent);
        optionsProto.SetFiltrationLevel(filtrationLevel);
        optionsProto.SetClientIP(clientIP);
        optionsProto.SetScreenScaleFactor(screenScaleFactor);
        optionsProto.SetVideoGalleryLimit(videoGalleryLimit);
        // XXX(a-square): see MEGAMIND-816
        // optionsProto.SetRawPersonalData(personalData.ToJson());

        auto& clientInfoProto = *baseRequest.MutableClientInfo();
        clientInfoProto.SetDeviceId(deviceId);
        clientInfoProto.SetLang(lang);
        clientInfoProto.SetAppId(appId);
        clientInfoProto.SetAppVersion(appVersion);
        clientInfoProto.SetDeviceManufacturer(deviceManufacturer);
        clientInfoProto.SetDeviceModel(deviceModel);
        clientInfoProto.SetPlatform(platform);
        clientInfoProto.SetOsVersion(osVersion);

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);

        // check the result
        const auto meta = NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */);
        UNIT_ASSERT_VALUES_EQUAL(false, meta->IsBanned().Get());
        UNIT_ASSERT_VALUES_EQUAL(false, meta->PureGC().Get());
        UNIT_ASSERT_VALUES_EQUAL(false, meta->HasImageSearchGranet().Get());

        UNIT_ASSERT_VALUES_EQUAL(utterance, meta->Utterance().Get());
        UNIT_ASSERT_VALUES_EQUAL(dialogId, meta->DialogId().Get());
        UNIT_ASSERT_VALUES_EQUAL(requestId, meta->RequestId().Get());
        UNIT_ASSERT_VALUES_EQUAL(lon, meta->Location()->Lon().Get());
        UNIT_ASSERT_VALUES_EQUAL(lat, meta->Location()->Lat().Get());
        UNIT_ASSERT_VALUES_EQUAL(accuracy, meta->Location()->Accuracy().Get());
        UNIT_ASSERT_VALUES_EQUAL(voiceSession, meta->VoiceSession().Get());
        UNIT_ASSERT_VALUES_EQUAL(soundLevel, meta->DeviceState()->SoundLevel().Get());

        const auto& actualInstalledAppsDict = meta->DeviceState()->InstalledApps()->GetDict();
        const THashMap<TString, TString> actualInstalledApps(actualInstalledAppsDict.begin(), actualInstalledAppsDict.end());
        UNIT_ASSERT_VALUES_EQUAL(expectedInstalledApps, actualInstalledApps);

        UNIT_ASSERT_VALUES_EQUAL(EContentSettings_descriptor()->FindValueByNumber(deviceConfigContentSettings)->name(),
                                 ToString(meta->DeviceState()->DeviceConfig()->ContentSettings()));

        UNIT_ASSERT_VALUES_EQUAL(musicPlaylistOwner, ToString(meta->DeviceState()->Music()->PlaylistOwner()));
        UNIT_ASSERT_VALUES_EQUAL(musicSessionId, ToString(meta->DeviceState()->Music()->SessionId()));
        UNIT_ASSERT_VALUES_EQUAL(musicCurrentlyPlayingTrackId, ToString(meta->DeviceState()->Music()->CurrentlyPlaying()->TrackId()));

        const auto& actualTrackInfoDict = meta->DeviceState()->Music()->CurrentlyPlaying()->TrackInfo()->GetDict();
        const THashMap<TString, TString> actualTrackInfo(actualTrackInfoDict.begin(), actualTrackInfoDict.end());
        UNIT_ASSERT_VALUES_EQUAL(musicCurrentlyPlayingTrackInfo, actualTrackInfo);

        UNIT_ASSERT_VALUES_EQUAL(ToString(rngSeed), meta->RngSeed().Get());
        UNIT_ASSERT_VALUES_EQUAL(1000000 * requestStartTimeSec, meta->RequestStartTime().Get());
        UNIT_ASSERT_VALUES_EQUAL(userAgent, meta->RawUserAgent().Get());
        UNIT_ASSERT_VALUES_EQUAL(filtrationLevel, meta->FiltrationLevel().Get());
        UNIT_ASSERT_VALUES_EQUAL(clientIP, meta->ClientIP().Get());
        UNIT_ASSERT_VALUES_EQUAL(screenScaleFactor, meta->ScreenScaleFactor().Get());
        UNIT_ASSERT_VALUES_EQUAL(videoGalleryLimit, meta->VideoGalleryLimit().Get());
        // XXX(a-square): see MEGAMIND-816
        // UNIT_ASSERT_VALUES_EQUAL(personalData, *meta->PersonalData().Get());
        UNIT_ASSERT_VALUES_EQUAL(UUID, meta->UUID().Get());
        UNIT_ASSERT_VALUES_EQUAL(deviceId, meta->DeviceId().Get());
        UNIT_ASSERT_VALUES_EQUAL(TIMEZONE, meta->TimeZone().Get());
        UNIT_ASSERT_VALUES_EQUAL(FromString<i64>(EPOCH), meta->Epoch().Get());
        UNIT_ASSERT_VALUES_EQUAL(clientId, meta->ClientId().Get());
        UNIT_ASSERT_VALUES_EQUAL(lang, meta->Lang().Get());
        UNIT_ASSERT_VALUES_EQUAL(forbidWebSearch, meta->ForbidWebSearch().Get());

        const auto supported = meta->ClientFeatures()->Supported();
        TSet<TStringBuf> actualFeatures(supported.begin(), supported.end());
        UNIT_ASSERT_VALUES_EQUAL(expectedFeatures, actualFeatures);

        const auto& experiments = meta->Experiments().AsDict<TStringBuf, i64>();
        const i64 actualExpValue = experiments[expKey].Get();
        UNIT_ASSERT_VALUES_EQUAL(expValue, actualExpValue);

        const auto clientInfo = meta->ClientInfo();
        UNIT_ASSERT_VALUES_EQUAL(appId, clientInfo->AppId().Get());
        UNIT_ASSERT_VALUES_EQUAL(appVersion, clientInfo->AppVersion().Get());
        UNIT_ASSERT_VALUES_EQUAL(deviceManufacturer, clientInfo->DeviceManufacturer().Get());
        UNIT_ASSERT_VALUES_EQUAL(deviceModel, clientInfo->DeviceModel().Get());
        UNIT_ASSERT_VALUES_EQUAL(platform, clientInfo->Platform().Get());
        UNIT_ASSERT_VALUES_EQUAL(osVersion, clientInfo->OsVersion().Get());
    }

    Y_UNIT_TEST(Validation) {
        NScenarios::TScenarioRunRequest request;
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);

        UNIT_ASSERT_EXCEPTION(NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */), TBassMetaValidationError);
    }

    Y_UNIT_TEST(TextUtterance) {
        static const TString utterance = "Hello, my name is Inigo Montoya";

        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();
        request.MutableInput()->MutableText()->SetUtterance(utterance);

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);
        const auto meta = NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */);
        UNIT_ASSERT_VALUES_EQUAL(utterance, meta->Utterance().Get());
    }

    Y_UNIT_TEST(VoiceUtterance) {
        static const TString utterance = "Hello, my name is Inigo Montoya";
        static const TString status = "okay";
        static constexpr double scoreNum = 0.8;
        static const TString userId = "alice";
        static const TString fooMode = "foo";
        static const TString barMode = "bar";
        static const TString classificationStatus = "classification_status";
        static const TString className = "jedi";
        static constexpr double classConfidence = 0.75;
        static const TString classTag = "force_user";

        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();
        auto& voice = *request.MutableInput()->MutableVoice();
        voice.SetUtterance(utterance);

        auto& biometryScoring = *voice.MutableBiometryScoring();
        biometryScoring.SetStatus(status);

        auto& score = *biometryScoring.AddScores();
        score.SetScore(scoreNum);
        score.SetUserId(userId);

        auto& foo = *biometryScoring.AddScoresWithMode();
        foo.SetMode(fooMode);
        auto& fooScore = *foo.AddScores();
        fooScore.SetScore(scoreNum);
        fooScore.SetUserId(userId);

        auto& bar = *biometryScoring.AddScoresWithMode();
        bar.SetMode(barMode);

        auto& biometryClassification = *voice.MutableBiometryClassification();
        biometryClassification.SetStatus(classificationStatus);

        auto& jedi = *biometryClassification.AddScores();
        jedi.SetClassName(className);
        jedi.SetConfidence(classConfidence);
        jedi.SetTag(classTag);

        auto& simpleJedi = *biometryClassification.AddSimple();
        simpleJedi.SetClassName(className);
        simpleJedi.SetTag(classTag);

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);
        const auto meta = NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */);
        const auto biometricsScores = meta->BiometricsScores();
        UNIT_ASSERT_VALUES_EQUAL(utterance, meta->Utterance().Get());
        UNIT_ASSERT_VALUES_EQUAL(status, biometricsScores->Status().Get());

        UNIT_ASSERT_VALUES_EQUAL(scoreNum, biometricsScores->Scores()[0]->Score().Get());
        UNIT_ASSERT_VALUES_EQUAL(userId, biometricsScores->Scores()[0]->UserId().Get());

        const auto fooScheme = biometricsScores->ScoresWithMode()[0];
        UNIT_ASSERT_VALUES_EQUAL(fooMode, fooScheme->Mode().Get());
        UNIT_ASSERT_VALUES_EQUAL(scoreNum, fooScheme->Scores()[0]->Score().Get());
        UNIT_ASSERT_VALUES_EQUAL(userId, fooScheme->Scores()[0]->UserId().Get());

        const auto barScheme = biometricsScores->ScoresWithMode()[1];
        UNIT_ASSERT_VALUES_EQUAL(barMode, barScheme->Mode().Get());
        UNIT_ASSERT(barScheme->Scores().Empty());

        const auto bioClassEventValue = (*meta->Event().GetRawValue())["biometry_classification"];
        UNIT_ASSERT_VALUES_EQUAL(classificationStatus, bioClassEventValue["status"].GetString());
        UNIT_ASSERT_VALUES_EQUAL(className, bioClassEventValue["scores"][0]["classname"].GetString());

        const auto bioClassMetaValue = meta->BiometryClassification();
        UNIT_ASSERT_VALUES_EQUAL(classificationStatus, bioClassMetaValue->Status().Get());
        UNIT_ASSERT_VALUES_EQUAL(classTag, bioClassMetaValue->Simple()[0]->Tag().Get());
        UNIT_ASSERT_VALUES_EQUAL(className, bioClassMetaValue->Simple()[0]->Classname().Get());
    }

    Y_UNIT_TEST(VoiceUtteranceAsrData) {
        const TString word1 = "four";
        const TString word2 = "wheels";
        const TString utterance = word1 + " " + word2;
        const TString asrUtterance = "4 wheels";

        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();
        auto& voice = *request.MutableInput()->MutableVoice();

        auto& asrData = *voice.AddAsrData();
        asrData.SetUtterance(asrUtterance);

        asrData.AddWords()->SetValue(word1);
        asrData.AddWords()->SetValue(word2);

        auto& expFields = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
        expFields[ToString(EXP_HW_ENABLE_BASS_ADAPTER_LEGACY_UTTERANCE)].set_string_value("");

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);
        const auto meta = NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */);
        UNIT_ASSERT_VALUES_EQUAL(utterance, meta->Utterance().Get());
        UNIT_ASSERT_VALUES_EQUAL(asrUtterance, meta->AsrUtterance().Get());
    }

    Y_UNIT_TEST(ImageSearch) {
        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);

        for (const bool imageSearch : {true, false}) {
            const auto meta = NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), imageSearch);
            UNIT_ASSERT_VALUES_EQUAL(imageSearch, meta->HasImageSearchGranet());
        }
    }

    Y_UNIT_TEST(RegionId) {
        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper(request, serviceCtx);

        const auto metaWithoutRegionId =
            NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */);
        UNIT_ASSERT(!metaWithoutRegionId->HasRegionId());

        ui32 regionId = 123;
        request.MutableBaseRequest()->MutableOptions()->SetUserDefinedRegionId(regionId);
        const auto metaWithRegionId =
            NImpl::CreateBassLikeMeta(wrapper, wrapper.Input(), false /* imageSearch */);
        UNIT_ASSERT_VALUES_EQUAL(regionId, metaWithRegionId->RegionId());
    }
}

Y_UNIT_TEST_SUITE(BassLikeForm) {
    Y_UNIT_TEST(Smoke) {
        static const TString formName = "hello";

        static const TString one = "one";
        static const TString oneType = "one_type";
        static const TString oneValue = "one_value";

        static const TString two = "two";
        static const TString twoType = "two_type";
        static const TString twoValue = "two_value";

        TFrame frame{formName};
        frame.AddSlot(TSlot{one, oneType, TSlot::TValue{oneValue}});
        frame.AddSlot(TSlot{two, twoType, TSlot::TValue{twoValue}});

        const auto bassForm = NImpl::CreateBassLikeForm(frame, /* sourceTextProvider= */ nullptr);

        UNIT_ASSERT_VALUES_EQUAL(formName, bassForm->Name().Get());

        for (const auto slot : bassForm->Slots()) {
            if (slot->Name().Get() == one) {
                UNIT_ASSERT_VALUES_EQUAL(oneType, slot->Type().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(oneValue), *slot->Value().Get());
                UNIT_ASSERT(slot->SourceText().IsNull());
            } else {
                UNIT_ASSERT_VALUES_EQUAL(two, slot->Name().Get());
                UNIT_ASSERT_VALUES_EQUAL(twoType, slot->Type().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(twoValue), *slot->Value().Get());
                UNIT_ASSERT(slot->SourceText().IsNull());
            }
        }
    }

    Y_UNIT_TEST(CheckIntType) {
        static const TString formName = "hello";

        static const TString first = "first";
        static const TString firstType = "int";
        static const auto firstValue = 100500;

        static const TString second = "second";
        static const TString secondType = "int";
        static const auto secondValue = 1ll << 32;

        TFrame frame{formName};
        frame.AddSlot(TSlot{first, firstType, TSlot::TValue{ToString(firstValue)}});
        frame.AddSlot(TSlot{second, secondType, TSlot::TValue{ToString(secondValue)}});

        const auto bassForm = NImpl::CreateBassLikeForm(frame, /* sourceTextProvider= */ nullptr);

        UNIT_ASSERT_VALUES_EQUAL(formName, bassForm->Name().Get());

        for (const auto slot : bassForm->Slots()) {
            if (slot->Name().Get() == first) {
                UNIT_ASSERT_VALUES_EQUAL(firstType, slot->Type().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(firstValue), *slot->Value().Get());
                UNIT_ASSERT(slot->SourceText().IsNull());
            } else {
                UNIT_ASSERT_VALUES_EQUAL(second, slot->Name().Get());
                UNIT_ASSERT_VALUES_EQUAL(secondType, slot->Type().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(secondValue), *slot->Value().Get());
                UNIT_ASSERT(slot->SourceText().IsNull());
            }
        }
    }

    Y_UNIT_TEST(SourceText) {
        static const TString formName = "hello";

        static const TString one = "one";
        static const TString oneType = "one_type";
        static const TString oneValue = "one_value";

        static const TString two = "two";
        static const TString twoType = "two_type";
        static const TString twoValue = "two_value";
        static const TString twoSourceText = "two_source_text";

        TSourceTextProvider sourceTextProvider = [&](const TStringBuf key, const TString& /* value */) -> TString {
            if (key == two) {
                return twoSourceText;
            }

            return {};
        };

        TFrame frame{formName};
        frame.AddSlot(TSlot{one, oneType, TSlot::TValue{oneValue}});
        frame.AddSlot(TSlot{two, twoType, TSlot::TValue{twoValue}});

        const auto bassForm = NImpl::CreateBassLikeForm(frame, &sourceTextProvider);

        UNIT_ASSERT_VALUES_EQUAL(formName, bassForm->Name().Get());

        for (const auto slot : bassForm->Slots()) {
            if (slot->Name().Get() == one) {
                UNIT_ASSERT_VALUES_EQUAL(oneType, slot->Type().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(oneValue), *slot->Value().Get());
                UNIT_ASSERT(slot->SourceText().IsNull());
            } else {
                UNIT_ASSERT_VALUES_EQUAL(two, slot->Name().Get());
                UNIT_ASSERT_VALUES_EQUAL(twoType, slot->Type().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(twoValue), *slot->Value().Get());
                UNIT_ASSERT_VALUES_EQUAL(NSc::TValue(twoSourceText), *slot->SourceText().Get());
            }
        }
    }
}


Y_UNIT_TEST_SUITE(BassLikeDataSources) {
    Y_UNIT_TEST(Smoke) {
        static const TString formName = "hello";

        static const TString one = "one";
        static const TString oneType = "one_type";
        static const TString oneValue = "one_value";

        // Create arguments.
        TFrame frame{formName};
        frame.AddSlot(TSlot{one, oneType, TSlot::TValue{oneValue}});

        NScenarios::TRequestMeta meta;
        auto request = CreateRequest<NScenarios::TScenarioRunRequest>();
        // Add data sources.
        NScenarios::TDialogHistoryDataSource dialogHistoryProto;
        *dialogHistoryProto.AddPhrases() = "test";
        {
            auto& dataSource = (*request.MutableDataSources())[EDataSourceType::DIALOG_HISTORY];
            *dataSource.MutableDialogHistory() = dialogHistoryProto;
        }
        TUserLocationProto userLocationProto;
        userLocationProto.SetUserRegion(213);
        userLocationProto.SetUserTld("ru");
        {
            auto& dataSource = (*request.MutableDataSources())[EDataSourceType::USER_LOCATION];
            *dataSource.MutableUserLocation() = userLocationProto;
        }

        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper wrapper{request, serviceCtx};
        const NJson::TJsonValue appHostParams;
        // Expect data source in bass request.
        {
            const auto httpRequest = PrepareBassVinsRequest(TRTLogger::NullLogger(), wrapper, frame,
                                                            /* sourceTextProvider= */ nullptr, meta,
                                                            /* imageSearch= */ false,
                                                            appHostParams,
                                                            /* forbidWebSearch= */ true,
                                                            {EDataSourceType::USER_LOCATION});
            const auto bassRequestRaw = NSc::TValue::FromJsonThrow(httpRequest.Request.GetContent());
            const TRequestConstScheme bassRequest(&bassRequestRaw);

            UNIT_ASSERT_VALUES_EQUAL(1, bassRequest.DataSources().Size());
            const auto it = std::cbegin(bassRequest.DataSources());
            UNIT_ASSERT_VALUES_EQUAL(static_cast<int>(EDataSourceType::USER_LOCATION), it.Key());
            UNIT_ASSERT(!it.Value().IsNull());
        }
        // Expect empty data sources by default.
        {
            const auto httpRequest = PrepareBassVinsRequest(TRTLogger::NullLogger(), wrapper, frame,
                                                            /* sourceTextProvider= */ nullptr, meta,
                                                            /* imageSearch= */ false, appHostParams);
            const auto bassRequestRaw = NSc::TValue::FromJsonThrow(httpRequest.Request.GetContent());
            const TRequestConstScheme bassRequest(&bassRequestRaw);

            UNIT_ASSERT(bassRequest.DataSources().Empty());
        }
    }
}

Y_UNIT_TEST_SUITE(PrepareBassRadioSimilarToObjContinueRequest) {
    Y_UNIT_TEST(Smoke) {
        NScenarios::TRequestMeta meta;
        NScenarios::TScenarioApplyRequest requestProto = CreateRequest<NScenarios::TScenarioApplyRequest>();
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioApplyRequestWrapper request{requestProto, serviceCtx};
        auto httpProxyRequest = PrepareBassRadioSimilarToObjContinueRequest(
            TRTLogger::NullLogger(), request, "track", "1695521", false, "", "",
            meta, "TEST_CONTINUATION", /* appHostParams = */{}
        );
        const auto& httpRequest = httpProxyRequest.Request;
        const auto content = JsonFromString(TString{httpRequest.GetContent()});
        UNIT_ASSERT_VALUES_EQUAL(4, content.GetMap().size());
        UNIT_ASSERT(!content["IsFinished"].GetBoolean());
        UNIT_ASSERT_STRINGS_EQUAL("TEST_CONTINUATION", content["ObjectTypeName"].GetString());

        const auto actualMeta = JsonFromString(content["Meta"].GetString());
        auto expectedMeta = JsonFromString(R"-({
            "client_features": {
                "supported": ["no_reliable_speakers", "no_bluetooth", "no_microphone"]
            },
            "client_id": "/ ( ;  )",
            "client_info": {
                "app_id": "",
                "app_version": "",
                "device_manufacturer": "",
                "device_model": "",
                "os_version": "",
                "platform": ""
            },
            "device_id": "",
            "dialog_id": "",
            "epoch": 1574930264,
            "event": {},
            "experiments": {},
            "has_image_search_granet": 0,
            "is_banned": 0,
            "lang": "",
            "pure_gc": 0,
            "request_id": "",
            "rng_seed": "0",
            "suppress_form_changes": 1,
            "tz": "UTC+3",
            "utterance": "",
            "uuid": "34D94983-65BD-49F2-B34E-BD123A7A9830",
            "voice_session": 0
        })-");
        UNIT_ASSERT_VALUES_EQUAL(expectedMeta, actualMeta);

        const auto state = JsonFromString(content["State"].GetString());
        auto expectedState = JsonFromString(R"-({
            "apply_arguments": {
                "web_answer": {
                    "id": "1695521",
                    "type": "track"
                }
            },
            "context": {
                "blocks": [],
                "form": {
                    "name": "personal_assistant.scenarios.music_play",
                    "slots": [{
                        "name": "need_similar",
                        "optional": true,
                        "type": "string",
                        "value": "need_similar"
                    }, {
                        "name": "track_id",
                        "optional": true,
                        "type": "string",
                        "value": "1695521"
                    }]
                }
            }
        })-");
        UNIT_ASSERT_VALUES_EQUAL(expectedState, state);
    }
}

Y_UNIT_TEST_SUITE(MusicPlayObjectAction) {
    Y_UNIT_TEST(CreateAction) {
        TFrame frame{"music_play_semantic_frame"};
        frame.AddSlot(TSlot{"object_id", "num_value", TSlot::TValue{"211604"}});
        frame.AddSlot(TSlot{"object_type", "enum_value", TSlot::TValue{"album"}});
        frame.AddSlot(TSlot{"start_from_track_id", "num_value", TSlot::TValue{"2142676"}});
        frame.AddSlot(TSlot{"track_offset_index", "num_value", TSlot::TValue{"9"}});
        frame.AddSlot(TSlot{"alarm_id", "string_value", TSlot::TValue{"1111ffff-11ff-11ff-11ff-111111ffffff"}});
        frame.AddSlot(TSlot{"offset_sec", "double_value", TSlot::TValue{"2.345"}});
        frame.AddSlot(TSlot{"order", "order_value", TSlot::TValue{"shuffle"}});
        frame.AddSlot(TSlot{"repeat", "repeat_value", TSlot::TValue{"All"}});

        const auto action = NImpl::CreateMusicPlayObjectAction(frame);

        UNIT_ASSERT_VALUES_EQUAL(action->Name(), "quasar.music_play_object");

        const auto expectedData = JsonFromString(R"({
            "object": {
                "id": "211604",
                "type": "album",
                "startFromId": "2142676",
                "startFromPosition": 9,
            },
            "alarm_id": "1111ffff-11ff-11ff-11ff-111111ffffff",
            "offset": 2.345,
            "shuffle": true,
            "repeat": true
        })");

        UNIT_ASSERT_VALUES_EQUAL(action->Data()->ToJsonValue(), expectedData);
    }

    Y_UNIT_TEST(HasSlotForMusicPlayObject) {
        {
            TFrame frame{"music_play_semantic_frame"};
            frame.AddSlot(TSlot{"object_id", "num_value", TSlot::TValue{"211604"}});

            UNIT_ASSERT_VALUES_EQUAL(HasSlotForMusicPlayObject(frame), true);
        }

        {
            TFrame frame{"music_play_semantic_frame"};
            frame.AddSlot(TSlot{"search_text", "string_value", TSlot::TValue{"foobar"}});

            UNIT_ASSERT_VALUES_EQUAL(HasSlotForMusicPlayObject(frame), false);
        }
    }
}

} // namespace NAlice::NHollywood
