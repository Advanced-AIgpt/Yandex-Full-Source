#include "service.h"
#include "servant.h"

namespace NAlice::NCuttlefish::NAppHostServices {

NThreading::TPromise<void> AudioSeparator(NAppHost::TServiceContextPtr ctx, TLogContext logContext) {
    TIntrusivePtr<TAudioSeparator> stream(new TAudioSeparator(ctx, std::move(logContext)));
    stream->OnNextInput();
    return stream->GetFinishPromise();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices
