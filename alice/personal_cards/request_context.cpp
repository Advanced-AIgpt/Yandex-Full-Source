#include "request_context.h"

#include <library/cpp/json/json_writer.h>

namespace NPersonalCards {

TRequestContext::TRequestContext(
    NJson::TJsonMap&& request,
    const TString& forwardedFor,
    const TString& startTime,
    const TMaybe<ui64> uidFromTvm
)
    : Request_(std::move(request))
    , ForwardedFor_(forwardedFor)
    , StartTime_(startTime)
    , UidFromTvm_(uidFromTvm)
{}

IOutputStream& operator << (IOutputStream& out, const TRequestContext& requestContext) {
    if (!requestContext.ForwardedFor().empty()) {
        out << "ForwardedFor: " << requestContext.ForwardedFor() << " ";
    }
    if (!requestContext.StartTime().empty()) {
        out << "StartTime: " << requestContext.StartTime() << " ";
    }
    out << NJson::WriteJson(
        requestContext.Request(),
        false, // formatOutput
        false, // sortkeys
        false  // validateUtf8
    );
    return out;
}

} // namespace NPersonalCards
