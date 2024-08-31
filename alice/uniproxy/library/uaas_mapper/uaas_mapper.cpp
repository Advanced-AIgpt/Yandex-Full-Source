#include "uaas_mapper.h"

#include <library/cpp/json/json_writer.h>
#include <library/cpp/string_utils/base64/base64.h>


namespace NVoice {

void GetUaasInfo(TStringBuf appId, TStringBuf platformInfo, TString& browserName, TString& mobilePlatform, TString& deviceType) {
    TUaasInfo result = GetUaasInfoByClientInfo({
        .AppInfo = {
            .AppId = TString(appId),
        },
        .DeviceInfo = {
            .Platform = TString(platformInfo),
        }
    });

    browserName = std::move(result.BrowserName);
    mobilePlatform = std::move(result.MobilePlatform);
    deviceType = std::move(result.DeviceType);
}

TString BuildUaasInfoJson(const TUaasInfo& uaasInfo) {
    TStringStream ss;

    {
        NJson::TJsonWriter writer(&ss, false);

        writer.OpenMap();
        writer.Write("browserName", uaasInfo.BrowserName);
        writer.Write("deviceType", uaasInfo.DeviceType);
        writer.Write("deviceModel", uaasInfo.DeviceModel);
        writer.Write("mobilePlatform", uaasInfo.MobilePlatform);
        writer.CloseMap();
    }

    return Base64Encode(ss.Str());
}

TString GetUaasYandexAppInfo(TStringBuf appId, TStringBuf platformInfo) {
    TUaasInfo uaasInfo;
    GetUaasInfo(
        appId, platformInfo,
        uaasInfo.BrowserName,
        uaasInfo.MobilePlatform,
        uaasInfo.DeviceType
    );

    return BuildUaasInfoJson(uaasInfo);
}

TUaasInfo GetUaasInfoByClientInfo(const TClientInfo& clientInfo) {
    return NSurfaceMapper::GetMapperRef().Map(clientInfo).UaasInfo;
}


TString GetUaasInfoJsonByClientInfo(const TClientInfo& clientInfo) {
    return BuildUaasInfoJson(GetUaasInfoByClientInfo(clientInfo));
}

TString GetUaasInfoJsonByQuasarPlatform(const TQuasarInfo& quasarInfo) {
    return BuildUaasInfoJson(NSurfaceMapper::GetQuasarPlatformMapperRef().Map(quasarInfo));
}

}   // namespace NVoice
