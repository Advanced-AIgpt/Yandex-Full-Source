#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

NAppHostHttp::THttpRequest PrepareHttpRequest(const TMusicScenarioSceneArgsFmRadio& sceneArgs, const TRequest& request);

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio
