#include "card_request.h"
#include "utils.h"

#include <alice/bass/libs/logging/logger.h>

namespace NPersonalCards {

namespace {

constexpr TStringBuf DEVICE_ID_PATH = "app_metrika.device_id";
constexpr TStringBuf DID_PATH = "app_metrika.did";
constexpr TStringBuf UID_PATH = "auth.uid";
constexpr TStringBuf UUID_PATH = "app_metrika.uuid";
constexpr TStringBuf YANDEXUID_PATH = "yandexuid";

constexpr TStringBuf CARD_CLIENT_ID_ATTR = "client_id";
constexpr TStringBuf CARD_TIME_NOW_ATTR = "time_now";

TInstant ExtractTimeNow(const NJson::TJsonMap& request) {
    const auto& val = request[CARD_TIME_NOW_ATTR];
    if (val.IsUInteger()) {
        return TInstant::Seconds(val.GetUInteger());
    } else if (val.IsString()) {
        if (ui64 res; TryFromString(val.GetString(), res)) {
            return TInstant::Seconds(res);
        }
    }

    return TInstant::Now();
}

TString ExtractUUID(const NJson::TJsonMap& request) {
    if (const auto idPtr = request.GetValueByPath(UUID_PATH); idPtr && idPtr->IsString()) {
        return NormalizeUUID(idPtr->GetString());
    }

    return TString();
}

TString ExtractDeviceId(const NJson::TJsonMap& request) {
    for (const auto path : {DEVICE_ID_PATH, DID_PATH} /* Paths order is important! */) {
        if (const auto idPtr = request.GetValueByPath(path); idPtr && idPtr->IsString()) {
            return NormalizeUUID(idPtr->GetString());
        }
    }

    return TString();
}

ui64 ExtractUID(const NJson::TJsonMap& request) {
    if (const auto idPtr = request.GetValueByPath(UID_PATH); idPtr) {
        if (idPtr->IsString()) {
            if (ui64 res; TryFromString(idPtr->GetString(), res)) {
                return res;
            } else {
                LOG(ERR) << "Invalid passport uid " << res << Endl;
            }
        } else if (idPtr->IsUInteger()) {
            return idPtr->GetUInteger();
        }
    }

    return 0;
}

ui64 ExtractYandexUID(const NJson::TJsonMap& request) {
    if (const auto idPtr = request.GetValueByPath(YANDEXUID_PATH); idPtr) {
        if (idPtr->IsString()) {
            if (ui64 res; TryFromString(idPtr->GetString(), res)) {
                return res;
            } else {
                LOG(ERR) << "Invalid yandex uid " << res << Endl;
            }
        } else if (idPtr->IsUInteger()) {
            return idPtr->GetUInteger();
        }
    }

    return 0;
}

TString ExtractBestUserId(const NJson::TJsonMap& request) {
    if (const auto uid = ExtractUID(request))
        return TStringBuilder() << "uid/"sv << uid;
    if (const auto did = ExtractDeviceId(request))
        return TStringBuilder() << "device_id/"sv << did;
    if (const auto uuid = ExtractUUID(request))
        return TStringBuilder() << "uuid/"sv << uuid;
    if (const auto yandexUid = ExtractYandexUID(request))
        return TStringBuilder() << "yandexuid/" << yandexUid;

    return TString();
}

} // namespace

TCardRequest::TCardRequest(const NJson::TJsonMap& request)
    : Timestamp_(ExtractTimeNow(request))
    , BestUserId_(ExtractBestUserId(request))
    , ClientId_(request[CARD_CLIENT_ID_ATTR].GetString())
{
}

} // namespace NPersonalCards
