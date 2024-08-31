#include "fm_radio.h"
#include "url_build.h"

#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/radio.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <util/string/cast.h>

namespace  NBASS {
namespace  NAutomotive {

namespace {
i32 ChooseSupportedRegionId(TContext& ctx, const TFMRadioDatabase& fmdb) {
    if (ctx.Meta().HasLocation()) {
        return fmdb.GetNearest(ctx.Meta().Location().Lat(), ctx.Meta().Location().Lon());
    }
    return 0;
}
} // namespace

TResultValue HandleFMRadio(TContext& ctx, const TFMRadioDatabase& fmdb) {
    i32 regionId = GetRegionId(ctx, fmdb);

    if (regionId == 0) {
        ctx.AddAttention(TStringBuf("no_region_fm_db"), {});
        return TResultValue();
    }
    // TODO: what if both slots are not empty?
    if (!IsSlotEmpty(ctx.GetSlot(NRadio::FM_RADIO_SLOT_NAME))) {
        return FMRadioCommandByName(ctx, fmdb, ToString(ctx.GetSlot(NRadio::FM_RADIO_SLOT_NAME)->Value.GetString()), regionId);
    } else if (!IsSlotEmpty(ctx.GetSlot(NRadio::FM_FREQ_SLOT_NAME))) {
         // we store frequency as TString, i.e. 106.6 => "10660"
        return FMRadioCommandByFreq(ctx, fmdb, ToString(trunc(static_cast<float>(ctx.GetSlot(NRadio::FM_FREQ_SLOT_NAME)->Value.ForceNumber()) * 100)), regionId);
    } else if (!IsSlotEmpty(ctx.GetSlot(NRadio::FM_RADIO_FREQ_SLOT_NAME))) {
        return FMRadioCommandByFreq(ctx, fmdb, ToString(trunc(static_cast<float>(ctx.GetSlot(NRadio::FM_RADIO_FREQ_SLOT_NAME)->Value.ForceNumber()) * 100)), regionId);
    } else {
        return TError(
            TError::EType::INVALIDPARAM,
            TStringBuf("fm_radio_error")
        );
    }
}

i32 GetRegionId(TContext& ctx, const TFMRadioDatabase& fmdb) {
    const auto& deviceState = ctx.Meta().DeviceState();
    if (deviceState.HasFmRadio() && deviceState.FmRadio().HasRegionId() && fmdb.HasRegion(deviceState.FmRadio().RegionId())) {
        return deviceState.FmRadio().RegionId();
    }
    return ChooseSupportedRegionId(ctx, fmdb);
}

} // NAutomotive
} // NBASS
