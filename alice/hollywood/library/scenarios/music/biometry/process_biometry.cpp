#include "process_biometry.h"

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/biometry/client_biometry.h>
#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>
#include <alice/hollywood/library/personal_data/personal_data.h>

#include <alice/megamind/protos/guest/guest_options.pb.h>

namespace NAlice::NHollywood::NMusic {

bool IsClassifiedAsChildRequest(const TBiometryClassification& biometryClassification) {
    // logic ported from https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/context/context.cpp?rev=r8264976#L633-644
    for (const auto& classification : biometryClassification.GetSimple()) {
        if (classification.GetTag() == "children" && classification.GetClassName() == "child") {
            return true;
        }
    }
    return false;
}

TBiometryData ConstructBiometryDataWithGuestUserId(const TString& guestUserId) {
    return {.IsIncognitoUser = false, .IsGuestUser = true, .UserId = guestUserId};
}

TMaybe<TBiometryData> ProcessBiometrySpecified(TRTLogger& logger, const TScenarioRunRequestWrapper& request, TStringBuf uid) {
    if (!SupportsClientBiometry(request)) {
        return ProcessBiometry(logger, request, uid);
    }

    LOG_INFO(logger) << "BioCapability is supported, client biometry will be processed";

    TBiometryData biometryData{
        .IsIncognitoUser = false,
        .IsGuestUser = false,
        .UserId = TString(uid),
    };

    const auto* guestOptions = GetGuestOptionsProto(request);
    if (!guestOptions) {
        LOG_ERROR(logger) << "No TGuestOptions data source were found in request";
        return biometryData;
    }

    if (!ValidateGuestOptionsDataSource(logger, *guestOptions)) {
        return biometryData;
    }

    if (IsIncognitoModeRunRequest(*guestOptions)) {
        biometryData.IsIncognitoUser = true;

        if (const auto ownerName = GetOwnerNameFromDataSync(request)) {
            biometryData.OwnerName = *ownerName;
        } else {
            LOG_ERROR(logger) << "Failed to get owner's name from DataSync";
        }

        if (const auto kolonkishUid = GetKolonkishUidFromDataSync(request)) {
            biometryData.UserId = *kolonkishUid;
        } else {
            LOG_ERROR(logger) << "Failed to get kolonkish uid from DataSync";
        }

        return biometryData;
    }

    if (guestOptions->GetStatus() != TGuestOptions::Match) {
        LOG_INFO(logger) << "IsOwnerEnrolled=" << guestOptions->GetIsOwnerEnrolled()
                         << ", Status=" << TGuestOptions::EStatus_Name(guestOptions->GetStatus())
                         << ", we act as there is no enrolled owner and no match";
        return biometryData;
    }

    biometryData.UserId = guestOptions->GetYandexUID();
    biometryData.IsGuestUser = biometryData.UserId != uid;
    return biometryData;
}

TMaybe<TBiometryData> ProcessBiometrySpecified(TRTLogger& logger, const TScenarioApplyRequestWrapper& request, TStringBuf uid) {
    if (!SupportsClientBiometry(request)) {
        return ProcessBiometry(logger, request, uid);
    }

    TBiometryData biometryData{
        .IsIncognitoUser = false,
        .IsGuestUser = false,
        .UserId = TString(uid),
    };

    LOG_INFO(logger) << "BioCapability support is found in applyArgs, guest credentials will be processed";

    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    if (IsIncognitoModeApplyRequest(applyArgs)) {
        LOG_INFO(logger) << "Owner is enrolled but no guest credentials were found in applyArgs. It implies incognito mode";
        biometryData.IsIncognitoUser = true;

        if (const auto ownerName = GetOwnerNameFromDataSync(request)) {
            biometryData.OwnerName = *ownerName;
        } else {
            LOG_ERROR(logger) << "Failed to get owner's name from DataSync";
        }

        if (const auto kolonkishUid = GetKolonkishUidFromDataSync(request)) {
            biometryData.UserId = *kolonkishUid;
        } else {
            LOG_ERROR(logger) << "Failed to get kolonkish uid from DataSync";
        }

        return biometryData;
    }

    if (!IsClientBiometryModeApplyRequest(logger, applyArgs)) {
        LOG_INFO(logger) << "IsOwnerEnrolled=" << applyArgs.GetIsOwnerEnrolled()
                         << ", HasGuestCredentials=" << applyArgs.HasGuestCredentials()
                         << ", we act as there is no enrolled owner and no match";
        return biometryData;
    }
    
    biometryData.UserId = applyArgs.GetGuestCredentials().GetUid();
    biometryData.IsGuestUser = biometryData.UserId != uid;
    return biometryData;
}

} // namespace NAlice::NHollywood::NMusic
