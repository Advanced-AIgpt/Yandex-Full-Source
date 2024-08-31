#include "apphost_http_request_signer.h"
#include "constants.h"

#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/auth/AWSAuthSigner.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/auth/AWSCredentialsProvider.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/http/standard/StandardHttpRequest.h>

#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>
#include <util/system/env.h>

namespace NAlice::NCuttlefish::NAws {

namespace {

Aws::Http::HttpMethod AppHostMethodToAwsMethod(NAppHostHttp::THttpRequest::EMethod apphostMethod) {
    switch (apphostMethod) {
        case NAppHostHttp::THttpRequest::Get: {
            return Aws::Http::HttpMethod::HTTP_GET;
        }
        case NAppHostHttp::THttpRequest::Post: {
            return Aws::Http::HttpMethod::HTTP_POST;
        }
        case NAppHostHttp::THttpRequest::Put: {
            return Aws::Http::HttpMethod::HTTP_PUT;
        }
        case NAppHostHttp::THttpRequest::Delete: {
            return Aws::Http::HttpMethod::HTTP_DELETE;
        }
        case NAppHostHttp::THttpRequest::Head: {
            return Aws::Http::HttpMethod::HTTP_HEAD;
        }
        case NAppHostHttp::THttpRequest::Patch: {
            return Aws::Http::HttpMethod::HTTP_PATCH;
        }

        case NAppHostHttp::THttpRequest::Connect:
        case NAppHostHttp::THttpRequest::Options:
        case NAppHostHttp::THttpRequest::Trace:
        // We don't want to use default (without default adding a new enum value will result in a compilation error)
        // So we need this to handle all cases
        case NAppHostHttp::THttpRequest_EMethod_THttpRequest_EMethod_INT_MAX_SENTINEL_DO_NOT_USE_:
        case NAppHostHttp::THttpRequest_EMethod_THttpRequest_EMethod_INT_MIN_SENTINEL_DO_NOT_USE_: {
            ythrow yexception() << NAppHostHttp::THttpRequest::EMethod_Name(apphostMethod) << " method not implemented in aws api";
        }
    };
}

template<const TString& serviceName, const TString& region>
static const TAppHostHttpRequestSigner& GetDefaultSignerInstance() {
    return *Singleton<TAppHostHttpRequestSigner>(
        GetEnv(AWS_ACCESS_KEY_ID_ENV_NAME),
        GetEnv(AWS_SECRET_ACCESS_KEY_ENV_NAME),
        serviceName,
        region
    );
}

} // namespace

class TAppHostHttpRequestSigner::TImpl {
public:
    TImpl(
        const TString& awsAccessKeyId,
        const TString& awsSecretAccessKey,
        const TString& serviceName,
        const TString& region
    )
        : AwsCredentialsProvider_(
            std::make_shared<Aws::Auth::SimpleAWSCredentialsProvider>(
                awsAccessKeyId.c_str(),
                awsSecretAccessKey.c_str()
            )
        )
        , AwsAuthV4Signer_(
            AwsCredentialsProvider_,
            serviceName.c_str(),
            region.c_str(),
            // WARNING: Payload signing not implemented
            // If you just change default nothing will happed, you also need to add some code to SignRequest
            Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
            /* urlEscapePath = */ false
        )
    {
    }

    void SignRequest(const TString& host, NAppHostHttp::THttpRequest& httpRequest) const {
        const TString url = TStringBuilder()
            // httpRequest.GetScheme() is useless (apphost just ignore this field)
            // And signature with Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never policy only works for https requests
            // proof: https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/source/auth/AWSAuthSigner.cpp?rev=r8861215#L204-211
            // So we assume that https is used
            << "https://"
            << host
            << (httpRequest.GetPath().empty() ? "" : httpRequest.GetPath())
        ;

        Aws::Http::Standard::StandardHttpRequest awsHttpRequest(url.c_str(), AppHostMethodToAwsMethod(httpRequest.GetMethod()));
        for (const auto& header : httpRequest.GetHeaders()) {
            awsHttpRequest.SetHeaderValue(header.GetName(), header.GetValue());
        }
        // We do not sign payload of http request
        // So just do not fill it and save CPU

        if (!AwsAuthV4Signer_.SignRequest(awsHttpRequest)) {
            // API is bad (it is impossible to get fail reason :( )
            ythrow yexception() << "Failed to sign http request";
        }

        httpRequest.ClearHeaders();
        for (const auto& [name, value] : awsHttpRequest.GetHeaders()) {
            auto* header = httpRequest.AddHeaders();
            header->SetName(name.c_str());
            header->SetValue(value.c_str());
        }
    }

private:
    // Not THolder because of aws contrib API
    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AwsCredentialsProvider_;
    Aws::Client::AWSAuthV4Signer AwsAuthV4Signer_;
};

TAppHostHttpRequestSigner::TAppHostHttpRequestSigner(
    const TString& awsAccessKeyId,
    const TString& awsSecretAccessKey,
    const TString& serviceName,
    const TString& region
)
    : Impl_(
        MakeHolder<TImpl>(
            awsAccessKeyId,
            awsSecretAccessKey,
            serviceName,
            region
        )
    )
{}

TAppHostHttpRequestSigner::~TAppHostHttpRequestSigner() {
}

void TAppHostHttpRequestSigner::SignRequest(const TString& host, NAppHostHttp::THttpRequest& httpRequest) const {
    Impl_->SignRequest(host, httpRequest);
}

const TAppHostHttpRequestSigner& GetDefaultS3RequestSignerInstance() {
    return GetDefaultSignerInstance<S3_SERVICE_NAME, S3_YANDEX_INTERNAL_REGION>();
}

} // namespace NAlice::NCuttlefish::NAws
