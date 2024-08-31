#pragma once
#include <utility>
#include <type_traits>
#include <alice/cuttlefish/library/convert/private/json_converts.h>
#include <alice/cuttlefish/library/convert/rapid_node.h>
#include <alice/cuttlefish/library/convert/json_value_writer.h>
#include <alice/cuttlefish/library/convert/methods.h>

namespace NAlice::NCuttlefish::NConvert {

namespace NPrivate {

template <typename MessageType, typename ...HandlerTypes>
struct THandlerList {
    using TNextMessage = MessageType;
};

template <typename MessageType, typename FirstHandlerType, typename ...HandlerTypes>
struct THandlerList<MessageType, FirstHandlerType, HandlerTypes...> {
    using TTail = THandlerList<TNextMessageType<FirstHandlerType, MessageType>, HandlerTypes...>;
    using TNextMessage = typename TTail::TNextMessage;

    template <typename ...TArgs>
    static inline auto Exec(TArgs&& ...args) {
        return FirstHandlerType::template Exec<TTail>(std::forward<TArgs>(args)...);
    }

    template <typename ...TArgs>
    static inline auto Serialize(TArgs&& ...args) {
        return FirstHandlerType::template Serialize<TTail>(std::forward<TArgs>(args)...);
    }

    template <typename ...TArgs>
    static inline auto IsSerializationNeeded(TArgs&& ...args) {
        return FirstHandlerType::template IsSerializationNeeded<TTail>(std::forward<TArgs>(args)...);
    }

    static constexpr bool SerializeAlways() {
        return FirstHandlerType::template SerializeAlways<TTail>();
    }

    template <typename NodeT, typename MessageT = MessageType>
    static constexpr bool CanParse() {
        return FirstHandlerType::template CanParse<NodeT, MessageT, TTail>();
    }
    template <typename NodeT, typename MessageT>
    static constexpr inline bool CanParseWithKey() {
        return FirstHandlerType::template CanParseWithKey<NodeT, MessageT, TTail>();
    }
};

template <typename MessageType, typename HandlerType>
struct THandlerList<MessageType, HandlerType> {
    using TNextMessage = TNextMessageType<HandlerType, MessageType>;

    template <typename ...TArgs>
    static inline auto Exec(TArgs&& ...args) {
        return HandlerType::Exec(std::forward<TArgs>(args)...);
    }

    template <typename ...TArgs>
    static inline auto Serialize(TArgs&& ...args) {
        return HandlerType::Serialize(std::forward<TArgs>(args)...);
    }

    template <typename ...TArgs>
    static inline auto IsSerializationNeeded(TArgs&& ...args) {
        return HandlerType::IsSerializationNeeded(std::forward<TArgs>(args)...);
    }

    static constexpr bool SerializeAlways() {
        return HandlerType::SerializeAlways();
    }

    template <typename NodeT, typename MessageT = MessageType>
    static constexpr bool CanParse() {
        return HandlerType::template CanParse<NodeT, MessageT>();
    }
    template <typename NodeT, typename MessageT>
    static constexpr inline bool CanParseWithKey() {
        return HandlerType::template CanParseWithKey<NodeT, MessageT>();
    }
};

}  // namespace NPrivate


// ------------------------------------------------------------------------------------------------
template <typename THandlerBuilder, typename FieldTraits>
class TFieldSetter {
// sugar class to simplify adding multiple handlers for a single field
public:
    TFieldSetter(THandlerBuilder builder)
        : Builder(builder)
    { }

    template <typename ...Ts>
    auto From(TStringBuf path) {
        Builder.template SetValue<FieldTraits, Ts...>(path);
        return *this;
    }

    template <typename ...Ts>
    auto SpareFrom(TStringBuf path) {
        Builder.template SetSpareValue<FieldTraits, Ts...>(path);
        return *this;
    }

private:
    THandlerBuilder Builder;
};


// ------------------------------------------------------------------------------------------------
template <typename ConverterType, typename ...Funcs>
class THandlerBuilder {
public:
    using TThis     = THandlerBuilder<ConverterType, Funcs...>;
    using TMessage  = typename ConverterType::TMessage;
    using THandlers = NPrivate::THandlerList<TMessage, Funcs...>;

    template <typename NextFunc>
    using TNext = THandlerBuilder<ConverterType, Funcs..., NextFunc>;

