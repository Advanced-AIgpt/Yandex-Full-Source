#pragma once
#include <alice/cuttlefish/library/convert/private/json_converts.h>
#include <type_traits>


namespace NAlice::NCuttlefish::NConvert{


struct TSoftConvert {
    template <typename FieldType, typename JsonNodeType>
    static inline decltype(auto) Convert(JsonNodeType& val) {
        return NPrivate::TJsonConverts<FieldType>::GetSoft(val);
    }
};


struct TStrictConvert {
    template <typename FieldType, typename JsonNodeType>
    static inline decltype(auto) Convert(JsonNodeType& val) {
        return NPrivate::TJsonConverts<FieldType>::GetSafe(val);
    }
};


struct TDefault { };

struct TSerializeAlways { };


namespace NMethods {
/**
 * Each struct here contains template static method `Exec` that perform a part of handling JSON
 * node. Another compatible struct is used as a first template argument of the method to perform
 * the rest of handling. Hence chain of such structs may completely handle a node.
*/

namespace NPrivate {

struct THelper {
    template <typename X, auto = &X::IsSerializationNeeded>
    constexpr static bool HasIsSerializationNeeded(const X*) {
        return true;
    };
    constexpr static bool HasIsSerializationNeeded(const void*) {
        return false;
    };

    template <typename NodeT, typename MessageT, typename HandlerT, void(*)(NodeT&, MessageT&) = &HandlerT::Parse>
    constexpr static bool CanParse(const HandlerT*) {
        return true;
    };
    template <typename NodeT, typename MessageT, typename HandlerT, void(HandlerT::*)(NodeT&, MessageT&) const = &HandlerT::Parse>
    constexpr static bool CanParse(const HandlerT*) {
        return true;
    };
    template <typename NodeT, typename MessageT>
    constexpr static bool CanParse(const void*) {
        return false;
    };

    template <typename NodeT, typename MessageT, typename HandlerT, typename = decltype(
        HandlerT::Parse(*(TString*)nullptr, *(NodeT*)nullptr, *(MessageT*)nullptr)
    ), typename = char> constexpr static bool CanParseWithKey(const HandlerT*) {
        return true;
    };
    template <typename NodeT, typename MessageT, typename HandlerT, typename = decltype(
        ((HandlerT*)nullptr)->Parse(*(TString*)nullptr, *(NodeT*)nullptr, *(MessageT*)nullptr)
    )> constexpr static bool CanParseWithKey(const HandlerT*, int) {
        return true;
    };
    template <typename NodeT, typename MessageT>
    constexpr static bool CanParseWithKey(const void*, int = {}) {
        return false;
    };

};

}  // namespace NPrivate

// ------------------------------------------------------------------------------------------------
struct TDoNothing {
    template <typename ...TArgs>
    static inline void Exec(TArgs&& ...) { }

    template <typename ...TArgs>
    static inline void Serialize(TArgs&& ...) { }


    template <typename ...TArgs>
    static constexpr bool IsSerializationNeeded(TArgs&& ...) {
        return true;
    }

    static constexpr inline bool SerializeAlways() {
        return false;
    }

    template <typename...>
    static constexpr inline bool CanParse() {
        return true;
    }

    template <typename...>
    static constexpr inline bool CanParseWithKey() {
        return true;
    }
};

// ------------------------------------------------------------------------------------------------
struct TBaseHandler {
    // here are methods that can be "overridden" in subclasses if needed

    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParse() {
        return NextFunc::template CanParse<NodeT, MessageT>();
    }

    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParseWithKey() {
        return NextFunc::template CanParseWithKey<NodeT, MessageT>();
    }

    template <typename NextFunc = TDoNothing>
    static constexpr inline bool SerializeAlways() {
        return NextFunc::SerializeAlways();
    }

    template <typename NextFunc = TDoNothing, typename ...TArgs>
    static constexpr inline bool IsSerializationNeeded(TArgs&& ...args) {
        return NextFunc::IsSerializationNeeded(std::forward<TArgs>(args)...);
    }
};

// ------------------------------------------------------------------------------------------------
struct TForEachInArray : TBaseHandler {
    template <typename NextFunc, typename MessageT>
    static inline void Exec(const NJson::TJsonValue& node, MessageT& msg) {
        for (const auto& item : node.GetArray()) {
            NextFunc::Exec(item, msg);
        }
    }

    template <typename NextFunc, typename MessageT>
    static inline void Exec(TRapidJsonNode& node, MessageT& msg) {
        TRapidJsonNode item;
        while (node.NextArrayNode(item)) {
            NextFunc::Exec(item, msg);
        }
    }

    template <typename NextFunc = TDoNothing, typename WriterT, typename MessageT>
    static inline void Serialize(WriterT&& writer, const MessageT& msg) {
        NextFunc::Serialize(writer.Array(), msg);
    }
};

// ------------------------------------------------------------------------------------------------
struct TForEachInMap : TBaseHandler {
    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParse() {
        return NextFunc::template CanParseWithKey<NodeT, MessageT>();
    }

