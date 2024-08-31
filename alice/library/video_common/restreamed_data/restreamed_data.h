#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/fwd.h>
#include <util/generic/hash.h>
#include <util/generic/singleton.h>


namespace NAlice::NVideoCommon {

class TRestreamedChannelsData {
public:
    void Init();
    static TRestreamedChannelsData& Instance();

    const THashMap<TString, const NSc::TValue>& GetRestreamedChannelsMap() const {
        return RestreamedChannelsMap;
    }
    TMaybe<NSc::TValue> GetRestreamedChannelInfo(TStringBuf channel) const;

private:
    Y_DECLARE_SINGLETON_FRIEND();
    TRestreamedChannelsData() = default;

private:
    THashMap<TString, const NSc::TValue> RestreamedChannelsMap;
};

} // namespace NAlice::NVideoCommon
