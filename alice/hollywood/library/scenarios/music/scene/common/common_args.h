#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/scenarios/music/proto/music.pb.h>

#include <alice/protos/data/scenario/music/content_id.pb.h>

//#include <util/generic/xrange.h>

namespace NAlice::NHollywoodFw::NMusic {

void FillCommonArgs(TMusicScenarioSceneArgsCommon& commonArgs, const TRunRequest& request);

// TODO(sparkle): use protobuf message reflection instead of templated code
template<typename TTsf>
void FillCommonArgs(TMusicScenarioSceneArgsCommon& commonArgs, const TRunRequest& request, const TTsf& tsf) {
    FillCommonArgs(commonArgs, request);

    // fill slot values
    if constexpr (requires { tsf.GetDisableNlg; }) {
        commonArgs.MutableFrame()->SetDisableNlg(tsf.GetDisableNlg().GetBoolValue());
    }
    if constexpr (requires { tsf.GetContentId; }) {
        if (tsf.HasContentId()) {
            *commonArgs.MutableFrame()->MutableContentId() = tsf.GetContentId().GetContentIdValue();
        }
    }

    // TODO(sparkle): set original frame
    //auto& tsfField = *commonArgs.MutableOriginalSemanticFrame();
    //const auto* descriptor = tsfField.GetDescriptor();
    //const auto* reflection = tsfField.GetReflection();

    //for (const int i : xrange(descriptor->field_count())) {
        //const auto* fieldDescriptor = descriptor->field(i);
        //if (fieldDescriptor->message_type() == tsf.GetDescriptor()) {
            //reflection->MutableMessage(&tsfField, fieldDescriptor)->CopyFrom(tsf);
        //}
    //}
}

// TODO(sparkle): make common method for all protobufs with reflection
const NData::NMusic::TContentId* TryGetContentId(const TMusicScenarioSceneArgsCommon& commonArgs);

} // namespace NAlice::NHollywoodFw::NMusic
