#include "mapper.h"

#include <alice/cuttlefish/library/utils/yaml/yaml_utils.h>

#include <library/cpp/resource/resource.h>

#include <contrib/libs/yaml-cpp/include/yaml-cpp/yaml.h>

#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/charset/utf8.h>


using namespace NYamlUtils;

namespace NVoice {
    namespace {
        static bool MatchStrings(TStringBuf value, TStringBuf pattern) {
            if (!pattern) {
                return true;
            }
            if (pattern.StartsWith(".*")) {
                return value.EndsWith(pattern.Skip(2));
            }
            return value == pattern;
        }
    }

    bool TClientInfo::CheckRestriction(const TClientInfo& restriction) const {
        // AppId was already checked before call of this function.
        return
            MatchStrings(AppInfo.AppVersion, restriction.AppInfo.AppVersion) &&
            MatchStrings(DeviceInfo.DeviceModel, restriction.DeviceInfo.DeviceModel) &&
            MatchStrings(DeviceInfo.DeviceModification, restriction.DeviceInfo.DeviceModification) &&
            MatchStrings(DeviceInfo.Platform, restriction.DeviceInfo.Platform);
    }
}

namespace NVoice::NSurfaceMapper {
    namespace {

        template <typename TEnum>
        TEnum AsEnum(TStringBuf value) {
            if (value.empty()) {
                // First value in "unknown".
                return TEnum();
            }
            return FromString<TEnum>(value);
        }

        template <typename TEnum>
        TFlags<TEnum> AsEnums(const TString& value) {
            TFlags<TEnum> result;
            for (auto value : StringSplitter(value).Split('/')) {
                result |= (AsEnum<TEnum>(value.Token()));
            }
            return result;
        }

        void VadidateInfo(const TStringBuf appId, const TSurfaceInfo& info) {
            // We shuold perform this check on startup.
            // It allows us to use single alias for set of enum values.
            Y_ENSURE(info.ListPlatforms().size() == 1, "Field Platform of " << appId << " is invalid");
            Y_ENSURE(info.ListTypes().size() == 1, "Field Type of " << appId << " is invalid");
        }

        template <typename Enum>
        TVector<TString> CastVector(const TVector<Enum>& items) {
            TVector<TString> result;
            for (const Enum item : items) {
                result.push_back(ToString(item));
            }
            return result;
        }

        TClientInfo LoadClientInfo(const YAML::Node& node) {
            TClientInfo result;
            FromSubnodeIfExist(node, result.AppInfo.AppId, "AppId");
            FromSubnodeIfExist(node, result.AppInfo.AppVersion, "AppVersion");
            FromSubnodeIfExist(node, result.DeviceInfo.DeviceModel, "DeviceModel");
            FromSubnodeIfExist(node, result.DeviceInfo.DeviceModification, "DeviceModification");
            FromSubnodeIfExist(node, result.DeviceInfo.Platform, "DevicePlatform");
            return result;
        }

        TUaasInfo LoadUaasInfo(const YAML::Node& node) {
            TUaasInfo result;
            FromSubnodeIfExist(node, result.BrowserName, "BrowserName");
            FromSubnodeIfExist(node, result.MobilePlatform, "MobilePlatform");
            FromSubnodeIfExist(node, result.DeviceType, "DeviceType");
            FromSubnodeIfExist(node, result.DeviceModel, "DeviceModel");
            return result;
        }

        THashMap<TString, TUaasInfo> LoadUaasInfoForQuasarFromYaml(const TString& yaml) {
            THashMap<TString, TUaasInfo> result;
            const YAML::Node document = YAML::Load(yaml);
            for (const YAML::Node& entry : document) {

                TVector<TString> platforms;
                if (FromSubnodeIfExist(entry, platforms, "QuasarPlatforms")) {

                    const YAML::Node& uaasInfoNode = entry["UaasInfo"];
                    Y_ASSERT(uaasInfoNode.IsDefined());
                    TUaasInfo info = LoadUaasInfo(uaasInfoNode);

                    for (const TString& platform : platforms) {
                        result[platform] = info;
                    }
                }
            }
            return result;
        }

        void FinalizeUaasInfo(TUaasInfo* info, const TClientInfo& clientInfo) {
            if (!(info->MobilePlatform)) {

                static const THashMap<TString, TString> plaformMap = {
                    {"Windows", "wp"},
                    {"Linux", "linux"},
                    {"android", "android"},
                    {"iphone", "iphone"},
                    {"ipad", "iphone"},
                };

                if (const TString* val = plaformMap.FindPtr(clientInfo.DeviceInfo.Platform)) {
                    info->MobilePlatform = *val;
                }
            }
        }

    }  // anonymous namespace

