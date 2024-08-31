#include "process_biometry.h"

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/protos/endpoint/capabilities//bio/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

#include <apphost/lib/service_testing/service_testing.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/biometry/ut/data";
constexpr TStringBuf CHILD_VOICE = "child-voice.json";
constexpr TStringBuf ADULT_VOICE = "adult-voice.json";
constexpr TStringBuf INCOGNITO_SCORED_VOICE = "incognito-scored-voice.json";

const TString DEFAULT_OAUTH_TOKEN = "NONEMPTYVALUE";
const TString DEFAULT_PERS_ID = "PersId-123";

const TString INCOGNITO_KOLONKISH_UID = "1440006881";
const TString INCOGNITO_REQUEST_DEVICE_ID = "XW00000000000000548000000b66c150";
const TString CHILD_REQUEST_DEVICE_ID = "feedface-e8a2-4439-b2e7-689d95f277b7";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

template<typename TScenarioRequest>
TScenarioRequest ConstructRequest(TStringBuf filename) {
    const TString data = ReadTestData(filename);
    const NJson::TJsonValue json = JsonFromString(data);
    return JsonToProto<TScenarioRequest>(json, /* validateUtf8 = */ false, /* ignoreUnknownFields = */ true);
}

NAlice::TEnvironmentState MakeBioCapabilityEnvironmentState(const TString& deviceId) {
    NAlice::TEnvironmentState environmentState;
    auto& endpoint = *environmentState.AddEndpoints();
    endpoint.SetId(deviceId);

    NAlice::TBioCapability bioCapability;
    endpoint.AddCapabilities()->PackFrom(bioCapability);

    return environmentState;
}

void AddGuestCredentialsToApplyArgs(NScenarios::TScenarioApplyRequest& applyRequestProto,
                                    const TString& guestUid,
                                    bool isOwnerEnrolled,
                                    bool addOAuthToken = true,
                                    const TMaybe<TString>& bioCapabilityDeviceId = Nothing()) {
    TMusicArguments musicArguments;
    musicArguments.MutableGuestCredentials()->SetUid(guestUid);
    musicArguments.SetIsOwnerEnrolled(isOwnerEnrolled);
    if (addOAuthToken) {
        musicArguments.MutableGuestCredentials()->SetOAuthTokenEncrypted(DEFAULT_OAUTH_TOKEN);
    }
    if (bioCapabilityDeviceId) {
        *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(*bioCapabilityDeviceId);
    }
    applyRequestProto.MutableArguments()->PackFrom(musicArguments);
}

void AddGuestCredentialsToRunRequest(NScenarios::TScenarioRunRequest& runRequestProto,
                                     const TString& guestUid,
                                     bool isOwnerEnrolled,
                                     bool addOAuthToken = true) {
    NScenarios::TDataSource guestOptions;
    guestOptions.MutableGuestOptions()->SetYandexUID(guestUid);
    guestOptions.MutableGuestOptions()->SetPersId(DEFAULT_PERS_ID);
    guestOptions.MutableGuestOptions()->SetStatus(TGuestOptions::Match);
    guestOptions.MutableGuestOptions()->SetIsOwnerEnrolled(isOwnerEnrolled);
    if (addOAuthToken) {
        guestOptions.MutableGuestOptions()->SetOAuthToken(DEFAULT_OAUTH_TOKEN);
    }
    (*runRequestProto.MutableDataSources())[EDataSourceType::GUEST_OPTIONS] = std::move(guestOptions);
}

void AddBioCapabilityToRunRequest(NScenarios::TScenarioRunRequest& runRequestProto, const TString& deviceId) {
    NScenarios::TDataSource environmentState;
    *environmentState.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(deviceId);
    (*runRequestProto.MutableDataSources())[EDataSourceType::ENVIRONMENT_STATE] = std::move(environmentState);
}

} // namespace

