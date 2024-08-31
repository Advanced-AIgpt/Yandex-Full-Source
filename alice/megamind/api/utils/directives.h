#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>

namespace NAlice::NMegamindApi {

NSpeechKit::TDirective MakeDirectiveWithTypedSemanticFrame(const TSemanticFrameRequestData& data);

} // namespace NAlice::NMegamindApi
