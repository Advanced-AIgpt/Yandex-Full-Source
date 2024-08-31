#pragma once

#include <alice/library/frame/description.h>
#include <search/begemot/rules/granet_config/proto/granet_config.pb.h>

namespace NBg {

NAlice::TFrameDescriptionMap ReadFrameDescriptionMap(const NProto::TGranetConfig& config, bool taggerOnly = false);

} // namespace NBg
