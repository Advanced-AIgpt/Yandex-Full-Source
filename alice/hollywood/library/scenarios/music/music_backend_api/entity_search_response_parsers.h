#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NMusic {

void ParsePlaylistSearchResponse(const TStringBuf response, TMusicQueueWrapper& mq, TString& playlistNameOut);

void ParseSpecialPlaylistResponse(const TStringBuf response, TMusicQueueWrapper& mq, TString& playlistNameOut);

void ParseNoveltyAlbumSearchResponse(const TStringBuf response, TMusicQueueWrapper& mq);

} // namespace NAlice::NHollywood::NMusic
