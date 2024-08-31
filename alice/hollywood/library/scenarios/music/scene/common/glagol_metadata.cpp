#include "glagol_metadata.h"

#include <util/generic/hash.h>

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

using NAlice::NHollywood::NMusic::ERepeatType;
using NAlice::NHollywood::NMusic::TContentId;
using NAlice::NHollywood::NMusic::TMusicQueueWrapper;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

constexpr TStringBuf SET_GLAGOL_METADATA_DIRECTIVE_NAME = "set_glagol_metadata";

NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType GetMetaContentType(const NHollywood::NMusic::TContentId_EContentType type) {
    static const THashMap<NHollywood::NMusic::TContentId_EContentType, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType> CONTENT_TYPE_MAP{
        {NHollywood::NMusic::TContentId_EContentType_Track, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Track},
        {NHollywood::NMusic::TContentId_EContentType_Album, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Album},
        {NHollywood::NMusic::TContentId_EContentType_Artist, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Artist},
        {NHollywood::NMusic::TContentId_EContentType_Playlist, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Playlist},
        {NHollywood::NMusic::TContentId_EContentType_Radio, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Radio},
        {NHollywood::NMusic::TContentId_EContentType_Generative, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Generative},
        {NHollywood::NMusic::TContentId_EContentType_FmRadio, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_FmRadio},
    };
    if (const auto* iter = CONTENT_TYPE_MAP.FindPtr(type)) {
        return *iter;
    }
    ythrow yexception() << "Unsupported ContentId type=" << TContentId_EContentType_Name(type);
}

NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode GetMetaRepeatMode(const ERepeatType type) {
    static const THashMap<ERepeatType, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode> REPEAT_MODE_MAP{
        {ERepeatType::RepeatNone, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_None},
        {ERepeatType::RepeatTrack, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_One},
        {ERepeatType::RepeatAll, NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_All},
    };
    if (const auto* iter = REPEAT_MODE_MAP.FindPtr(type)) {
        return *iter;
    }
    ythrow yexception() << "Unsupported repeat type=" << ERepeatType_Name(type);
}

class TGlagolMetadataBuilder {
public:
    void SetContentId(const TContentId& contentId) {
        auto& musicMetadata = *Metadata_.MutableMusicMetadata();
        musicMetadata.SetId(contentId.GetId());
        musicMetadata.SetType(GetMetaContentType(contentId.GetType()));
    }

    void SetShuffled(bool shuffled) {
        auto& musicMetadata = *Metadata_.MutableMusicMetadata();
        musicMetadata.SetShuffled(shuffled);
    }

    void SetRepeatType(ERepeatType repeatType) {
        auto& musicMetadata = *Metadata_.MutableMusicMetadata();
        musicMetadata.SetRepeatMode(GetMetaRepeatMode(repeatType));
    }

    void SetPreviousTrack(TStringBuf trackId) {
        auto& prevTrackInfo = *Metadata_.MutableMusicMetadata()->MutablePrevTrackInfo();
        prevTrackInfo.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        prevTrackInfo.SetId(trackId.data(), trackId.size());
    }

    void SetNextTrack(TStringBuf trackId) {
        auto& nextTrackInfo = *Metadata_.MutableMusicMetadata()->MutableNextTrackInfo();
        nextTrackInfo.SetStreamType(NAlice::NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        nextTrackInfo.SetId(trackId.data(), trackId.size());
    }

    decltype(auto) Build() && {
        return std::move(Metadata_);
    }

private:
    NScenarios::TAudioPlayDirective::TAudioPlayMetadata::TGlagolMetadata Metadata_;
};

} // namespace

NScenarios::TAudioPlayDirective::TAudioPlayMetadata::TGlagolMetadata BuildGlagolMetadata(const NHollywood::NMusic::TMusicQueueWrapper& musicQueue) {
    TGlagolMetadataBuilder builder;
    builder.SetContentId(musicQueue.ContentId());

    if (const TMaybe<bool> shuffled = musicQueue.GetMetadataShuffled()) {
        builder.SetShuffled(*shuffled);
    }

    if (const TMaybe<ERepeatType> repeatType = musicQueue.GetMetadataRepeatType()) {
        builder.SetRepeatType(*repeatType);
    }

    if (musicQueue.HasPreviousItem()) {
        builder.SetPreviousTrack(musicQueue.PreviousItem().GetTrackId());
    }

    if (musicQueue.HasNextItem()) {
        builder.SetNextTrack(musicQueue.NextItem().GetTrackId());
    }

    return std::move(builder).Build();
}

NScenarios::TSetGlagolMetadataDirective BuildSetGlagolMetadataDirective(const TMusicQueueWrapper& musicQueue) {
    NScenarios::TSetGlagolMetadataDirective directive;
    directive.SetName(SET_GLAGOL_METADATA_DIRECTIVE_NAME.data(), SET_GLAGOL_METADATA_DIRECTIVE_NAME.size());
    *directive.MutableGlagolMetadata() = BuildGlagolMetadata(musicQueue);
    return std::move(directive);
}

} // namespace NAlice::NHollywoodFw::NMusic
