#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>


namespace NAlice::NCuttlefish::NAppHostServices {
    // Parse raw WebSocket message (NAliceProtocol::TWsEvent) into Protobuf (NAliceProtocol::T*Event)
    NThreading::TPromise<void> StreamRawToProtobuf(NAppHost::TServiceContextPtr ctx, TLogContext logContext);

    // Serialize Protobuf (NAliceProtocol::T*Event) into raw WebSocket message (NAliceProtocol::TWsEvent)
    NThreading::TPromise<void> StreamProtobufToRaw(NAppHost::TServiceContextPtr ctx, TLogContext logContext);

}
