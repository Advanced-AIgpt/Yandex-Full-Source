#include "http_response.h"

namespace NAlice {

void IHttpResponse::Flush() {
    if (IsFlushed_) {
        return;
    }

    DoOut();
    IsFlushed_ = true;
}

} // namespace NAlice
