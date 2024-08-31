#include "directives.h"

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>

#include <alice/library/proto/protobuf.h>

#include <util/string/cast.h>

namespace NAlice::NMegamindApi {

NSpeechKit::TDirective MakeDirectiveWithTypedSemanticFrame(const TSemanticFrameRequestData& data) {
    NSpeechKit::TDirective directive{};
    directive.SetType(ToString(NMegamind::EDirectiveType::ServerAction));
    directive.SetName(TString{NMegamind::SEMANTIC_FRAME_REQUEST_NAME});
    *directive.MutablePayload() = MessageToStruct(data);
    return directive;
}

} // namespace NAlice::NMegamindApi