Y_UNIT_TEST_SUITE(ProcessBiometryTest) {

Y_UNIT_TEST(ChildVoice) {
    const auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(CHILD_VOICE);
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "012345";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be a child
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
}

Y_UNIT_TEST(AdultVoice) {
    const auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(ADULT_VOICE);
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "012345";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should not be a child
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
}

Y_UNIT_TEST(IncognitoScoredVoice) {
    const auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an incognito
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
}

Y_UNIT_TEST(NoBioCapabilityGuestScoredVoiceInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToApplyArgs(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ true);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an incognito as there is no BioCapability
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(NoBioCapabilityGuestScoredVoiceInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToRunRequest(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ true);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an incognito as there is no BioCapability
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(NoBioCapabilityEmptyGuestCredentialsInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);

    TMusicArguments musicArguments;
    musicArguments.MutableGuestCredentials();
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an incognito as there is no BioCapability
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(NoBioCapabilityEmptyGuestDataSourceInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(INCOGNITO_SCORED_VOICE);

    NScenarios::TDataSource guestOptions;
    guestOptions.MutableGuestOptions();
    (*requestProto.MutableDataSources())[EDataSourceType::GUEST_OPTIONS] = std::move(guestOptions);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an incognito as there is no BioCapability
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(NoBioCapabilityEmptyGuestOAuthCredentialInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToApplyArgs(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ true,
                                   /* addOAuthToken = */ false);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an incognito as there is no BioCapability
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(NoBioCapabilityEmptyGuestOAuthDataSourceFieldInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToRunRequest(requestProto, guestUid,
                                    /* isOwnerEnrolled = */ true,
                                    /* addOAuthToken = */ false);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an incognito as there is no BioCapability
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(GuestVoiceInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToApplyArgs(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ true,
                                   /* addOAuthToken = */ true,
                                   INCOGNITO_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, guestUid);
    // the user should be a guest
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
}

Y_UNIT_TEST(GuestScoredVoiceInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToRunRequest(requestProto, guestUid,
                                    /* isOwnerEnrolled = */ true);
    AddBioCapabilityToRunRequest(requestProto, INCOGNITO_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, guestUid);
    // the user should be a guest
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
}

Y_UNIT_TEST(NoOwnerEmptyGuestCredentialsInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);

    TMusicArguments musicArguments;
    musicArguments.MutableGuestCredentials();
    *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(INCOGNITO_REQUEST_DEVICE_ID);
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(EmptyGuestCredentialsInApplyRequestOwnerEnrolled) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);

    TMusicArguments musicArguments;
    musicArguments.MutableGuestCredentials();
    *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(INCOGNITO_REQUEST_DEVICE_ID);
    musicArguments.SetIsOwnerEnrolled(true);
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(NoGuestCredentialsInApplyRequestOwnerEnrolled) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);

    TMusicArguments musicArguments;
    *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(INCOGNITO_REQUEST_DEVICE_ID);
    musicArguments.SetIsOwnerEnrolled(true);
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an incognito
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID);
}

Y_UNIT_TEST(EmptyGuestDataSourceInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(INCOGNITO_SCORED_VOICE);

    NScenarios::TDataSource guestOptions;
    guestOptions.MutableGuestOptions();
    (*requestProto.MutableDataSources())[EDataSourceType::GUEST_OPTIONS] = std::move(guestOptions);

    AddBioCapabilityToRunRequest(requestProto, INCOGNITO_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(EmptyGuestOAuthCredentialInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToApplyArgs(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ true,
                                   /* addOAuthToken = */ false,
                                   INCOGNITO_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(EmptyGuestOAuthDataSourceFieldInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(INCOGNITO_SCORED_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToRunRequest(requestProto, guestUid,
                                    /* isOwnerEnrolled = */ true,
                                    /* addOAuthToken = */ false);
    AddBioCapabilityToRunRequest(requestProto, INCOGNITO_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(NoOwnerChildGuestVoiceInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(CHILD_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToApplyArgs(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ false,
                                   /* addOAuthToken = */ true,
                                   CHILD_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, guestUid);
    // the user should be a guest
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
}

Y_UNIT_TEST(NoOwnerChildGuestVoiceInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(CHILD_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToRunRequest(requestProto, guestUid,
                                    /* isOwnerEnrolled = */ false);
    AddBioCapabilityToRunRequest(requestProto, CHILD_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, guestUid);
    // the user should be a guest
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
}

Y_UNIT_TEST(NoOwnerChildVoiceEmptyGuestCredentialsInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(CHILD_VOICE);

    TMusicArguments musicArguments;
    musicArguments.MutableGuestCredentials();
    *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(CHILD_REQUEST_DEVICE_ID);
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(ChildVoiceEmptyGuestCredentialsInApplyRequestOwnerEnrolled) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(CHILD_VOICE);

    TMusicArguments musicArguments;
    musicArguments.MutableGuestCredentials();
    *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(CHILD_REQUEST_DEVICE_ID);
    musicArguments.SetIsOwnerEnrolled(true);
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(ChildVoiceNoGuestCredentialsOwnerEnrolledInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(CHILD_VOICE);

    TMusicArguments musicArguments;
    *musicArguments.MutableEnvironmentState() = MakeBioCapabilityEnvironmentState(CHILD_REQUEST_DEVICE_ID);
    musicArguments.SetIsOwnerEnrolled(true);
    requestProto.MutableArguments()->PackFrom(musicArguments);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an incognito
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    // UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, INCOGNITO_KOLONKISH_UID); NOTE(klim-roma): no kolonkish uid in DataSync
}

Y_UNIT_TEST(ChildVoiceNoMatchOwnerEnrolledInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(CHILD_VOICE);

    NScenarios::TDataSource guestOptions;
    guestOptions.MutableGuestOptions()->SetIsOwnerEnrolled(true);
    (*requestProto.MutableDataSources())[EDataSourceType::GUEST_OPTIONS] = std::move(guestOptions);

    AddBioCapabilityToRunRequest(requestProto, CHILD_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(ChildVoiceEmptyGuestCredentialsOwnerEnrolledInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(CHILD_VOICE);

    NScenarios::TDataSource guestOptions;
    guestOptions.MutableGuestOptions()->SetStatus(TGuestOptions::Match);
    guestOptions.MutableGuestOptions()->SetIsOwnerEnrolled(true);
    (*requestProto.MutableDataSources())[EDataSourceType::GUEST_OPTIONS] = std::move(guestOptions);

    AddBioCapabilityToRunRequest(requestProto, CHILD_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(NoOwnerChildVoiceEmptyGuestOAuthCredentialInApplyRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioApplyRequest>(CHILD_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToApplyArgs(requestProto, guestUid,
                                   /* isOwnerEnrolled = */ false,
                                   /* addOAuthToken = */ false,
                                   CHILD_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), applyRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

Y_UNIT_TEST(NoOwnerChildVoiceEmptyGuestOAuthDataSourceFieldInRunRequest) {
    auto requestProto = ConstructRequest<NScenarios::TScenarioRunRequest>(CHILD_VOICE);
    const TString guestUid = "202300222";
    AddGuestCredentialsToRunRequest(requestProto, guestUid,
                                    /* isOwnerEnrolled = */ false,
                                    /* addOAuthToken = */ false);
    AddBioCapabilityToRunRequest(requestProto, CHILD_REQUEST_DEVICE_ID);

    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper runRequest{requestProto, serviceCtx};

    const TStringBuf uid = "291303909";
    const auto biometryData = ProcessBiometryOrFallback(TRTLogger::NullLogger(), runRequest, uid);

    // the user should be an owner
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsIncognitoUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsGuestUser, false);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.IsChild, true);
    UNIT_ASSERT_VALUES_EQUAL(biometryData.UserId, uid);
}

};

}  // namespace NAlice::NHollywood::NMusic
