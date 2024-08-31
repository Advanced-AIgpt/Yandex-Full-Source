#include "ner.h"

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

TNerRequester::TNerRequester(NHttpFetcher::TRequestPtr request, TStringBuf utterance, TStringBuf skillId) {
    NSc::TValue content;
    if (!utterance.Empty()) {
        content["utterance"].SetString(utterance);
    } else {
        return;
    }

    if (!skillId.Empty()) {
        content["skill_id"].SetString(skillId);
    }

    request->SetBody(content.ToJson(), TStringBuf("POST"));
    request->SetContentType(TStringBuf("application/json"));
    Handle_ = request->Fetch();
}

const NSc::TValue* TNerRequester::Response() {
    if (!Handle_) {
        return Response_.Get();
    }

    const NHttpFetcher::TResponse::TRef response = Handle_->Wait();
    Handle_.Reset();
    if (response->IsError()) {
        return Response_.Get();
    }

    try {
        // XXX add checking?!
        Response_.ConstructInPlace(NSc::TValue::FromJsonThrow(response->Data));
    }
    catch (const NSc::TSchemeParseException& e) {
        LOG(ERR) << "NerApi: json parsing error: offset: " << e.Offset
                 << ", reason: " << e.Reason
                 << ", body: " << response->Data
                 << Endl;
    }
    catch (...) {
        LOG(ERR) << "NerApi: exception during parsing json: " << CurrentExceptionMessage() << Endl;
    }

    return Response_.Get();
}

} // namespace NBASS
