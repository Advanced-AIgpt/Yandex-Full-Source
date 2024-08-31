#include "mock_responses.h"

namespace NAlice {

TMockResponses::TMockResponses() {
    using namespace testing;
    EXPECT_CALL(*this, WizardResponse(_)).WillRepeatedly(ReturnRef(WizardResponse_));
    EXPECT_CALL(*this, BegemotResponseRewrittenRequestResponse(_)).WillRepeatedly(ReturnRef(BegemotResponseRewrittenRequestResponse_));
    EXPECT_CALL(*this, BlackBoxResponse(_)).WillRepeatedly(ReturnRef(BlackBoxResponse_));
    EXPECT_CALL(*this, PersonalIntentsResponse(_)).WillRepeatedly(ReturnRef(PersonalIntentsResponse_));
}

void TMockResponses::SetWizardResponse(TWizardResponse&& wizardResponse) {
    WizardResponse_ = std::move(wizardResponse);
    BegemotResponseRewrittenRequestResponse_.SetData(WizardResponse_.GetRewrittenRequest());
}

void TMockResponses::SetBlackBoxResponse(TBlackBoxFullUserInfoProto&& blackBoxResponse) {
    BlackBoxResponse_ = std::move(blackBoxResponse);
}

void TMockResponses::SetPersonalIntentsResponse(NKvSaaS::TPersonalIntentsResponse&& personalIntentsResponse) {
    PersonalIntentsResponse_ = std::move(personalIntentsResponse);
}

} // namespace NAlice
