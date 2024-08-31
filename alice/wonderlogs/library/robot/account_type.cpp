#include "account_type.h"

#include <alice/megamind/protos/speechkit/request.pb.h>

#include <google/protobuf/struct.pb.h>

#include <util/generic/vector.h>

namespace {

const TVector<TStringBuf> BAD_PREFIXES = {"deadbeef", "ffffffff", "dddddddd", "feedface",
                                          "deafbeef", "eeadbeef", "shooting"};

} // namespace

namespace NAlice::NWonderlogs {

TAccountTypeInfo::TAccountTypeInfo(const TWonderlog& wonderlog)
    : Uuid(wonderlog.GetUuid())
    , MessageId(wonderlog.GetMessageId())
    , MegamindRequestId(wonderlog.GetMegamindRequestId())
    , ProcessId(
          wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetBassOptions().HasProcessId()
              ? wonderlog.GetSpeechkitRequest().GetRequest().GetAdditionalOptions().GetBassOptions().GetProcessId()
              : TMaybe<TString>{}) {
    const auto& location = wonderlog.GetSpeechkitRequest().GetRequest().GetLaasRegion().fields();
    {
        const auto isYandexStaff = location.find("is_yandex_staff");
        if (isYandexStaff != location.end() &&
            isYandexStaff->second.kind_case() == google::protobuf::Value::kBoolValue) {
            LocationYandexStaff = isYandexStaff->second.bool_value();
        }
    }
    {
        const auto isYandexNet = location.find("is_yandex_net");
        if (isYandexNet != location.end() && isYandexNet->second.kind_case() == google::protobuf::Value::kBoolValue) {
            LocationYandexNet = isYandexNet->second.bool_value();
        }
    }
    if (wonderlog.HasDownloadingInfo()) {
        TVector<const TWonderlog::TDownloadingInfo::TIpInfo*> ipInfos;
        if (wonderlog.GetDownloadingInfo().HasUniproxy()) {
            ipInfos.push_back(&wonderlog.GetDownloadingInfo().GetUniproxy());
        }
        if (wonderlog.GetDownloadingInfo().HasMegamind()) {
            ipInfos.push_back(&wonderlog.GetDownloadingInfo().GetMegamind());
        }
        for (const auto* ipInfo : ipInfos) {
            if (ipInfo->HasYandexNet()) {
                YandexNet = ipInfo->GetYandexNet();
            }
            if (ipInfo->HasStaffNet()) {
                YandexStaff = ipInfo->GetStaffNet();
            }
        }
    }
}

bool TAccountTypeInfo::GoodId(const TStringBuf id) {
    return !AnyOf(BAD_PREFIXES.begin(), BAD_PREFIXES.end(),
                  [&id](const auto prefix) { return id.StartsWith(prefix); });
}

bool TAccountTypeInfo::Robot() const {
    if (ProcessId) {
        return true;
    }
    for (const auto& id : {Uuid, MessageId, MegamindRequestId}) {
        if (!GoodId(id)) {
            return true;
        }
    }
    if (((LocationYandexNet && *LocationYandexNet) || (YandexNet && *YandexNet)) && !Staff()) {
        return true;
    }
    return false;
}

bool TAccountTypeInfo::Staff() const {
    return (LocationYandexStaff && *LocationYandexStaff) || (YandexStaff && *YandexStaff);
}

TWonderlog::EAccountType TAccountTypeInfo::Type() const {
    if (Robot()) {
        return TWonderlog::AT_ROBOT;
    }
    if (Staff()) {
        return TWonderlog::AT_STAFF;
    }
    return TWonderlog::AT_HUMAN;
}

} // namespace NAlice::NWonderlogs
