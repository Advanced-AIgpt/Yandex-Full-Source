#include "time_util.h"

#include "util/string/builder.h"
#include "util/string/printf.h"

namespace NAlice::NHollywood::NMusic {

TString FormatTInstant(const TInstant& timestamp) {
    return TStringBuilder() << timestamp.FormatGmTime("%Y-%m-%dT%H:%M:%S.")
                            << Sprintf("%03dZ", timestamp.MilliSecondsOfSecond());
}

} // NAlice::NHollywood::NMusic