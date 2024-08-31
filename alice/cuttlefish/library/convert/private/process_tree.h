#pragma once
#include <library/cpp/json/json_value.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <type_traits>
#include <alice/cuttlefish/library/convert/rapid_node.h>


namespace NAlice::NCuttlefish::NConvert::NPrivate {



template <typename ValueType>
struct TProcessTree {
    using TValue = ValueType;
    using TThis = TProcessTree<ValueType>;

    static_assert(std::is_nothrow_default_constructible_v<TValue>);

    const TString Name;
    TValue Value;
    TVector<TThis> Children;

    TProcessTree(const TStringBuf name = "")
        : Name(name)
        , Value()
    { }

    template <typename T>
    void Add(TStringBuf path, T&& val, char delim = '/') {
        TStringBuf rootName, restPath;
        if (!path.TrySplit(delim, rootName, restPath)) {
            FindOrCreate(path).Value = std::forward<T>(val);
        } else {
            FindOrCreate(rootName).Add(restPath, std::forward<T>(val), delim);
        }
    }

    TThis& FindOrCreate(TStringBuf name) {
        for (TThis& child : Children) {
            if (child.Name == name)
                return child;
        }
        return Children.emplace_back(name);
    }

    const TThis* FindPtr(TStringBuf name) const {
        for (const TThis& child : Children) {
            if (child.Name == name)
                return &child;
        }
        return nullptr;
    }

    template <typename ...TArgs>
    void Parse(const NJson::TJsonValue::TMapType& jsonRoot, TArgs&& ...args) const
    {
        static_assert(std::is_convertible_v<TValue, bool>);
        static_assert(std::is_invocable_v<TValue, const NJson::TJsonValue&, TArgs&&...>);

        for (const auto& child : Children) {
            if (const NJson::TJsonValue* it = jsonRoot.FindPtr(child.Name)) {
                if (child.Value)
                    child.Value(*it, std::forward<TArgs>(args)...);
                if (it->IsMap())
                    child.Parse(it->GetMap(), std::forward<TArgs>(args)...);
            }
        }
    }
    template <typename ...TArgs>
    void Parse(TRapidJsonNode& node, TArgs&& ...args) const
    {
        static_assert(std::is_convertible_v<TValue, bool>);
        static_assert(std::is_invocable_v<TValue, TRapidJsonNode&, TArgs&&...>);

        TString key;
        TRapidJsonNode val;
        while (node.NextMapNode(key, val)) {
            if (const TThis* child = FindPtr(key)) {
                if (child->Value)
                    child->Value(val, std::forward<TArgs>(args)...);
                if (val.IsMap())
                    child->Parse(val, std::forward<TArgs>(args)...);
            }
        }
    }

    template <typename WriterT, typename ...TArgs>
    void Serialize(WriterT&& writer, TArgs&& ...args) const
    {
        static_assert(std::is_convertible_v<TValue, bool>);

        for (const auto& child : Children) {
            if (child.Value) {
                if (child.Value.Needed(std::forward<TArgs>(args)...)) {
                    writer.Key(child.Name);
                    child.Value.Serialize(writer, std::forward<TArgs>(args)...);
                }
            } else {
                writer.Key(child.Name);
                child.Serialize(writer.Map(), std::forward<TArgs>(args)...);
            }
        }
    }
};

}  // namespace NAlice::NCuttlefish::NConvert::NPrivate
