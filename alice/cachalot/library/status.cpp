#include <alice/cachalot/library/status.h>


namespace NCachalot {


TStatus::operator NNeh::IRequest::TResponseError() const {
    switch (Status) {
        case EResponseStatus::BAD_REQUEST:
            return NNeh::IRequest::BadRequest;
        case EResponseStatus::SERVICE_UNAVAILABLE:
            return NNeh::IRequest::ServiceUnavailable;
        case EResponseStatus::NOT_IMPLEMENTED:
            return NNeh::IRequest::NotImplemented;
        case EResponseStatus::QUERY_PREPARE_FAILED:
        case EResponseStatus::QUERY_EXECUTE_FAILED:
        case EResponseStatus::INTERNAL_ERROR:
            return NNeh::IRequest::InternalError;
        case EResponseStatus::TOO_MANY_REQUESTS:
            return NNeh::IRequest::TooManyRequests;
        default:
            return NNeh::IRequest::InternalError;
    }
    return NNeh::IRequest::InternalError;
}


}   // namespace NCachalot