    template <typename NextFunc, typename MessageT>
    static inline void Exec(const NJson::TJsonValue& node, MessageT& msg)
    {
        for (const auto& kv : node.GetMap())
            NextFunc::Exec(kv.first, kv.second, msg);
    }

    template <typename NextFunc, typename MessageT>
    static inline void Exec(TRapidJsonNode& node, MessageT& msg)
    {
        TString key;
        TRapidJsonNode value;
        while (node.NextMapNode(key, value))
            NextFunc::Exec(key, value, msg);
    }

    template <typename NextFunc = TDoNothing, typename WriterT, typename MessageT>
    static inline void Serialize(WriterT&& writer, const MessageT& msg) {
        NextFunc::Serialize(writer.Map(), msg);
    }
};

// ------------------------------------------------------------------------------------------------
template <typename FieldTraits>
struct TSetKey : TBaseHandler {
    using FieldType = typename FieldTraits::FieldType;

    template <typename NextFunc = TDoNothing, typename JsonNodeType>
    static inline void Exec(TStringBuf key, JsonNodeType& node, typename FieldTraits::MessageType& msg)
    {
        FieldTraits::Set(msg, FromString<FieldType>(key));
        NextFunc::Exec(node, msg);
    }

    template <typename NextFunc = TDoNothing, typename WriterT>
    static inline void Serialize(WriterT&& writer, const typename FieldTraits::MessageType& msg)
    {
        writer.Key(ToString(FieldTraits::Get(msg)));
        NextFunc::Serialize(writer, msg);
    }
};

// ------------------------------------------------------------------------------------------------
template <typename FieldTraits>
struct TAddNew : TBaseHandler {
    using TMessage = typename FieldTraits::MessageType;
    using TNextMessage = typename FieldTraits::FieldType;

    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParse() {
        return NextFunc::template CanParse<NodeT, TNextMessage>();
    }
    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParseWithKey() {
        return NextFunc::template CanParseWithKey<NodeT, TNextMessage>();
    }

    template <typename NextFunc = TDoNothing, typename NodeT>
    static inline void Exec(NodeT& node, TMessage& msg) {
        NextFunc::Exec(node, *FieldTraits::Add(msg));
    }

    template <typename NextFunc = TDoNothing, typename NodeT>
    static inline void Exec(TStringBuf key, NodeT& node, TMessage& msg) {
        NextFunc::Exec(key, node, *FieldTraits::Add(msg));
    }

    template <typename NextFunc = TDoNothing, typename WriterT>
    static inline void Serialize(WriterT&& writer, const TMessage& msg) {
        for (size_t i = 0; i < FieldTraits::Size(msg); ++i) {
            NextFunc::Serialize(writer, FieldTraits::Get(msg, i));
        }
    }

    template <typename NextFunc = TDoNothing>
    static inline bool IsSerializationNeeded(const TMessage& msg) {
        return FieldTraits::Size(msg) > 0;
    }
};

// ------------------------------------------------------------------------------------------------
template <typename FieldTraits, typename ConvertType = TSoftConvert>
struct TAppend : TBaseHandler {
    using TField = typename FieldTraits::FieldType;
    using TMessage = typename FieldTraits::MessageType;

    template <typename NodeT>
    static inline decltype(auto) Convert(NodeT& val) {
        return ConvertType::template Convert<TField>(val);
    }

    template <typename NodeT>
    static inline void Exec(NodeT& node, TMessage& msg) {
        FieldTraits::Add(msg, Convert(node));
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& writer, const TMessage& msg) {
        for (size_t i = 0; i < FieldTraits::Size(msg); ++i)
            writer.Value(FieldTraits::Get(msg, i));
    }

    static inline bool IsSerializationNeeded(const TMessage& msg) {
        return FieldTraits::Size(msg) > 0;
    }
};

// ------------------------------------------------------------------------------------------------
template <typename FieldTraits>
struct TSub : TBaseHandler {
    using TMessage = typename FieldTraits::MessageType;
    using TNextMessage = typename FieldTraits::FieldType;

    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParse() {
        return NextFunc::template CanParse<NodeT, TNextMessage>();
    }
    template <typename NodeT, typename MessageT, typename NextFunc = TDoNothing>
    static constexpr inline bool CanParseWithKey() {
        return NextFunc::template CanParseWithKey<NodeT, TNextMessage>();
    }

    template <typename NextFunc, typename NodeT>
    static inline void Exec(NodeT& node, TMessage& msg) {
        NextFunc::Exec(node, *FieldTraits::Mutable(msg));
    }

    template <typename NextFunc = TDoNothing, typename WriterT>
    static inline void Serialize(WriterT&& writer, const TMessage& msg) {
        NextFunc::Serialize(writer, FieldTraits::Get(msg));
    }

