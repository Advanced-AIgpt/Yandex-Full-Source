#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>

#include <apphost/api/service/cpp/service_context.h>
#include <apphost/lib/service_testing/service_testing.h>


class TTestFixture {
public:
    TTestFixture();

public:
    TIntrusivePtr<NAppHost::NService::TTestContext> AppHostContext;
};

