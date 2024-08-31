#pragma once
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>


namespace NAlice::NCuttlefish::NConvert {


template <NJson::EJsonValueType Type>
class TJsonValueWriter {
public:
    static_assert(Type == NJson::JSON_UNDEFINED || Type == NJson::JSON_MAP || Type == NJson::JSON_ARRAY, "invalid type");

    inline TJsonValueWriter(NJson::TJsonValue& node)
        : Node(node)
    { }

    inline TJsonValueWriter<NJson::JSON_MAP> Map() {
        return SubNode(NJson::JSON_MAP);
    }

    inline TJsonValueWriter<NJson::JSON_ARRAY> Array() {
        return SubNode(NJson::JSON_ARRAY);
    }

    inline void Key(TStringBuf key) {
        static_assert(Type == NJson::JSON_MAP, "node is not a Map");

        Y_ASSERT(!CurrentNode);
        CurrentNode = &Node[key];
    }

    inline void Null() {
        Value(NJson::JSON_NULL);
    }

    template <typename T>
    inline void Value(T&& val) {
        static_assert(Type != NJson::JSON_UNDEFINED, "undefined node type");

        if constexpr (Type == NJson::JSON_MAP) {
            Y_ASSERT(CurrentNode);
            CurrentNode->SetValue(std::forward<T>(val));
            CurrentNode = nullptr;
            return;
        }
        Node.AppendValue(std::forward<T>(val));
    }

    template <typename KeyT, typename ValueT>
    inline void Insert(KeyT&& key, ValueT&& value) {
        Key(std::forward<KeyT>(key));
        Value(std::forward<ValueT>(value));
    }

private:
    NJson::TJsonValue& SubNode(NJson::EJsonValueType subNodeType) {
        if constexpr (Type == NJson::JSON_UNDEFINED) {
            return Node.SetType(subNodeType);
        }
        if constexpr (Type == NJson::JSON_MAP) {
            Y_ASSERT(CurrentNode);
            NJson::TJsonValue* node = CurrentNode;
            CurrentNode = nullptr;
            return node->SetType(subNodeType);
        }
        return Node.AppendValue(subNodeType);
    }

    NJson::TJsonValue& Node;
    NJson::TJsonValue* CurrentNode = nullptr;
};


class TJsonValueRootWriter
    : public NJson::TJsonValue
    , public TJsonValueWriter<NJson::JSON_UNDEFINED>
{
public:
    TJsonValueRootWriter()
        : TJsonValueWriter<NJson::JSON_UNDEFINED>(static_cast<NJson::TJsonValue&>(*this))
    { }
};


}  // namespace NAlice::NCuttlefish::NConvert

