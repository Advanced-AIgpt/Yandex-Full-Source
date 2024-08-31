#include "channel_item.h"

#include <alice/library/video_common/video_helper.h>
#include <alice/protos/data/video/video.pb.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace {

bool IsPersonalChannel(const NJson::TJsonValue& channel) {
    return channel["channel_type"].GetString() == "personal";
}

TString BuildTvEpisodeName(const NJson::TJsonValue& episode) {
    TStringBuf programTitle = episode["program_title"].GetString();
    TStringBuf title = episode["title"].GetString();

    if (programTitle == title) {
        return TString(title);
    } else {
        return TStringBuilder() << programTitle << (programTitle ? ". " : "") << title;
    }
}

bool IsDeepHD(const NJson::TJsonValue& channel) {
    if (!channel["streams"].IsArray()) {
        return false;
    }
    for (const auto& stream : channel["streams"].GetArray()) {
        for (const auto& option : stream["options"].GetArray()) {
            if (option.GetString() == "hires") {
                return true;
            }
        }
    }
    return false;
}

} // namespace

namespace NAlice::NHollywood::NTvChannelsEfir {

TMaybe<NAlice::TVideoItem> ParseChannelJson(const NJson::TJsonValue& channel) {
    NAlice::TVideoItem videoItem;
    const auto& episode = channel["episode"];
    videoItem.SetProviderItemId(channel["content_id"].GetString());
    videoItem.SetProviderName(TString(NAlice::NVideoCommon::PROVIDER_STRM));
    videoItem.SetType(ToString(NAlice::NVideoCommon::EContentType::TvStream));
    videoItem.SetChannelType(channel["channel_type"].GetString());

    auto* tvStreamInfo = videoItem.MutableTvStreamInfo();
    tvStreamInfo->SetChannelType(videoItem.GetChannelType());
    tvStreamInfo->SetIsPersonal(IsPersonalChannel(channel));
    tvStreamInfo->SetTvEpisodeId(episode["content_id"].GetString());
    TString episodeName = BuildTvEpisodeName(episode);
    tvStreamInfo->SetTvEpisodeName(episodeName);
    videoItem.SetTvEpisodeName(episodeName);
    tvStreamInfo->SetDeepHD(IsDeepHD(channel));

    videoItem.SetName(channel["title"].GetString());
    videoItem.SetDescription(channel["description"].GetString());
    videoItem.SetDescription(channel["description"].GetString());
    const auto thumbnail = NAlice::NVideoCommon::BuildThumbnailUri(channel["thumbnail"].GetString());
    videoItem.SetThumbnailUrl16x9(thumbnail);
    videoItem.SetThumbnailUrl16x9Small(thumbnail);
    if (const auto* relevance = channel.GetValueByPath("meta.weight")) {
        videoItem.SetRelevance(relevance->GetUInteger());
    }

    return MakeMaybe<NAlice::TVideoItem>(std::move(videoItem));
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
