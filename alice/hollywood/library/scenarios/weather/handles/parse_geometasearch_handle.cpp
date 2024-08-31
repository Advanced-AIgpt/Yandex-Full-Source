#include "parse_geometasearch_handle.h"

#include <alice/hollywood/library/scenarios/weather/request_helper/geometasearch.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

void TWeatherParseGeometasearchHandle::Do(TScenarioHandleContext& ctx) const {
    const TFrame frame = TFrame::FromProto(ctx.ServiceCtx.GetOnlyProtobufItem<TSemanticFrame>("semantic_frame"));
    TPtrWrapper<TSlot> where = frame.FindSlot("where");
    Y_ENSURE(where);

    const TGeometasearchRequestHelper<ERequestPhase::After> geoMetaSearch{ctx};
    TryAddWeatherGeoId(ctx.Ctx.Logger(), ctx.ServiceCtx, geoMetaSearch.TryParseGeoId(where->Value.AsString()));
}

}  // namespace NAlice::NHollywood::NWeather
