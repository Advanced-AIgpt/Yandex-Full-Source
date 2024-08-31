#include <alice/cachalot/library/modules/activation/common.h>

namespace NCachalot {

size_t TActivationOperationOptions::CombineFlags() const {
    size_t flags = 0;
    flags = (flags << 1) | static_cast<size_t>(IgnoreRms);
    return flags;
}

}  // namespace NCachalot
