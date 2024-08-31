#include "utils.h"

#include <util/string/subst.h>

namespace NPersonalCards {

TString NormalizeUUID(TString str) {
    SubstGlobal(str, "-", "", 0);
    str.to_lower();
    return str;
}

} // namespace NPersonalCards
