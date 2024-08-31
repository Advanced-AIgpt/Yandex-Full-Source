#pragma once

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NMusic {

const TString RENDER_SONG_TEXT_PUSH_TITLE = "push_title";
const TString RENDER_SONG_TEXT_PUSH_TEXT = "push_text";
const TString SONG_TEXT_PUSH_POLICY = "unlimited_policy";
const TString SONG_TEXT_PUSH_TAG = "alice_music_song_text";

bool IsPlayerCommandRelevant(const TRunRequest& request);

TString GenerateSendSongPushUri(TStringBuf trackId);

} // namespace NAlice::NHollywoodFw::NMusic
