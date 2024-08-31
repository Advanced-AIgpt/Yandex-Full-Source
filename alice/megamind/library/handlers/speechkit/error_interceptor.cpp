#include "error_interceptor.h"

#include <alice/megamind/library/response_meta/error.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

TResponseErrorInterceptor::TResponseErrorInterceptor(IHttpResponse& httpResponse)
    : HttpResponse_{httpResponse}
{
}

TResponseErrorInterceptor& TResponseErrorInterceptor::EnableSensors(NMetrics::ISensors& sensors) {
    Sensors_ = &sensors;
    return *this;
}

TResponseErrorInterceptor& TResponseErrorInterceptor::EnableLogging(TRTLogger& logger) {
    Logger_ = &logger;
    return *this;
}

TResponseErrorInterceptor& TResponseErrorInterceptor::SetNetLocation(TStringBuf netLocation) {
    NetLocation_ = netLocation;
    return *this;
}

TResponseErrorInterceptor& TResponseErrorInterceptor::SetLabel(TStringBuf label) {
    Label_ = label;
    return *this;
}

void TResponseErrorInterceptor::ProcessExecutor(std::function<TStatus()> executor) {
    TMaybe<TErrorMetaBuilder> errorMetaBuilder;

    try {
        if (auto err = executor()) {
            errorMetaBuilder.ConstructInPlace(*err);
        }
    } catch (const TRequestCtx::TBadRequestException& e) {
        errorMetaBuilder.ConstructInPlace(TError{TError::EType::BadRequest} << e.what());
    } catch (...) {
        auto err = TError{TError::EType::Exception} << "Caught exception while handling '" << Label_ << "' request : " << CurrentExceptionMessage();
        if (Logger_) {
            LOG_ERROR(*Logger_) << err.ErrorMsg;
        }
        errorMetaBuilder.ConstructInPlace(err);
    }

    if (errorMetaBuilder.Defined()) {
        if (!NetLocation_.Empty()) {
            errorMetaBuilder->SetNetLocation(NetLocation_);
        }

        if (Sensors_) {
            Sensors_->IncRate(NSignal::APPHOST_REQUEST_EXCEPTION);
            AddSensorForHttpResponseCode(*Sensors_, errorMetaBuilder->HttpCode());
        }

        errorMetaBuilder->ToHttpResponse(HttpResponse_);
        if (Logger_) {
            LOG_INFO(*Logger_) << "Megamind response: " << errorMetaBuilder->AsJson();
        }
    }
}

} // namespace NAlice::NMegamind
