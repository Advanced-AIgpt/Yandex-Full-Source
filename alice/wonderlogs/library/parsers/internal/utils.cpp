#include "utils.h"

namespace NAlice::NWonderlogs {

bool NotEmpty(const TMaybe<TString>& id) {
    return id && !id->empty();
}

} // namespace NAlice::NWonderlogs
