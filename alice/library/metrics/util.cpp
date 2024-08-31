#include "util.h"

#include <util/string/builder.h>

namespace NAlice::NMetrics {

TString NormalizeSensorNameForGolovan(TStringBuf name) {
    TStringBuilder b;
    b.reserve(name.size());

    size_t i = 0;
    while (i < name.size()) {
        while (i < name.size() && name[i] == '_') {
            ++i;
        }
        size_t j = i;
        while (j < name.size() && name[j] != '_') {
            ++j;
        }
        if (b && i != j) {
            b << '_';
        }
        b << name.SubStr(i, j - i);
        i = j;
    }
    return b;
}

} // namespace NAlice::NMetrics
