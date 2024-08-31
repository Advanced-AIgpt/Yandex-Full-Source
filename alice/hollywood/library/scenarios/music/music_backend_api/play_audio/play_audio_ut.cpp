#include "play_audio.h"

#include <alice/hollywood/library/scenarios/music/time_util/time_util.h>

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

auto MakeProto() {
    return TPlayAudioEventBuilder().From("from")
        .TrackId("trackId")
        .ArtistId("artistId")
        .PlayId("playId")
        .TotalPlayedSeconds(123.0f)
        .TrackLengthSeconds(321.0f)
        .EndPositionSeconds(456.0f)
        .PlaylistId("playlistId")
        .RadioSessionId("foobarbaz")
        .Incognito(true).BuildProto();
}

Y_UNIT_TEST_SUITE(PlayAudioTest) {

Y_UNIT_TEST(EventBuilder) {
    auto proto = MakeProto();

    UNIT_ASSERT_EQUAL(proto.GetFrom(), "from");
    UNIT_ASSERT_EQUAL(proto.GetTrackId(), "trackId");
    UNIT_ASSERT_EQUAL(proto.GetArtistId(), "artistId");
    UNIT_ASSERT_EQUAL(proto.GetPlayId(), "playId");
    UNIT_ASSERT_EQUAL(proto.GetTotalPlayedSeconds(), 123.0f);
    UNIT_ASSERT_EQUAL(proto.GetTrackLengthSeconds(), 321.0f);
    UNIT_ASSERT_EQUAL(proto.GetEndPositionSeconds(), 456.0f);
    UNIT_ASSERT_EQUAL(proto.GetPlaylistId(), "playlistId");
    UNIT_ASSERT_EQUAL(proto.GetRadioSessionId(), "foobarbaz");
    UNIT_ASSERT_EQUAL(proto.GetIncognito(), true);
}

Y_UNIT_TEST(JsonBuilder) {
    auto now = TInstant::Now();
    auto jsonStr = TPlayAudioJsonBuilder(MessageToStruct(MakeProto()))
        .PlayedSec(333.0f)
        .DurationSec(111.0f)
        .PositionSec(222.0f)
        .Timestamp(now)
        .BuildJsonString();

    auto jsonArr = JsonFromString(jsonStr)["plays"];
    UNIT_ASSERT(jsonArr.IsArray());
    auto& json = jsonArr[0];

    UNIT_ASSERT_EQUAL(json["from"].GetString(), "from");
    UNIT_ASSERT_EQUAL(json["trackId"].GetString(), "trackId");
    UNIT_ASSERT_EQUAL(json["artistId"].GetString(), "artistId");
    UNIT_ASSERT_EQUAL(json["playId"].GetString(), "playId");
    UNIT_ASSERT_EQUAL(json["radioSessionId"].GetString(), "foobarbaz");
    UNIT_ASSERT_EQUAL(json["endPositionSeconds"].GetDouble(), 222.0f);
    UNIT_ASSERT_EQUAL(json["totalPlayedSeconds"].GetDouble(), 333.0f);
    UNIT_ASSERT_EQUAL(json["trackLengthSeconds"].GetDouble(), 111.0f);
    UNIT_ASSERT_EQUAL(json["timestamp"].GetString(), FormatTInstant(now));
    UNIT_ASSERT_EQUAL(json["incognito"].GetBoolean(), true);
}

}

} // namespace

} //namespace NAlice::NHollywood::NMusic
