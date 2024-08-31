#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywood::NMusic {

void ParseAlbum(const NJson::TJsonValue& albumJson, TMusicQueueWrapper& mq, TMusicContext& mCtx);

} // namespace NAlice::NHollywood::NMusic
