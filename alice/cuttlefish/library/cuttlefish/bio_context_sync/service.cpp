#include <alice/cuttlefish/library/cuttlefish/bio_context_sync/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    void BioContextSync(NAppHost::IServiceContext &ctx, TLogContext logContext) {
        TSourceMetrics metrics(ctx, "check_bio_context_update");
        BioContextSyncInternal(ctx, MakeHolder<TBioContextSyncProcessor>(logContext, metrics), logContext, metrics);
    }

}
