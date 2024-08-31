#include "afisha_proxy.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>


namespace NBASS {
namespace {
constexpr TStringBuf FLAG_AFISHA_POI_EVENTS = "afisha_poi_events";

const TStringBuf PLACE_REPERTORY_QUERY_NAME = R"(alice-place-repertory)";

const TStringBuf PLACE_REPERTORY_TEMPLATE = R"(
    query ($placeId: ID!, $paging: PagingInput) {
      place(id: $placeId) {
        url(domain:true)
      }
      placeRepertory(id: $placeId, paging: $paging) {
        items {
          event {
            url(domain:true)
            tags(type: "genre") {
              name
            }
            image {
              bgColor
              linkColor
              image(size: s272x156_crop) {
                url
              }
            }
            title
            kinopoisk {
              rating
            }
            userRating {
              overall {
                value
                count
              }
            }
          }
          scheduleInfo {
            preview(short:false) {
              text
            }
          }
        }
        paging {
          total
          offset
          limit
        }
      }
    })";
}  // namespace

namespace NAfisha {

TMaybe<NSc::TValue> TAfishaProxy::DoRequest(const NSc::TValue& postData, const TStringBuf queryName) {
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().Afisha().Request();
    request->SetMethod("POST");
    request->SetBody(postData.ToJsonSafe());
    request->AddCgiParam("query_name", queryName);
    request->SetContentType("application/json");
    NHttpFetcher::TResponse::TRef handler = request->Fetch()->Wait();
    if (handler->IsError()) {
        LOG(ERR) << "Afisha place request Error " << queryName << handler->GetErrorText() << Endl;
        return Nothing();
    }
    NSc::TValue result = NSc::TValue::FromJson(handler->Data);
    if (result.IsNull()) {
        LOG(ERR) << "Can not parse afisha place repertory response. Response:" << handler->Data << Endl;
        return Nothing();
    }
    return result;
}

TMaybe<NSc::TValue> TAfishaProxy::GetPlaceRepertory(const TStringBuf placeId, const int limit) {
    if (!Ctx.HasExpFlag(FLAG_AFISHA_POI_EVENTS)) {
        return Nothing();
    }
    NSc::TValue paging;
    paging["limit"] = limit;
    paging["offset"] = 0;
    NSc::TValue variables;
    variables["placeId"].SetString(placeId);
    variables["paging"] = paging;
    NSc::TValue postData;
    postData["query"] = PLACE_REPERTORY_TEMPLATE;
    postData["variables"] = variables;
    return DoRequest(postData, PLACE_REPERTORY_QUERY_NAME);
}

TAfishaProxy::TAfishaProxy(TContext& ctx)
        : Ctx(ctx) {
}
}  // namespace NAfisha
}  // namespace NBASS
