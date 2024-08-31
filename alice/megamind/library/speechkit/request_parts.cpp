#include "request_parts.h"

#include "request_build.h"

#include <util/digest/multi.h>

namespace NAlice::NMegamind {
namespace {


// On the fact that this function returns the same results from the same query on all releases
// depends Alice's testing and acceptance processes
// !! PLEASE DON'T CHANGE THE LOGIC OF RANDOM SEED CALCULATION !!
ui64 CalcSeed(TStringBuf salt, const TSpeechKitRequestProto& proto, const TEvent& event) {
    const auto& header = proto.GetHeader();

    if (header.HasRandomSeed()) {
        return header.GetRandomSeed();
    }

    const auto hash = MultiHash(header.GetRequestId(), salt);

    // if this assert fails, we need to add HasType() to the check
    static_assert(EEventType::voice_input != 0);
    if (event.GetType() == EEventType::voice_input) {
        // we have different seed for voice input for a historical reason
        return MultiHash(hash, /* HypothesisNumber= */ 0, /* EndOfUtterance */ true);
    }

    return hash;
}

} // namespace

// TRequestParts --------------------------------------------------------------
// static
TErrorOr<TRequestParts> TRequestParts::Create(TSpeechKitInitContext& ctx, TRequestComponentsView<TEventComponent> skr) {
    return TRequestParts{ctx, skr};
}

TRequestParts::TRequestParts(TSpeechKitInitContext& ctx, TRequestComponentsView<TEventComponent> skrView)
    : Proto_{ctx.Proto}
    , Headers_{ctx.Headers}
    , Path_{ctx.Path}
    , Seed_{CalcSeed(ctx.RngSalt, Proto(), skrView.Event())}
{
    // Prevent to send 'expect: 100-continue' header to vins.
    Headers_.RemoveHeader("expect");
}

} // namespace NAlice::NMegamind
