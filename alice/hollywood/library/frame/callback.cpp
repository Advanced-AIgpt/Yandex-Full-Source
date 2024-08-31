#include "callback.h"

#include <alice/library/json/json.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice {
namespace NHollywood {

TMaybe<TFrame> GetCallbackFrame(const NScenarios::TCallbackDirective* callback) {
    if (!callback || callback->GetName() != FRAME_CALLBACK) {
        return Nothing();
    }

    return TFrame::FromProto(JsonToProto<TSemanticFrame>(
        JsonFromString(callback->GetPayload().fields().at("frame").string_value())));
}

NScenarios::TCallbackDirective ToCallback(const TSemanticFrame& frame) {
    NScenarios::TCallbackDirective callback;
    callback.SetName(TString{FRAME_CALLBACK});
    (*callback.MutablePayload()->mutable_fields())["frame"].set_string_value(JsonStringFromProto(frame));
    return callback;
}

} // namespace NHollywood
} // namespace NAlice
