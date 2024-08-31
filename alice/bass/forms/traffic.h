#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/libs/globalctx/fwd.h>

#include <library/cpp/scheme/fwd.h>

#include <util/generic/strbuf.h>

namespace NBASS {
namespace NTraffic {

namespace NImpl {

NSc::TValue TransformTrafficResponseToOldApiVersion(const NSc::TValue& response, ui64 timestamp,
                                                    const NGeobase::TLookup& geoBase);

} // namespace NImpl

/*
Fills trafficJson with information about traffic and changes theGeo so that it points to the city for which information was found about traffic jams
*/
TResultValue GetTrafficInfo(TContext& ctx, NSc::TValue* trafficJson, NGeobase::TRegion& region);

} // namespace NTraffic

/** It shows traffic info for cities. Check where slot and if it is empty get location from user's position.
 * It downloads traffic data periodically and then use this result as a response.
 * Input slots: where (location)
 * Output slots:
 *   resolved_where (resolved geo from slot <where> or from coords);
 *   traffic_info (main answer of traffic info)
 @code
{
  "meta": {
    "uuid": "12345678",
    "epoch": 1506347530,
    "tz": "Europe/Moscow",
    "location": {
      "lat": 55.733771,
      "lon": 37.587937
    },
    "uid": 0
  },
  "form": {
    "slots": [
      {
        "name": "where",
        "optional": false,
        "type": "string",
        "value": "на льва толстого"
      }
    ],
    "name": "personal_assistant.scenarios.show_traffic"
  }
}
 @endcode
 */
class TTrafficFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers, IGlobalContext& globalCtx);

private:
    void AddShowTrafficNaviCommandBlocks(TContext& ctx, TStringBuf naviUrl);
};

} // namespace NBASS
