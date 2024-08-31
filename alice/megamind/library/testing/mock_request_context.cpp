#include "mock_request_context.h"

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind {

using namespace testing;

TMockInitializer::TMockInitializer() {
    EXPECT_CALL(*this, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
    EXPECT_CALL(*this, StealCgi()).WillRepeatedly(Return(ByMove(Cgi)));
    EXPECT_CALL(*this, StealUri()).WillRepeatedly(Return(ByMove(Uri)));
    EXPECT_CALL(*this, StealHeaders()).WillRepeatedly(Return(ByMove(Headers)));
}

void TMockRequestCtx::DefaultInit() {
    EXPECT_CALL(*this, NodeLocation()).WillRepeatedly(Return(TStringBuf("NODE")));
    EXPECT_CALL(*this, Body()).WillRepeatedly(ReturnRef(Default<TString>()));
}

} // namespace NAlice::NMegamind
