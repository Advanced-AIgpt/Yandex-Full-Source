#include "blackbox_http.h"
#include "blackbox.h"

namespace NAlice {

TBlackBoxHttpFetcher::TBlackBoxHttpFetcher()
    : State_{TBlackBoxError{EBlackBoxErrorCode::NoRequest}}
{
}

TBlackBoxStatus TBlackBoxHttpFetcher::StartRequest(NHttpFetcher::TRequest& request, TStringBuf userIp, const TMaybe<TString>& authToken) {
    NHttpFetcher::TRequestBuilder requestBuilder{request};

    if (auto e = PrepareBlackBoxRequest(requestBuilder, userIp, authToken)) {
        State_ = std::move(*e);
    } else {
        State_ = request.Fetch();
    }

    return BlackBoxSuccess();
}

TBlackBoxErrorOr<TString> TBlackBoxHttpFetcher::Response() {
    class TVisitor {
    public:
        TVisitor(TState& state)
            : State(state)
        {
        }

        TBlackBoxErrorOr<TString> operator()(THandleRef handle) {
            auto response = handle->Wait();
            if (!response) {
                TBlackBoxError error{EBlackBoxErrorCode::NoResponse};
                State = error;
                return std::move(error);
            }

            if (response->IsError()) {
                TBlackBoxError error = TBlackBoxError{EBlackBoxErrorCode::BadData} << "BlackBox response error: " << response->GetErrorText();
                State = error;
                return std::move(error);
            }

            State = response;
            return response->Data;
        }

        TBlackBoxErrorOr<TString> operator()(TResponseRef response) {
            Y_ASSERT(response);
            return response->Data;
        }

        TBlackBoxErrorOr<TString> operator()(TBlackBoxError error) {
            return std::move(error);
        }

    private:
        TState& State;
    };

    return std::visit(TVisitor(State_), State_);
}

TBlackBoxErrorOr<TBlackBoxFullUserInfoProto> TBlackBoxHttpFetcher::GetFullInfo() {
    return Response().AndThen([](TString&& content) { return TBlackBoxApi{}.ParseFullInfo(content); });
}

TBlackBoxErrorOr<TString> TBlackBoxHttpFetcher::GetUid() {
    return Response().AndThen([](TString&& content) { return TBlackBoxApi{}.ParseUid(content); });
}

TBlackBoxErrorOr<TString> TBlackBoxHttpFetcher::GetTVM2UserTicket() {
    return Response().AndThen([](TString&& content) { return TBlackBoxApi{}.ParseTvm2UserTicket(content); });
}

} // namespace NAlice
