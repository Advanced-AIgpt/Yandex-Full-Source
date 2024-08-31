#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NTvChannels {
constexpr TStringBuf FRAME = "alice.switch_tv_channel";
constexpr TStringBuf FRAME_V2_TEXT = "alice.switch_tv_channel2_text";
constexpr TStringBuf FRAME_V2_NUM = "alice.switch_tv_channel2_num";
constexpr TStringBuf FRAME_SWITCH_SF = "alice.switch_tv_channel_sf";
constexpr TStringBuf NLG_TEMPLATE_NAME = "tv_channels";
constexpr TStringBuf FORM_V2_ENABLED_FLAG = "tv_channels_form_v2_enabled";

std::unique_ptr<TScenarioRunResponse> RenderIrrelevant(TRTLogger& logger, const TScenarioHandleContext& ctx,
                                                       const TScenarioRunRequestWrapper& request,
                                                       TMaybe<TFrame> frame = Nothing());

} // namespace NAlice::NHollywood::NTvChannels
