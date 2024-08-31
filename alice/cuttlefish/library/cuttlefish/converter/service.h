#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <apphost/api/service/cpp/service.h>


namespace NAlice::NCuttlefish::NAppHostServices {

// Parse raw WebSocket message (NAliceProtocol::TWsEvent) into Protobuf (NAliceProtocol::T*Event)
bool RawToProtobufImpl(NAppHost::IServiceContext& ctx, TLogContext logContext, NAliceProtocol::TEvent& event);
void RawToProtobuf(NAppHost::IServiceContext& ctx, TLogContext logContext);

// Serialize Protobuf (NAliceProtocol::T*Event) into raw WebSocket message (NAliceProtocol::TWsEvent)
void ProtobufToRawImpl(NAppHost::IServiceContext& ctx, const NAliceProtocol::TDirective& directive);
void ProtobufToRaw(NAppHost::IServiceContext& ctx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices
