#pragma once
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/cuttlefish/library/convert/converter.h>
#include <alice/cuttlefish/library/proto_converters/converters.h>
#include <library/cpp/json/json_value.h>


namespace NAlice::NCuttlefish::NAppHostServices::NConverter {

class TConvertException : public yexception { };


template <typename ProtoType>
using TConverter = NAlice::NCuttlefish::TConverter<ProtoType>;

const TConverter<NAliceProtocol::TSynchronizeStateEvent>& GetSynchronizeStateEventConverter();
const TConverter<NAliceProtocol::TEventException>& GetEventExceptionConverter();
const TConverter<NAliceProtocol::TDirective>& GetDirectiveConverter();

constexpr const TConverter<NAliceProtocol::TSessionContext>& GetSessionContextConverter() {
    return NAlice::NCuttlefish::SessionContextConverter();
}

constexpr const TConverter<NAliceProtocol::TEventHeader>& GetEventHeaderConverter() {
    return NAlice::NCuttlefish::MessageHeaderConverter();
}

}  // namespace NAlice::NCuttlefish::NAppHostServices::NConverter