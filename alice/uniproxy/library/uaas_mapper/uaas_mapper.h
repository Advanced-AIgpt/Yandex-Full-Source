#pragma once

#include <alice/cuttlefish/library/surface_mapper/mapper.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>


namespace NVoice {

/**
 *  @brief
 *  @param[in]  appId           SpeechKit/VINS application id, e.g. "ru.yandex.searchapp"
 *  @param[in]  platformInfo    Platform, e.g. "iphone", "android", "linux" etc.
 *  @param[out] browserName     ???
 *  @param[out] mobilePlatform  ???
 *  @param[out] deviceType      ???
 *  @return true if browserName and mobilePlatform contains valid strings for UAAS, false otherwise
 */
void GetUaasInfo(TStringBuf appId, TStringBuf platformInfo, TString& browserName, TString& mobilePlatform, TString& deviceType);


/**
 *  @brief
 *  @param[in]  appId           SpeechKit/VINS application id, e.g. "ru.yandex.searchapp"
 *  @param[in]  platformInfo    Platform, e.g. "iphone", "android", "linux" etc.
 *  @return base64-encoded json string (VOICESERV-2886)
 */
TString GetUaasYandexAppInfo(TStringBuf appId, TStringBuf platformInfo);

TUaasInfo GetUaasInfoByClientInfo(const TClientInfo& clientInfo);
TString GetUaasInfoJsonByClientInfo(const TClientInfo& clientInfo);
TString GetUaasInfoJsonByQuasarPlatform(const TQuasarInfo& quasarInfo);

}   // namespace NVoice
