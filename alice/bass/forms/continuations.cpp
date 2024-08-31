#include "continuations.h"

#include <alice/library/json/json.h>
#include <alice/megamind/protos/common/events.pb.h>

namespace NBASS {

TResultValue FillContext(TContext::TPtr& context, NSc::TValue value, TGlobalContextPtr globalContext, NSc::TValue meta,
                         const TString& authHeader, const TString& appInfoHeader, const TString& fakeTimeHeader,
                         const TMaybe<TString>& userTicketHeader, const NSc::TValue& configPatch) {

    // Create context.
    TMaybe<NAlice::TEvent> speechKitEvent;
    if (meta.Has("event")) {
        const NJson::TJsonValue json = meta["event"].ToJsonValue();
        NAlice::TEvent tmpEvent;
        if (NAlice::JsonToProto(json, tmpEvent).ok()) {
            speechKitEvent = tmpEvent;
        }
    }
    TContext::TInitializer initData(globalContext, std::move(value["req_id"]), authHeader, appInfoHeader,
                                    fakeTimeHeader, userTicketHeader, std::move(speechKitEvent));
    initData.ConfigPatch = configPatch;
    initData.Id = std::move(value["form_id"]);
    NSc::TValue contextValue = std::move(value["context"]);
    contextValue["meta"] = std::move(meta);
    return TContext::FromJson(contextValue, initData, &context);
}

} // namespace NBASS
