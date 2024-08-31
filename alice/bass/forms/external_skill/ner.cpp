#include "ner.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {
namespace NExternalSkill {

void TNerInfo::Request(TContext& ctx) {
    const TSlot* reqSlot = ctx.GetSlot("request", "string");
    if (IsSlotEmpty(reqSlot)) {
        return;
    }

    NHttpFetcher::TRequestPtr request = ctx.GetSources().NerApi().Request();

    NSc::TValue content;
    content["utterance"].SetString(reqSlot->Value.GetString());

    TContext::TSlot* slotSkill = ctx.GetSlot(TStringBuf("skill_id"));
    if (!IsSlotEmpty(slotSkill)) {
        content["skill_id"].SetString(slotSkill->Value.GetString());
    }

    request->SetBody(content.ToJson(), TStringBuf("POST"));
    request->SetContentType(TStringBuf("application/json"));
    Req = request->Fetch();
}

TMaybe<NSc::TValue> TNerInfo::Response() const {
    if (!Req) {
        return Nothing();
    }

    const auto response = Req->Wait();
    if (response->IsError()) {
        return Nothing();
    }

    try {
        // XXX add checking?!
        return NSc::TValue::FromJsonThrow(response->Data);
    }
    catch (const NSc::TSchemeParseException& e) {
        LOG(ERR) << "NerApi: json parsing error: offset: " << e.Offset
                 << ", resason: " << e.Reason
                 << ", body: " << response->Data
                 << Endl;
    }
    catch (...) {
        LOG(ERR) << "NerApi: exception during parsing json: " << CurrentExceptionMessage() << Endl;
    }

    return Nothing();
}

} // namespace NBASS::NExternalSkill
} // namespace NBASS
