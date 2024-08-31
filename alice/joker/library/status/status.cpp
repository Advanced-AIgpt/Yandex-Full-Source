#include "status.h"

#include <alice/library/network/common.h>

#include <library/cpp/http/server/response.h>

namespace NAlice::NJoker {

const TError& TError::HttpResponse(THttpOutput& out) const {
    THttpResponse{HttpCode}
        .SetContentType(NContentTypes::TEXT_PLAIN)
        .SetContent(TStringBuilder{} << Type << CRLF << ErrorMsg_)
        .OutTo(out);
    return *this;
}

TString TError::AsString() const {
    return TStringBuilder{} << ErrorMsg_ << " (" << Type << ", " << HttpCode << ')';
}

} // namespace NAlice::NJoker

template <>
void Out<NAlice::NJoker::TError>(IOutputStream& out, const NAlice::NJoker::TError& error) {
    out << error.AsString();
}