    TString TSurfaceInfo::GetSurface() const {
        return ToString(Surface);
    }

    EVendor TSurfaceInfo::GetVendor() const {
        return Vendor;
    }

    EPlatform TSurfaceInfo::GetPlatform() const {
        return GetGreatestNamedFlag(Platforms);
    }

    EType TSurfaceInfo::GetType() const {
        return GetGreatestNamedFlag(Types);
    }

    TVector<EPlatform> TSurfaceInfo::ListAllPlatforms() const {
        return ListAllFlags(Platforms);
    }

    TVector<EPlatform> TSurfaceInfo::ListPlatforms() const {
        return ListDisjointFlags(Platforms);
    }

    TVector<EType> TSurfaceInfo::ListAllTypes() const {
        return ListAllFlags(Types);
    }

    TVector<EType> TSurfaceInfo::ListTypes() const {
        return ListDisjointFlags(Types);
    }

    TString TSurfaceInfo::GetVendorStr() const {
        return ToString(GetVendor());
    }

    TString TSurfaceInfo::GetPlatformStr() const {
        return ToString(GetPlatform());
    }

    TString TSurfaceInfo::GetTypeStr() const {
        return ToString(GetType());
    }

    TVector<TString> TSurfaceInfo::ListAllPlatformsStr() const {
        return CastVector(ListAllPlatforms());
    }

    TVector<TString> TSurfaceInfo::ListAllTypesStr() const {
        return CastVector(ListAllTypes());
    }

    TVector<TString> TSurfaceInfo::ListPlatformsStr() const {
        return CastVector(ListPlatforms());
    }

    TVector<TString> TSurfaceInfo::ListTypesStr() const {
        return CastVector(ListTypes());
    }


    class TMapper::TImpl {
    private:
        static const inline TString UnknownAppId = "UnknownAppId";

        template<typename TValueType>
        class TClientInfoMap {
        private:
            using TValue = TValueType;
            using TRecord = std::pair<TClientInfo, TValue>;
            using TRecords = TVector<TRecord>;

        public:
            const TValue* FindPtr(const TClientInfo& key) const {
                const TRecords* recordsPtr = Mapping.FindPtr(ToLowerUTF8(key.AppInfo.AppId));
                if (!recordsPtr) {
                    recordsPtr = Mapping.FindPtr(UnknownAppId);
                }
                Y_ASSERT(recordsPtr);
                if (recordsPtr) {
                    for (const auto& [restriction, value] : *recordsPtr) {
                        if (key.CheckRestriction(restriction)) {
                            return &value;
                        }
                    }
                }
                return nullptr;
            }

            void AddRecord(TClientInfo restriction, TValue value) {
                const TString appId = restriction.AppInfo.AppId;
                Mapping[appId].emplace_back(std::move(restriction), std::move(value));
            }

        private:
            THashMap<TString, TRecords> Mapping;
        };

        static TClientInfoMap<TSurfaceInfo> LoadAppInfoFromYaml(const TString& yaml) {
            TClientInfoMap<TSurfaceInfo> result;
            const YAML::Node document = YAML::Load(yaml);
            for (const YAML::Node& entry : document) {
                TClientInfo key = LoadClientInfo(entry);

                TSurfaceInfo info;
                info.Surface = FromString<ESurface>(SubnodeAs<TString>(entry, "Surface"));
                info.Platforms = AsEnums<EPlatform>(SubnodeAs<TString>(entry, "Platform"));
                info.Types = AsEnums<EType>(SubnodeAs<TString>(entry, "Type"));
                info.Vendor = AsEnum<EVendor>(SubnodeAs<TString>(entry, "Vendor"));
                FromSubnodeIfExist(entry, info.KeysWhiteList, "AuthTokens");

                if (key.AppInfo.AppId) {
                    VadidateInfo(key.AppInfo.AppId, info);
                } else {
                    key.AppInfo.AppId = UnknownAppId;
                }
                result.AddRecord(std::move(key), std::move(info));
            }
            return result;
        }

        static TClientInfoMap<TUaasInfo> LoadUaasInfoFromYaml(const TString& yaml) {
            TClientInfoMap<TUaasInfo> result;
            const YAML::Node document = YAML::Load(yaml);
            for (const YAML::Node& entry : document) {

                const YAML::Node& uaasInfoNode = entry["UaasInfo"];
                Y_ASSERT(uaasInfoNode.IsDefined());
                TUaasInfo info = LoadUaasInfo(uaasInfoNode);

                if (const YAML::Node& restrictions = entry["Restrictions"]; restrictions.IsDefined()) {
                    TClientInfo key = LoadClientInfo(restrictions);

                    if (!key.AppInfo.AppId) {
                        key.AppInfo.AppId = UnknownAppId;
                    }

                    result.AddRecord(std::move(key), std::move(info));

                } else if (TVector<TString> appIds; FromSubnodeIfExist(entry, appIds, "SimpleRestrictions")){
                    for (const TString& appId : appIds) {
                        result.AddRecord({
                            .AppInfo = {
                                .AppId = appId,
                            }
                        }, info);
                    }

                } else {
                    Y_ASSERT(false);  // no restrictions found.
                }
            }
            return result;
        }

