#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>

#include <voicetech/library/messages/message.h>

#include <apphost/api/service/cpp/service_context.h>
#include <apphost/lib/service_testing/service_testing.h>


class TTestFixture {
public:
    TTestFixture();

    bool HasFlag(const TStringBuf flag);

public:
    NVoicetech::NUniproxy2::TMessage Message;
    NAliceProtocol::TRequestContext RequestContext;
    NAliceProtocol::TSessionContext SessionContext;
    TIntrusivePtr<NAppHost::NService::TTestContext> AppHostContext;
};