    template <typename NextFunc = TDoNothing>
    static inline bool IsSerializationNeeded(const TMessage& msg) {
        return NextFunc::SerializeAlways() || (FieldTraits::Has(msg) && NextFunc::IsSerializationNeeded(FieldTraits::Get(msg)));
    }
};

// ------------------------------------------------------------------------------------------------
template <typename FieldTraits, typename ConvertType = TSoftConvert, typename SerializationTrats = TDefault>
struct TSetValue : TBaseHandler {
    using TMessage = typename FieldTraits::MessageType;
    using TField = typename FieldTraits::FieldType;

    template <typename NodeT>
    static inline decltype(auto) Convert(NodeT& val) {
        return ConvertType::template Convert<TField>(val);
    }

    template <typename NodeT>
    static inline void Exec(NodeT& node, TMessage& msg) {
        FieldTraits::Set(msg, Convert(node));
    }

    template <typename WriterT>
    static inline void Serialize(WriterT&& writer, const TMessage& msg) {
        writer.Value(FieldTraits::Get(msg));
    }

    template <typename NextFunc = TDoNothing>
    static constexpr inline bool SerializeAlways() {
        return std::is_same_v<SerializationTrats, TSerializeAlways>;
    }

    static inline bool IsSerializationNeeded(const TMessage& msg) {
        return SerializeAlways() || FieldTraits::Has(msg);
    }
};

// ------------------------------------------------------------------------------------------------
template <typename FieldTraits, typename ConvertType = TSoftConvert>
struct TSetSpareValue : TSetValue<FieldTraits, ConvertType>  {
    using TMessage = typename FieldTraits::MessageType;

    template <typename NodeT>
    static inline void Exec(NodeT& node, TMessage& msg) {
        if (!FieldTraits::IsSet(msg)) {
            FieldTraits::Set(msg, TSetValue<FieldTraits, ConvertType>::Convert(node));
        }
    }

    static constexpr bool IsSerializationNeeded(const TMessage&) {
        return false;
    }
};


// ------------------------------------------------------------------------------------------------
// TODO: use TCustomAllocated instead
template <const auto& Parser>
struct TParse : TBaseHandler {
    template <typename NodeT, typename MessageT>
    static inline void Exec(NodeT& node, MessageT& msg) {
        Parser.Parse(msg, node);
    }

    template <typename WriterT, typename MessageT>
    static inline void Serialize(WriterT&& writer, const MessageT& msg) {
        Parser.Serialize(msg, writer);
    }
};

// ------------------------------------------------------------------------------------------------
template <typename HandlerT>
struct TCustom : TBaseHandler {
    template <typename NodeT, typename MessageT>
    static constexpr inline bool CanParse() {
        return NPrivate::THelper::CanParse<NodeT, MessageT>((HandlerT*)nullptr);
    }
    template <typename NodeT, typename MessageT>
    static constexpr inline bool CanParseWithKey() {
        return NPrivate::THelper::CanParseWithKey<NodeT, MessageT>((HandlerT*)nullptr);
    }

    template <typename ...TArgs>
    static inline void Exec(TArgs&& ...args) {
        HandlerT::Parse(std::forward<TArgs>(args)...);
    }

    template <typename ...TArgs>
    static inline void Serialize(TArgs&& ...args) {
        HandlerT::Serialize(std::forward<TArgs>(args)...);
    }

    template <typename MessageT>
    static constexpr inline bool IsSerializationNeeded(const MessageT& msg)
    {
        if constexpr (NPrivate::THelper::HasIsSerializationNeeded((HandlerT*)nullptr)) {
            return HandlerT::IsSerializationNeeded(msg);
        }
        return true;
    }
};


// ------------------------------------------------------------------------------------------------
template <const auto& Handler>
struct TCustomAllocated : TBaseHandler {
    template <typename NodeT, typename MessageT>
    static constexpr inline bool CanParse() {
        return NPrivate::THelper::CanParse<NodeT, MessageT>(&Handler);
    }
    template <typename NodeT, typename MessageT>
    static constexpr inline bool CanParseWithKey() {
        return NPrivate::THelper::CanParseWithKey<NodeT, MessageT>(&Handler);
    }

    template <typename ...TArgs>
    static inline void Exec(TArgs&& ...args) {
        Handler.Parse(std::forward<TArgs>(args)...);
    }

    template <typename ...TArgs>
    static inline void Serialize(TArgs&& ...args) {
        Handler.Serialize(std::forward<TArgs>(args)...);
    }

    template <typename MessageT>
    static constexpr inline bool IsSerializationNeeded(const MessageT& msg)
    {
        if constexpr (NPrivate::THelper::HasIsSerializationNeeded(&Handler)) {
            return Handler.IsSerializationNeeded(msg);
        }
        return true;
    }
};



}  // namespace NMethods
}  // namespace NAlice::NCuttlefish::NConvert