    public:
        TImpl()
            : AppInfoMapping(LoadAppInfoFromYaml(NResource::Find("/app_info.yaml")))
            , UaasInfoMapping(LoadUaasInfoFromYaml(NResource::Find("/uaas_info.yaml")))
        {
        }

        TMaybe<TSurfaceInfo> TryMap(const TClientInfo& clientInfo) const {
            TMaybe<TSurfaceInfo> info = Nothing();

            if (const TSurfaceInfo* appInfoPtr = AppInfoMapping.FindPtr(clientInfo)) {
                info = *appInfoPtr;

                if (const TUaasInfo* uaasInfoPtr = UaasInfoMapping.FindPtr(clientInfo)) {
                    info->UaasInfo = *uaasInfoPtr;
                    FinalizeUaasInfo(&(info->UaasInfo), clientInfo);
                }
            }

            return info;
        }

    private:
        TClientInfoMap<TSurfaceInfo> AppInfoMapping;
        TClientInfoMap<TUaasInfo> UaasInfoMapping;
    };

    TMapper::TMapper()
        : Impl(MakeHolder<TMapper::TImpl>())
    {
    }

    TMapper::~TMapper() { }

    TSurfaceInfo TMapper::Map(const TClientInfo& clientInfo) const {
        return TryMap(clientInfo).GetOrElse(DefaultSurfaceInfo);
    }

    // Do not use in new C++ code!
    TSurfaceInfo TMapper::MapForPythonBinding(const TString& appId) const {
        return Map({
            .AppInfo = {
                .AppId = appId
            }
        });
    }

    TMaybe<TSurfaceInfo> TMapper::TryMap(const TClientInfo& clientInfo) const {
        return Impl->TryMap(clientInfo);
    }

    const TMapper* GetMapperPtr() {
        return Singleton<TMapper>();
    }

    const TMapper& GetMapperRef() {
        return *GetMapperPtr();
    }


    TQuasarPlatformMapper::TQuasarPlatformMapper()
        : Mapping(LoadUaasInfoForQuasarFromYaml(NResource::Find("/uaas_info.yaml")))
    {
        Mapping[""] = GetMapperRef().Map({}).UaasInfo;
    }

    TUaasInfo TQuasarPlatformMapper::Map(const TQuasarInfo& quasarInfo) const {
        TUaasInfo result = MapImpl(quasarInfo);
        FinalizeUaasInfo(&result, {
            .DeviceInfo = {
                .Platform = "android",
            },
        });
        return result;
    }

    TUaasInfo TQuasarPlatformMapper::MapImpl(const TQuasarInfo& quasarInfo) const {
        if (const TUaasInfo* result = Mapping.FindPtr(quasarInfo.QuasarPlatform)) {
            return *result;
        } else {
            return Mapping.at("");
        }
    }

    const TQuasarPlatformMapper& GetQuasarPlatformMapperRef() {
        return *Singleton<TQuasarPlatformMapper>();
    }

}


template <>
void Out<NVoice::TClientInfo>(IOutputStream& out, const  NVoice::TClientInfo& x) {
    out << "{"
        "AppId=" << x.AppInfo.AppId << ", "
        "AppVersion=" << x.AppInfo.AppVersion << ", "
        "DeviceModel=" << x.DeviceInfo.DeviceModel << ", "
        "Platform=" << x.DeviceInfo.Platform << "}";
}


template <>
void Out<NVoice::TUaasInfo>(IOutputStream& out, const  NVoice::TUaasInfo& x) {
    out << "<"
        "BrowserName=" << x.BrowserName << ", "
        "MobilePlatform=" << x.MobilePlatform << ", "
        "DeviceType=" << x.DeviceType << ", "
        "DeviceModel=" << x.DeviceModel << ">";
}

template <>
void Out<NVoice::NSurfaceMapper::TSurfaceInfo>(IOutputStream& out, const  NVoice::NSurfaceMapper::TSurfaceInfo& x) {
    out << "("
        "surface=" << x.Surface << ", "
        "vendor=" << x.Vendor << ", "
        "platforms=" << x.Platforms << ", "
        "types=" << x.Types << ", "
        "uaas_info=" << x.UaasInfo << ")";
}
