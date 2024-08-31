#include "url_build.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/libs/radio/fmdb.h>

#include <util/string/builder.h>

namespace NBASS {
namespace NAutomotive {

TResultValue FMRadioCommandByName(TContext& ctx, const TFMRadioDatabase& FMDB, const TString& radioName, const i32 regionId) {
    if (FMDB.HasRadioByRegion(regionId, radioName)) {
        TDirectiveBuilder<TAutoFmRadioDirective> builder(TStringBuf("fm_radio"));
        builder.InsertParam("name", radioName);
        builder.InsertParam("frequency", FMDB.GetRadioByRegion(regionId, radioName));
        return builder.AddDirective(ctx);
    }
    ctx.AddAttention(TStringBuf("no_fm_station"), {});
    return TResultValue();
}

TResultValue FMRadioCommandByFreq(TContext& ctx, const TFMRadioDatabase& FMDB, const TString& radioFreq, const i32 regionId) {
    TDirectiveBuilder<TAutoFmRadioDirective> builder(TStringBuf("fm_radio"));
    builder.InsertParam("frequency", radioFreq);
    if (FMDB.HasRadioByRegion(regionId, radioFreq)) {
        builder.InsertParam("name", FMDB.GetRadioByRegion(regionId, radioFreq));
    }
    return builder.AddDirective(ctx);
}


} // NBASS
} // NAutomotive
