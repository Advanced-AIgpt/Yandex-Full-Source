#pragma once

#include <alice/cuttlefish/library/utils/string_utils.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/array_ref.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/src_location.h>

#include <algorithm>
#include <type_traits>

namespace NPrivate {

// Helper type that can be implicitly converted into anything except `TExclude`
template <class TExclude>
struct XType {
    XType() = delete;

    template <class T, class = std::enable_if_t<!std::is_same_v<T, TExclude>>>
    operator T() const; // no need to define since it's never actually called
};


template <class Type, class ...TypeArgs>
constexpr std::size_t ConstructorArgumentsMinCount() {
    if constexpr (std::is_constructible_v<Type, TypeArgs...>) {
        return sizeof...(TypeArgs);
    } else {
        return NPrivate::ConstructorArgumentsMinCount<Type, TypeArgs..., XType<Type>>();
    }
}

} // namespace NPrivate


// Minimal count of arguments for `T` constructor
// NOTE: 0 if `T` has default constructor
template <class T>
inline constexpr std::size_t ConstructorArgumentsMinCount = NPrivate::ConstructorArgumentsMinCount<T>();

// ------------------------------------------------------------------------------------------------

template <typename T>
const T& FromJson(const NJson::TJsonValue& x);

template <>
inline const TString& FromJson<TString>(const NJson::TJsonValue& x) {
    return x.GetString();
}

template <typename TPathContainer = TArrayRef<const TStringBuf>>
const NJson::TJsonValue* GetJsonValueByPath(const NJson::TJsonValue& root, const TPathContainer& path) {
    const NJson::TJsonValue* it = &root;
    for (const auto& p : path) {
        it = it->GetMap().FindPtr(p);
        if (it == nullptr)
            break;
    }
    return it;
}

template <typename T>
const T* GetValueByPath(const NJson::TJsonValue& root, const TArrayRef<const TStringBuf> path) {
    if (const NJson::TJsonValue* json = GetJsonValueByPath(root, path))
        return &FromJson<T>(*json);
    return nullptr;
}

bool EnsureVinsExperimentsFormat(NJson::TJsonValue* flags);
