#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <search/begemot/rules/alice/microintents/proto/alice_microintents.pb.h>

namespace NBg {

bool BuildMicrointentsFrame(const NProto::TAliceMicrointentsResult& microintentsResult, NAlice::TSemanticFrame& resultFrame);

} // namespace NBg
