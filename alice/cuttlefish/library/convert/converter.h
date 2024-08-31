#pragma once
#include <alice/cuttlefish/library/convert/private/process_tree.h>
#include <alice/cuttlefish/library/convert/builder.h>
#include <alice/cuttlefish/library/convert/rapid_node.h>
#include <alice/cuttlefish/library/convert/json_value_writer.h>
#include <utility>
#include <type_traits>


namespace NAlice::NCuttlefish::NConvert {

template <typename MessageType>
class TConverter {
public:
    using TThis     = TConverter<MessageType>;
    using TMessage  = MessageType;

    struct TParseHandler {
        void(*ParseRapid)(TRapidJsonNode&, TMessage&) = nullptr;
        void(*ParseValue)(const NJson::TJsonValue&, TMessage&) = nullptr;

        inline operator bool() const {
            return ParseRapid || ParseValue;
        }

        inline void operator()(TRapidJsonNode& node, TMessage& msg) const {
            Y_ASSERT(ParseRapid);
            return ParseRapid(node, msg);
        }

        inline void operator()(const NJson::TJsonValue& node, TMessage& msg) const {
            Y_ASSERT(ParseValue);
            return ParseValue(node, msg);
        }
    };

    struct TSerializeHandler {
        void(*SerializeRapid)(TRapidJsonWriter<T_MAP>&, const TMessage&) = nullptr;
        void(*SerializeValue)(TJsonValueWriter<NJson::JSON_MAP>&, const TMessage&) = nullptr;
        bool(*Needed)(const TMessage&) = nullptr;

        inline operator bool() const {
            return SerializeRapid != nullptr;
        }

        inline void Serialize(TRapidJsonWriter<T_MAP>& writer, const TMessage& msg) const {
            return SerializeRapid(writer, msg);
        }

        inline void Serialize(TJsonValueWriter<NJson::JSON_MAP>& writer, const TMessage& msg) const {
            return SerializeValue(writer, msg);
        }
    };

public:
    void AddParseHandler(TParseHandler handler, TStringBuf path) {
        Y_ASSERT(handler);
        ParseTree.Add(path, handler);
    }

    void AddSerializeHandler(TSerializeHandler handler, TStringBuf path) {
        Y_ASSERT(handler);
        SerializeTree.Add(path, handler);
    }

    // TODO: embed Builder's methods into this class
    inline THandlerBuilder<TThis> Build() {
        return THandlerBuilder<TThis>(*this);
    }

    template <typename NodeT>
    inline void Parse(TMessage& msg, NodeT node) const {
        if constexpr (std::is_convertible_v<NodeT&, TRapidJsonNode&>) {
            ParseTree.Parse(node, msg);
        } else {
            ParseTree.Parse(node.GetMapSafe(), msg);
        }
    }

    template <typename WriterT>
    void Serialize(const TMessage& msg, WriterT&& writer) const {
        SerializeTree.Serialize(writer.Map(), msg);
    }

private:
    NPrivate::TProcessTree<TParseHandler> ParseTree;
    NPrivate::TProcessTree<TSerializeHandler> SerializeTree;
};


template <typename MessageT>
using TJsonValueConverter = TConverter<MessageT>;

template <typename MessageT>
using TRapidJsonConverter = TConverter<MessageT>;

}  // namespace NAlice::NCuttlefish::NConvert