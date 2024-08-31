#pragma once
#include <alice/cuttlefish/library/convert/converter.h>
#include <alice/cuttlefish/library/convert/json_value_writer.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>


namespace NAlice::NCuttlefish {

template <typename ProtoType>
using TConverter = NConvert::TJsonValueConverter<ProtoType>;

namespace NProtoConverters {
    extern const TConverter<NAliceProtocol::TSessionContext> SessionContextConv;
    extern const TConverter<NAliceProtocol::TEventHeader> MessageHeaderConv;
};

template <typename ConverterType>
inline auto JsonToProtobuf(const ConverterType& conv, const NJson::TJsonValue& node) {
    typename ConverterType::TMessage message;
    conv.Parse(message, node);
    return message;
}

template <typename ConverterType>
inline auto ProtobufToJson(const ConverterType& conv, const typename ConverterType::TMessage& msg) {
    NAlice::NCuttlefish::NConvert::TRapidJsonRootWriter w;
    conv.Serialize(msg, w);
    return TString(w.GetString());
}

template <typename ConverterType>
inline NJson::TJsonValue ProtobufToJsonValue(const ConverterType& conv, const typename ConverterType::TMessage& msg) {
    NAlice::NCuttlefish::NConvert::TJsonValueRootWriter w;
    conv.Serialize(msg, w);
    return w;
}

constexpr const TConverter<NAliceProtocol::TSessionContext>& SessionContextConverter() {
    return NProtoConverters::SessionContextConv;
}
constexpr const TConverter<NAliceProtocol::TEventHeader>& MessageHeaderConverter() {
    return NProtoConverters::MessageHeaderConv;
}

}  // namespace NAlice::NCuttlefish
