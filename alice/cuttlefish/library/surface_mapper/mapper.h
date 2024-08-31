#pragma once

#include "enums.h"
#include "surfaces.h"

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

namespace NVoice {

    struct TClientInfo {
        struct TDeviceInfo {
            TString DeviceModel;
            TString Platform;
            TString DeviceModification;
        };

        struct TApplicationInfo {
            TString AppId;
            TString AppVersion;
        };

        TApplicationInfo AppInfo;
        TDeviceInfo DeviceInfo;

        bool CheckRestriction(const TClientInfo& restriction) const;
    };

    struct TQuasarInfo {
        TString QuasarPlatform;
    };

    struct TUaasInfo {
        TString BrowserName;
        TString MobilePlatform;
        TString DeviceType;
        TString DeviceModel;
    };

}  // namespace NVoice


namespace NVoice::NSurfaceMapper {
    class TSurfaceInfo {
    public:
        ESurface Surface = ESurface::Unknown;
        EVendor Vendor = V_UNKNOWN;
        TPlatforms Platforms;
        TTypes Types;
        TVector<TString> KeysWhiteList;
        TUaasInfo UaasInfo;

        // Methods for python bindings.
        TString GetSurface() const;
        EVendor GetVendor() const;

        // Following functions return most valuable single platform/type.
        // They can be used as graph label.
        EPlatform GetPlatform() const;
        EType GetType() const;

        // Following functions return list of all platform/type names and suitable aliases.
        // They create vector, thus they are efficient only in context of python bindings. Use TFlags api in C++.
        TVector<EPlatform> ListAllPlatforms() const;
        TVector<EType> ListAllTypes() const;

        // Following functions return list of disjoint names and aliases.
        // They create vector, thus they are efficient only in context of python bindings. Use TFlags api in C++.
        TVector<EPlatform> ListPlatforms() const;
        TVector<EType> ListTypes() const;

        // String versions of the above functions.
        TString GetVendorStr() const;
        TString GetPlatformStr() const;
        TString GetTypeStr() const;
        TVector<TString> ListAllPlatformsStr() const;
        TVector<TString> ListAllTypesStr() const;
        TVector<TString> ListPlatformsStr() const;
        TVector<TString> ListTypesStr() const;
    };

    class TMapper {
    public:
        static inline const TSurfaceInfo DefaultSurfaceInfo = TSurfaceInfo();

    public:
        TMapper();

        ~TMapper();

        TMaybe<TSurfaceInfo> TryMap(const TClientInfo& clientInfo) const;

        TSurfaceInfo Map(const TClientInfo& clientInfo) const;

        // Do not use in new C++ code!
        TSurfaceInfo MapForPythonBinding(const TString& appId) const;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };

    const TMapper* GetMapperPtr();
    const TMapper& GetMapperRef();


    class TQuasarPlatformMapper {
    public:
        TQuasarPlatformMapper();

        TUaasInfo Map(const TQuasarInfo& quasarInfo) const;

    private:
        TUaasInfo MapImpl(const TQuasarInfo& quasarInfo) const;

        THashMap<TString, TUaasInfo> Mapping;
    };

    const TQuasarPlatformMapper& GetQuasarPlatformMapperRef();
}
