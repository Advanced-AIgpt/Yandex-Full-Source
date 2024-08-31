#include "restreamed_data.h"

#include <library/cpp/resource/resource.h>

#include <util/system/guard.h>
#include <util/generic/maybe.h>

namespace NAlice::NVideoCommon {

void TRestreamedChannelsData::Init() {
    NSc::TValue channelsJson;
    try {
        NSc::TJsonOpts jsonOpt;
        jsonOpt.AllowComments = true;
        NSc::TValue::FromJson(channelsJson, NResource::Find("restreamed_channels.json"), jsonOpt);
    } catch (yexception e) {
        Cerr << "Can't find restreamed channels resource: " << e.what() << Endl;
        return;
    }

    for (const auto& channel : channelsJson.GetDict()) {
        RestreamedChannelsMap.emplace(channel);
    }
}

TRestreamedChannelsData& TRestreamedChannelsData::Instance() {
    return *Singleton<TRestreamedChannelsData>();
}

TMaybe<NSc::TValue> TRestreamedChannelsData::GetRestreamedChannelInfo(TStringBuf channel) const {
    try {
        return RestreamedChannelsMap.at(channel);
    } catch (yexception e) {
        Cerr << "Restreamed channel with id <" << channel << "> was not found." << Endl;
    }
    return Nothing();
}

} // namespace NAlice::NVideoCommon
