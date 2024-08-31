#include "common.h"
#include "music_play_anaphora.h"

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

namespace {

// TODO(a-square): make a fully generic struct query library

const TProtoList& GetStructList(const TProtoStruct& container, const TString& key) {
    const auto& value = container.fields().at(key);
    Y_ENSURE(value.has_list_value());
    return value.list_value();
}

const TProtoStruct& GetListStruct(const TProtoList& container, const int index) {
    Y_ENSURE(0 <= index && index < container.values_size());
    const auto& value = container.values()[index];
    Y_ENSURE(value.has_struct_value());
    return value.struct_value();
}

const TString GetStructString(const TProtoStruct& container, const TString& key) {
    const auto& value = container.fields().at(key);
    switch (value.kind_case()) {
        case TProtoValue::kNullValue:
            return {};
        case TProtoValue::kNumberValue:
            return ToString(value.number_value());
        case TProtoValue::kStringValue:
            return value.string_value();
        case TProtoValue::kBoolValue:
            return value.bool_value() ? "true" : "false";
        default:
            ythrow yexception() << "Value doesn't have a primitive type";
    }
}

} // namespace

// checks that music_play_anaphora is relevant and returns the currently playing track info
const TProtoStruct* CheckAndGetMusicPlayAnaphoraTrack(const TFrame* frame, const TScenarioRunRequestWrapper& request) {
    // no frame => not relevant
    if (!frame) {
        return nullptr;
    }

    // disabled => not relevant
    if (!request.HasExpFlag(NExperiments::EXP_HOLLYWOOD_MUSIC_PLAY_ANAPHORA)) {
        return nullptr;
    }

    // no track is playing => not relevant
    const auto& currentlyPlaying = request.BaseRequestProto().GetDeviceState().GetMusic().GetCurrentlyPlaying();
    if (!currentlyPlaying.HasRawTrackInfo()) {
        return nullptr;
    }

    // empty track info => not relevant
    const auto& trackInfo = currentlyPlaying.GetRawTrackInfo();
    if (trackInfo.fields_size() == 0) {
        return nullptr;
    }

    return &trackInfo;
}

TFrame TransformMusicPlayAnaphora(const TFrame& musicPlayAnaphoraFrame, const TProtoStruct& currentTrack) {
    // all slots should be forwarded, except for target_type
    auto frame = musicPlayAnaphoraFrame;
    frame.SetName(MUSIC_PLAY_FRAME);

    // used for setting the intent feature down the line
    frame.AddSlot({ORIGINAL_INTENT, "string", TSlot::TValue{MUSIC_PLAY_ANAPHORA_FRAME}});

    const bool needSimilar = (frame.FindSlot("need_similar") != nullptr);
    TString targetType;
    if (const auto targetTypeSlot = frame.FindSlot("target_type")) {
        targetType = targetTypeSlot->Value.AsString();
        frame.RemoveSlots("target_type");
    }

    if (targetType == "album") {
        const auto& albums = GetStructList(currentTrack, "albums");
        const auto& firstAlbum = GetListStruct(albums, 0);
        frame.AddSlot({"album_id", "string", TSlot::TValue{GetStructString(firstAlbum, "id")}});
    } else if (targetType == "artist" || (!targetType && !needSimilar)) {
        const auto& artists = GetStructList(currentTrack, "artists");
        const auto& firstArtist = GetListStruct(artists, 0);
        frame.AddSlot({"artist_id", "string", TSlot::TValue{GetStructString(firstArtist, "id")}});
    } else {
        frame.AddSlot({"track_id", "string", TSlot::TValue{GetStructString(currentTrack, "id")}});
    }

    return frame;
}

} // namespace NAlice::NHollywood::NMusic
