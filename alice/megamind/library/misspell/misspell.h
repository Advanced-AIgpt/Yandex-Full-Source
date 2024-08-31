#pragma once

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/protos/common/misspell.pb.h>

#include <alice/library/network/request_builder.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

TSourcePrepareStatus CreateMisspellRequest(const TString& utterance, bool processMisspell, NNetwork::IRequestBuilder& request);
TErrorOr<TMisspellProto> ParseMisspellResponse(TStringBuf content);

template<typename TContextView>
bool NeedMisspellRequest(const IEvent* event, const TContextView& contextView) {
    if (!event) {
        return false;
    }
    if (event->IsTextInput()) {
        return true;
    }
    if (event->IsVoiceInput()) {
        return contextView.HasExpFlag(EXP_ENABLE_VOICE_MISSPELL);
    }

    return false;
}

} // namespace NAlice::NMegamind
