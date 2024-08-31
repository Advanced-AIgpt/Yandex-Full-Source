#include "stage_timers.h"

#include <alice/megamind/library/apphost_request/item_names.h>

#include <alice/megamind/library/apphost_request/protos/stage_timestamp.pb.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace NAlice::NMegamind {

void TStageTimersAppHost::Upload(const TStringBuf name, TInstant at) {
    NMegamindAppHost::TStageTimestamp stageTimestamp;
    stageTimestamp.SetName(ToString(name));
    stageTimestamp.SetTimeMs(at.MilliSeconds());
    ItemProxyAdapter_.PutIntoContext(stageTimestamp, AH_STAGE_TIMESTAMP);
}

void TStageTimersAppHost::LoadFromContext(IAppHostCtx& ahCtx) {
    auto callback = [this](const NMegamindAppHost::TStageTimestamp& stageTimestamp) {
        this->Add(stageTimestamp.GetName(), TInstant::MilliSeconds(stageTimestamp.GetTimeMs()));
    };
    ahCtx.ItemProxyAdapter().ForEachCached<NMegamindAppHost::TStageTimestamp>(AH_STAGE_TIMESTAMP, callback);
}

} // namespace NAlice::NMegamind
