#pragma once

#include <util/system/compiler.h>

namespace NMatrix {

template <typename TEvent>
using TRequestEventPatcher = void (*)(TEvent& event);

template <typename TEvent>
void EmptyRequestEventPatcher(TEvent& event) {
    Y_UNUSED(event);
}

} // namespace NMatrix
