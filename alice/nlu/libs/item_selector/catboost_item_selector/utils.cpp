#include "utils.h"

#include <util/string/builder.h>

namespace NAlice {
namespace NItemSelector{

TString GetPositionalText(size_t position) {
    return TStringBuilder() << "Включи номер " << position;
}

} // NItemSelector
} // NAlice
