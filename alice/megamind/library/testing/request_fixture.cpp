#include "request_fixture.h"

#include <alice/library/logger/logger.h>

using namespace testing;

namespace NAlice::NMegamind {
namespace {

} // namespace

// TMockRequestBuilder ---------------------------------------------------------
TMockRequestBuilder::TMockRequestBuilder(TMockGlobalContext& globalCtx, const TString& utterance)
    : GlobalCtx_{globalCtx}
{
    EXPECT_CALL(Context_, Responses()).WillRepeatedly(ReturnRef(Responses_));
    EXPECT_CALL(Context_, Session()).WillRepeatedly(Return(nullptr));
    EXPECT_CALL(Context_, PreviousScenarioSession()).WillRepeatedly(Return(TSessionProto::TScenarioSession::default_instance()));
    EXPECT_CALL(Context_, PolyglotUtterance()).WillRepeatedly(Return(utterance));
    EXPECT_CALL(Context_, StackEngineCore()).WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));
}

TMockContext& TMockRequestBuilder::Context() {
    return Context_;
}

TMockGlobalContext& TMockRequestBuilder::GlobalCtx() {
    return GlobalCtx_;
}

TMockResponses& TMockRequestBuilder::Responses() {
    return Responses_;
}

TMockRequestCtx& TMockRequestBuilder::RequestCtx() {
    if (!RequestCtx_.Defined()) {
        RequestCtx_.ConstructInPlace(&GlobalCtx_, &RequestCtxInitializer_);
    }

    return RequestCtx_.GetRef();
}

TMockLightWalkerRequestCtx& TMockRequestBuilder::LightWalkerCtx() {
    if (!LightWalkerCtx_.Defined()) {
        LightWalkerCtx_.ConstructInPlace();
        EXPECT_CALL(LightWalkerCtx_.GetRef(), Ctx()).WillRepeatedly(ReturnRef(Context_));
        EXPECT_CALL(Const(LightWalkerCtx_.GetRef()), Ctx()).WillRepeatedly(ReturnRef(Context_));
    }

    return LightWalkerCtx_.GetRef();
}

// TPredefinedRequestFixture ---------------------------------------------------
TPredefinedRequestFixture::TPredefinedRequestFixture() {
    GlobalCtx_.GenericInit();
}

TMockRequestBuilder TPredefinedRequestFixture::MockRequestBuilder(const TString& utterance) {
    return TMockRequestBuilder{GlobalCtx_, utterance};
}

} // namespace NAlice::NMegamind
