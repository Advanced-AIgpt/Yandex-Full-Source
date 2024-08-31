#pragma once

#include "mock_context.h"
#include "mock_global_context.h"
#include "mock_request_context.h"
#include "mock_responses.h"
#include "mock_walker_request.h"

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TMockRequestBuilder {
public:
    TMockRequestBuilder(TMockGlobalContext& globalCtx, const TString& utterance);

    TMockResponses& Responses();
    TMockContext& Context();
    TMockRequestCtx& RequestCtx();
    TMockLightWalkerRequestCtx& LightWalkerCtx();
    TMockGlobalContext& GlobalCtx();

private:
    TMockGlobalContext& GlobalCtx_;
    testing::StrictMock<TMockInitializer> RequestCtxInitializer_;
    testing::StrictMock<TMockResponses> Responses_;
    testing::StrictMock<TMockContext> Context_;
    TMaybe<testing::StrictMock<TMockRequestCtx>> RequestCtx_;
    TMaybe<testing::StrictMock<TMockLightWalkerRequestCtx>> LightWalkerCtx_;
    TQualityStorage QualityStorage_;
};

class TPredefinedRequestFixture : public NUnitTest::TBaseFixture {
public:
    TPredefinedRequestFixture();

    TMockGlobalContext& GlobalCtx() {
        return GlobalCtx_;
    }

    TMockRequestBuilder MockRequestBuilder(const TString& utterance);

private:
    testing::NiceMock<TMockGlobalContext> GlobalCtx_;
};

} // namespace NAlice::NMegamind
