#include "util.h"

namespace NAlice::NHollywood::NTvChannelsEfir {

TString GetChannelName(const TScenarioRunRequestWrapper& request) {
    if (request.Input().FindSemanticFrame(SHOW_CHANNEL_BY_NAME_FRAME)) {
        auto frame = request.Input().CreateRequestFrame(SHOW_CHANNEL_BY_NAME_FRAME);
        if (const auto slot = frame.FindSlot(CHANNEL_NAME_SLOT)) {
            return slot->Value.AsString();
        }
    }
    return TString{};
}

} // namespace NAlice::NHollywood::NTvChannelsEfir
