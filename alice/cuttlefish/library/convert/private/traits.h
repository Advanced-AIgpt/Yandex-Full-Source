#pragma once
#include <type_traits>

namespace NAlice::NCuttlefish::NConvert {

namespace NPrivate {

    template <typename>
    constexpr void Helper();

    template <typename T, typename DefaultT, typename = void>
    struct TNextMessageType {
        using Type = DefaultT;
    };

    template <typename T, typename DefaultT>
    struct TNextMessageType<T, DefaultT, decltype(Helper<typename T::TNextMessage>())> {
        using Type = typename T::TNextMessage;
    };

}  // namespace NPrivate
// ------------------------------------------------------------------------------------------------

template <typename T>
using IfSignedInt = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>;

template <typename T>
using IfUnsignedInt = std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T> && !std::is_same_v<T, bool>>;

template <typename T>
using IfBool = std::enable_if_t<std::is_same_v<T, bool>>;

template <typename T>
using IfFloat = std::enable_if_t<std::is_floating_point_v<T>>;

// ------------------------------------------------------------------------------------------------

template <typename T, typename DefaultT>
using TNextMessageType = typename NPrivate::TNextMessageType<T, DefaultT>::Type;

}  // namespace NAlice::NCuttlefish::NConvert
