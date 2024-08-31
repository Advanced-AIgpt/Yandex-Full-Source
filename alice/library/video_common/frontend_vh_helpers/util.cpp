#include "util.h"

#include <util/string/builder.h>

namespace NAlice::NVideoCommon {

TString MakeEntref(const TString& intoId) {
    return TStringBuilder() << "entnext=" << intoId;
}

} // namespace NAlice::NVideoCommon
