#pragma once

#include <alice/library/frame/description.h>
#include <alice/nlu/libs/frame/slot.h>
#include <alice/megamind/protos/common/frame.pb.h>
#include <search/begemot/rules/granet/proto/granet.pb.h>

#include <util/generic/vector.h>

namespace NBg {
    NAlice::TRecognizedSlot GetRecognizedSlot(const NBg::NProto::TGranetTag& granetTag);

    TVector<TString> GetGranetTokenTexts(const TVector<NProto::TGranetToken>& granetTokens);

    NAlice::TSemanticFrame ConvertFormToSemanticFrame(
        const NBg::NProto::TGranetForm& granetForm,
        const NAlice::TFrameDescription* description = nullptr
    );
} // namespace NBg
