#pragma once

#include <util/generic/string.h>

#include <functional>


namespace NAlice {
namespace NFrameFiller {

struct TSearchDocMeta {
    TString Type;
    TString Subtype;
};

using TAcceptDocPredicate = std::function<bool(const TSearchDocMeta&)>;

} // namespace NFrameFiller
} // namespace NAlice
