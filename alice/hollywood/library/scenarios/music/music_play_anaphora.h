#pragma once

#include <alice/hollywood/library/request/request.h>

#include <contrib/libs/protobuf/src/google/protobuf/struct.pb.h>

namespace NAlice::NHollywood::NMusic {

using TProtoList = google::protobuf::ListValue;
using TProtoStruct = google::protobuf::Struct;
using TProtoValue = google::protobuf::Value;

// checks that music_play_anaphora is relevant and returns the currently playing track info
const TProtoStruct* CheckAndGetMusicPlayAnaphoraTrack(const TFrame* frame, const TScenarioRunRequestWrapper& request);

TFrame TransformMusicPlayAnaphora(const TFrame& musicPlayAnaphoraFrame, const TProtoStruct& currentTrack);

} // namespace NAlice::NHollywood::NMusic
