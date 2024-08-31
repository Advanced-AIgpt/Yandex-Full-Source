#include "demo_smart_home.h"

#include "defs.h"
#include "utils.h"

#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>


namespace NAlice::NIot {

namespace {

template <typename T>
void AddPrefixToIds(const TStringBuf prefix, T* iotUserInfoField) {
    for (auto& item : *iotUserInfoField) {
        item.SetId(TStringBuilder() << prefix << item.GetId());
    }
}

TIoTUserInfo ReadSmartHome(const TStringBuf path) {
    auto ioTUserInfo = IoTFromIoTValue(NSc::TValue::FromJson(NResource::Find(path)));

    AddPrefixToIds(DEMO_PREFIX, ioTUserInfo.MutableDevices());
    AddPrefixToIds(DEMO_PREFIX, ioTUserInfo.MutableGroups());
    AddPrefixToIds(DEMO_PREFIX, ioTUserInfo.MutableRooms());
    AddPrefixToIds(DEMO_PREFIX, ioTUserInfo.MutableColors());
    AddPrefixToIds(DEMO_PREFIX, ioTUserInfo.MutableScenarios());

    return ioTUserInfo;
}

class TDemoSmartHomeLoader {
public:
    static const TIoTUserInfo* GetDemoSmartHome(const ELanguage language) {
        return Singleton<TDemoSmartHomeLoader>()->Storage_.FindPtr(language);
    }

private:
    TDemoSmartHomeLoader() {
        constexpr ELanguage supportedLanguages[] = {LANG_RUS, LANG_ARA};
        for (const auto language : supportedLanguages) {
            LoadDemoSmartHome(language);
        }
    }

    void LoadDemoSmartHome(const ELanguage language) {
        const TString fileName = TStringBuilder() << NameByLanguage(language) << "_demo_sh.json";
        Storage_[language] = ReadSmartHome(fileName);
    }

private:
    THashMap<ELanguage, TIoTUserInfo> Storage_;

    Y_DECLARE_SINGLETON_FRIEND();
};


}  // namespace

const TIoTUserInfo* GetDemoSmartHome(ELanguage language) {
    return TDemoSmartHomeLoader::GetDemoSmartHome(language);
}

}  // namespace NAlice::NIot
