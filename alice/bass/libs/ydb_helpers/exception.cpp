#include "exception.h"

#include <util/stream/str.h>
#include <util/string/builder.h>

using namespace NYdb;

namespace NYdbHelpers {
// TYdbErrorException ----------------------------------------------------------
TYdbErrorException::TYdbErrorException(const TStatus& status)
    : Status(status)
    , Message(StatusToString(status)) {
}

const char* TYdbErrorException::what() const noexcept {
    return Message.c_str();
}

// -----------------------------------------------------------------------------
TStatus ThrowOnError(const TStatus& status) {
    if (!status.IsSuccess())
        ythrow TYdbErrorException(status);
    return status;
}

TString StatusToString(const TStatus& status) {
    TString msg = TStringBuilder() << status.GetStatus() << Endl;

    {
        TStringOutput output(msg);
        status.GetIssues().PrintTo(output);
    }

    return msg;
}
} // namespace NYdbHelpers
