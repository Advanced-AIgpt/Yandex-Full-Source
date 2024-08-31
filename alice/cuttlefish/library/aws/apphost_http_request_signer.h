#pragma once

#include <apphost/lib/proto_answers/http.pb.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NCuttlefish::NAws {

class TAppHostHttpRequestSigner {
public:
    TAppHostHttpRequestSigner(
        const TString& awsAccessKeyId,
        const TString& awsSecretAccessKey,
        const TString& serviceName,
        const TString& region
    );
    ~TAppHostHttpRequestSigner();

    // Add signature headers to http request
    void SignRequest(const TString& host, NAppHostHttp::THttpRequest& httpRequest) const;

private:
    class TImpl;
    THolder<TImpl> Impl_;
};

// Default signer singletons
// They use standard aws env variables to get access key id and secret access key
// and default service names and regions for yandex
const TAppHostHttpRequestSigner& GetDefaultS3RequestSignerInstance();

} // namespace NAlice::NCuttlefish::NAws
