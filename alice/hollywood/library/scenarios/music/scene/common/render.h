#pragma once

#include <alice/hollywood/library/framework/core/render.h>
#include <alice/hollywood/library/framework/core/return_types.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

void DoCommonRender(const TMusicScenarioRenderArgsCommon&, TRender&);
TRetResponse CommonRender(const TMusicScenarioRenderArgsCommon&, TRender&);

} // namespace NAlice::NHollywoodFw::NMusic
