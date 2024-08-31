#include "str.h"

#include <util/string/strip.h>

TStringBuf NAlice::RemoveTrailing(TStringBuf value, char c) {
    return StripStringRight(value, EqualsStripAdapter(c));
 }
