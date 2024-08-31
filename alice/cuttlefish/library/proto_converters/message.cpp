#include "converters.h"
#include "enum_converter.h"
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.traits.pb.h>


namespace NAlice::NCuttlefish::NProtoConverters {

namespace {

TConverter<NAliceProtocol::TEventHeader> CreateMessageHeaderConverter()
{
    namespace NAliceProtocolTraits = NProtoTraits::NAliceProtocol;

    static const TEnumFieldConverter<NAliceProtocolTraits::TEventHeader::Namespace, NAliceProtocol::TEventHeader::UNKNOWN_NAMESPACE> NamespaceConv({
        {"System", NAliceProtocol::TEventHeader::SYSTEM},
    });
    static const TEnumFieldConverter<NAliceProtocolTraits::TEventHeader::Name, NAliceProtocol::TEventHeader::UNKNOWN_NAME> NameConv({
        {"SynchronizeState", NAliceProtocol::TEventHeader::SYNCHRONIZE_STATE},
        {"EventException", NAliceProtocol::TEventHeader::EVENT_EXCEPTION},
        {"InvalidAuth", NAliceProtocol::TEventHeader::INVALID_AUTH},
        {"SynchronizeStateResponse", NAliceProtocol::TEventHeader::SYNCHRONIZE_STATE_RESPONSE},
    });

    TConverter<NAliceProtocol::TEventHeader> conv;
    auto b = conv.Build();
    b.Custom<NamespaceConv>("namespace");
    b.Custom<NameConv>("name");
    b.SetValue<NAliceProtocolTraits::TEventHeader::MessageId>().From("messageId");
    b.SetValue<NAliceProtocolTraits::TEventHeader::RefMessageId>().From("refMessageId");
    b.SetValue<NAliceProtocolTraits::TEventHeader::StreamId>().From("streamId");
    return conv;
}

}  // anonymous namespace

const TConverter<NAliceProtocol::TEventHeader> MessageHeaderConv = CreateMessageHeaderConverter();

}  // namespace NAlice::NCuttlefish::NProtoConverters
