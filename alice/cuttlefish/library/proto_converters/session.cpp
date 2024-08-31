#include "converters.h"
#include "converter_handlers.h"
#include <alice/cuttlefish/library/protos/session.traits.pb.h>


namespace NAlice::NCuttlefish::NProtoConverters {

namespace {

/*
{
    "InitialMessageId": "",
    "AppToken": "",
    "Application": {
        "Id": "",
        "Type": ""
    },
    "Device": {
        "Id": "",
        "Platform": "",
        "Model": "",
        "Manufacturer": ""
    },
    "User": {
        "AuthToken": "",
        "ICookie": "",
        "Uuid": "",
        "Puid": "",
        "Guid": "",
        "Yuid": ""
    },
    "UserOptions": {
        "DoNotUseLogs": ""
    },
    "Experiments": {
        "FlagsJson": {
            "AppInfo": "",
            "Data": ""
        }
    },
    "Messenger": {
        "UserType": "",
        "Version": "",
        "FanoutAuth": "",
        "Debug": {
            "MinVersion": "",
            "CurVersion": ""
        }
    }
}
*/
TConverter<NAliceProtocol::TSessionContext> CreateSessionContextConverter()
{
    using namespace NProtoTraits::NAliceProtocol;
    using NAlice::NCuttlefish::NConvert::TStrictConvert;

    TConverter<NAliceProtocol::TSessionContext> conv;
    auto b = conv.Build();

    b.SetValue<TSessionContext::SessionId>("SessionId");
    b.SetValue<TSessionContext::InitialMessageId>("InitialMessageId");

    b.SetValue<TSessionContext::AppToken>("AppToken");
    b.SetValue<TSessionContext::AppId>("Application/Id");
    b.SetValue<TSessionContext::AppType>("Application/Type");
    b.SetValue<TSessionContext::LaasResponseHasWifiInfo>("LaasResponseHasWifiInfo");

    auto userInfo = b.Sub<TSessionContext::UserInfo>();
    userInfo.SetValue<TUserInfo::Uuid>("User/Uuid");
    userInfo.SetValue<TUserInfo::Yuid>("User/Yuid");
    userInfo.SetValue<TUserInfo::Puid>("User/Puid");
    userInfo.SetValue<TUserInfo::Guid>("User/Guid");
    userInfo.Custom<TAuthTokenHandler>("User/AuthToken");
    userInfo.SetValue<TUserInfo::StaffLogin>("User/StaffLogin");
    userInfo.SetValue<TUserInfo::ICookie>("User/ICookie");
    userInfo.SetValue<TUserInfo::LaasRegion>("User/LaasRegion");

    auto userOptions = b.Sub<TSessionContext::UserOptions>();
    userOptions.SetValue<TUserOptions::DoNotUseLogs, NConvert::TSoftConvert, NConvert::TSerializeAlways>("UserOptions/DoNotUseLogs");

    auto fj = b.Sub<TSessionContext::Experiments>().Sub<TExperimentsContext::FlagsJsonData>();
    fj.SetValue<TFlagsJsonData::Data>("Experiments/FlagsJson/Data");
    fj.SetValue<TFlagsJsonData::AppInfo>("Experiments/FlagsJson/AppInfo");

    auto deviceInfo = b.Sub<TSessionContext::DeviceInfo>();
    deviceInfo.SetValue<TDeviceInfo::DeviceManufacturer>("Device/Manufacturer");
    deviceInfo.SetValue<TDeviceInfo::DeviceModel>("Device/Model");
    deviceInfo.SetValue<TDeviceInfo::DeviceId>("Device/Id");
    deviceInfo.SetValue<TDeviceInfo::Platform>("Device/Platform");
    deviceInfo.SetValue<TDeviceInfo::OsVersion>("Device/OsVersion");
    deviceInfo.SetValue<TDeviceInfo::NetworkType>("Device/NetworkType");
    deviceInfo.ForEachInArray().Append<TDeviceInfo::SupportedFeatures>("Device/SupportedFeatures");

    return conv;
}

}  // anonymous namespace

const TConverter<NAliceProtocol::TSessionContext> SessionContextConv = CreateSessionContextConverter();

}  // anonymous NAlice::NCuttlefish::NProtoConverters

