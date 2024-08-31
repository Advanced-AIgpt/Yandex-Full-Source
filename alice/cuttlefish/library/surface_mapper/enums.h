#pragma once

#include <util/generic/flags.h>
#include <util/generic/vector.h>

#include <array>

namespace NVoice::NSurfaceMapper {
    namespace NPrivate {
        template <typename T, size_t N>
        constexpr bool IsDescOrder(const std::array<T, N>& array) {
            for (size_t i = 1; i < N; ++i) {
                if (array[i - 1] <= array[i]) {
                    return false;
                }
            }
            return true;
        }
    }

    template <typename TEnum>
    struct TEnumIterator {
    };

#define Y_REGISTER_ENUM(type, ...)                                                                 \
    template <>                                                                                    \
    struct TEnumIterator<type> {                                                                   \
        inline static constexpr std::array Values{__VA_ARGS__};                                    \
        static_assert(NPrivate::IsDescOrder(Values), "Values must be sorted in descending order"); \
    };

    enum EVendor {
        V_UNKNOWN = 0 /* "unknown" */,
        V_YANDEX = 1 /* "yandex" */,
        V_OTHER = 2 /* "other" */,
    };

    enum EPlatform {
        P_UNKNOWN = 0 /* "unknown" */,
        P_ANDROID = 1 /* "android" */,
        P_IOS = 2 /* "ios" */,
        P_MOBILE = 3 /* "mobile" */,
        P_DESKTOP = 4 /* "desktop" */,
        P_LINUX = 8 /* "linux" */,
        P_BACKEND = 16 /* "backend" */,
    };

    Y_DECLARE_FLAGS(TPlatforms, EPlatform);
    Y_DECLARE_OPERATORS_FOR_FLAGS(TPlatforms);
    Y_REGISTER_ENUM(EPlatform, P_LINUX, P_DESKTOP, P_MOBILE, P_IOS, P_ANDROID);

    enum EType {
        T_UNKNOWN = 0 /* "unknown" */,
        T_DEV = 1 /* "dev" */,
        T_BETA = 2 /* "beta" */,
        T_NONPROD = 3 /* "nonprod" */,
        T_PROD = 4 /* "prod" */,
        T_PUBLIC = 6 /* "public" */,
        T_ALL = 7 /* "all" */,
    };

    Y_DECLARE_FLAGS(TTypes, EType);
    Y_DECLARE_OPERATORS_FOR_FLAGS(TTypes);
    Y_REGISTER_ENUM(EType, T_ALL, T_PUBLIC, T_PROD, T_NONPROD, T_BETA, T_DEV);

#undef Y_REGISTER_ENUM

    ////////////////////////////////////////////////////////////////////////////////

    namespace NPrivate {
        template <bool disjoint, typename TEnum>
        TVector<TEnum> ListFlags(TFlags<TEnum> flags) {
            TVector<TEnum> result;
            for (const TEnum value : TEnumIterator<TEnum>::Values) {
                if (flags.HasFlags(value)) {
                    result.push_back(value);
                    if constexpr (disjoint) {
                        flags.RemoveFlags(value);
                    }
                }
            }
            if (disjoint && result.empty()) {
                // Returning 'unknown' for disjoint mode is usable for monitorings.
                result.push_back(TEnum(0));
            }
            return result;
        }
    }

    template <typename TEnum>
    TEnum GetGreatestNamedFlag(const TFlags<TEnum>& flags) {
        for (const TEnum value : TEnumIterator<TEnum>::Values) {
            if (flags.HasFlags(value)) {
                return value;
            }
        }
        // 'unknown' is usable for monitorings.
        return TEnum(0);
    }

    template <typename TEnum>
    TVector<TEnum> ListAllFlags(TFlags<TEnum> flags) {
        return NPrivate::ListFlags<false>(std::move(flags));
    }

    template <typename TEnum>
    TVector<TEnum> ListDisjointFlags(TFlags<TEnum> flags) {
        return NPrivate::ListFlags<true>(std::move(flags));
    }
}
