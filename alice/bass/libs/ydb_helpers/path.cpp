#include "path.h"

#include <util/stream/output.h>

template <>
void Out<NYdbHelpers::TTablePath>(IOutputStream& os, const NYdbHelpers::TTablePath& table) {
    os << "NYdbHelpers::TTablePath [" << table.Database << ", " << table.Name << "]";
}