    template <typename NodeT>
    using TParseFunc = void(*)(NodeT&, TMessage&);

    template <typename NodeT>
    constexpr static inline TParseFunc<NodeT> GetParser() {
        if constexpr (THandlers::template CanParse<NodeT>()) {
            return &THandlers::Exec;
        }
        return nullptr;
    }

    template <typename WriterType>
    static inline void Serialize(WriterType& writer, const TMessage& msg) {
        THandlers::Serialize(writer, msg);
    }

    static inline bool IsSerializationNeeded(const TMessage& msg) {
        return THandlers::IsSerializationNeeded(msg);
    }

public:
    THandlerBuilder(ConverterType& conv)
        : Conv(conv)
    { }

    auto ForEachInArray() {
        return TNext<NMethods::TForEachInArray>(Conv);
    }

    auto ForEachInMap() {
        return TNext<NMethods::TForEachInMap>(Conv);
    }

    template <typename FieldTraits>
    auto AddNew() {
        return TNext<NMethods::TAddNew<FieldTraits>>(Conv);
    }

    template <typename ...Ts>
    auto Append() {
        return TNext<NMethods::TAppend<Ts...>>(Conv);
    }

    template <typename FieldTraits>
    auto Sub() {
        return TNext<NMethods::TSub<FieldTraits>>(Conv);
    };

    template <const auto& Parser>
    auto Parse() {
        return TNext<NMethods::TParse<Parser>>(Conv);
    }

    template <typename FieldTraits, typename... Ts>
    auto SetValue() {
        return TNext<NMethods::TSetValue<FieldTraits, Ts...>>(Conv);
    };

    template <typename FieldTraits, typename ConvertType = TSoftConvert>
    auto SetSpareValue() {
        return TNext<NMethods::TSetSpareValue<FieldTraits, ConvertType>>(Conv);
    };

    template <typename FieldTraits>
    auto SetKey() {
        return TNext<NMethods::TSetKey<FieldTraits>>(Conv);
    };

    template <typename HandlerT>
    auto Custom() {
        return TNext<NMethods::TCustom<HandlerT>>(Conv);
    }

    template <const auto& Handler>
    auto Custom() {
        return TNext<NMethods::TCustomAllocated<Handler>>(Conv);
    }

    template <typename FieldTraits>
    auto Field() {
        return TFieldSetter<TThis, FieldTraits>(*this);
    }

    // --------------------------------------------------------------------------------------------
    // terminal versions are below

    auto From(TStringBuf path) {
        SetHandlers(path);
        return *this;
    }

    template <typename FieldTraits>
    auto SetKey(TStringBuf path) {
        SetKey<FieldTraits>().From(path);
        return *this;
    };

    template <typename ...Ts>
    auto SetValue(TStringBuf path) {
        SetValue<Ts...>().From(path);
        return *this;
    };

    template <typename ...Ts>
    auto SetSpareValue(TStringBuf path) {
        SetSpareValue<Ts...>().From(path);
        return *this;
    };

    template <typename ...Ts>
    auto Append(TStringBuf path) {
        Append<Ts...>().From(path);
        return *this;
    }

    template <typename ParserType>
    auto Parse(ParserType parser, TStringBuf path) {
        Parse(parser).From(path);
        return *this;
    }

    template <const auto& Parser>
    auto Parse(TStringBuf path) {
        Parse<Parser>().From(path);
        return *this;
    }

    template <typename HandlerT>
    auto Custom(TStringBuf path) {
        Custom<HandlerT>().From(path);
        return *this;
    }

    template <const auto& Handler>
    auto Custom(TStringBuf path) {
        Custom<Handler>().From(path);
        return *this;
    }

private:
    inline void SetHandlers(TStringBuf path) {
        Conv.AddParseHandler({
            GetParser<TRapidJsonNode>(),
            GetParser<const NJson::TJsonValue>()
        }, path);
        Conv.AddSerializeHandler({
            &TThis::Serialize<TRapidJsonWriter<T_MAP>>,
            &TThis::Serialize<TJsonValueWriter<NJson::JSON_MAP>>,
            &TThis::IsSerializationNeeded
        }, path);
    }

    ConverterType& Conv;
};




}  // namespace NAlice::NCuttlefish::NConvert
