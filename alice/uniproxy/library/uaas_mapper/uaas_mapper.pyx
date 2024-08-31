from util.generic.string cimport TString
from util.generic.string cimport TStringBuf


cdef extern from "alice/uniproxy/library/uaas_mapper/uaas_mapper.h" namespace "NVoice":
    void GetUaasInfo(TStringBuf appId, TStringBuf platformInfo, TString& browserName, TString& mobilePlatform, TString& deviceType)

    TString GetUaasYandexAppInfo(TStringBuf appId, TStringBuf platformInfo)


def GetUaasAppInfo(appid: str, platform_info: str):
    cdef TString browser
    cdef TString platform
    cdef TString device
    cdef TStringBuf app_id_buf
    cdef TStringBuf platform_info_buf

    app_id_tmp = appid.encode('utf-8')
    app_id_buf = app_id_tmp

    platform_info_tmp = platform_info.encode('utf-8')
    platform_info_buf = platform_info_tmp

    GetUaasInfo(app_id_buf, platform_info_buf, browser, platform, device)

    return browser.decode('utf-8'), platform.decode('utf-8'), device.decode('utf-8')


def GetUaasAppHeader(appid: str, platform_info: str):
    cdef TStringBuf app_id_buf
    cdef TStringBuf platform_info_buf

    app_id_tmp = appid.encode('utf-8')
    app_id_buf = app_id_tmp

    platform_info_tmp = platform_info.encode('utf-8')
    platform_info_buf = platform_info_tmp

    value = GetUaasYandexAppInfo(app_id_buf, platform_info_buf)
    return value.decode('utf-8')
