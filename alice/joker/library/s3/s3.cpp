#include "s3.h"

#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/auth/AWSCredentialsProvider.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-core/include/aws/core/Aws.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3/include/aws/s3/model/CreateBucketRequest.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3/include/aws/s3/model/DeleteObjectRequest.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3/include/aws/s3/model/GetObjectRequest.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3/include/aws/s3/model/HeadBucketRequest.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3/include/aws/s3/model/PutObjectRequest.h>
#include <contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3/include/aws/s3/S3Client.h>

#include <util/datetime/base.h>
#include <util/stream/output.h>

#include <strstream>

namespace NAlice::NJoker {
namespace {

constexpr size_t BUFSIZE = 1024;

THolder<Aws::S3::S3Client> ConstructClient(TStringBuf host, TDuration timeout, const IS3Storage::TCredentials* credentials) {
    auto onAwsInit = []() {
        Aws::SDKOptions options;
        Aws::InitAPI(options);
        return true;
    };
    static const bool awsIniter = onAwsInit();
    Y_UNUSED(awsIniter);

    Aws::Client::ClientConfiguration conf;
    conf.region = "us-east-1";
    conf.scheme = Aws::Http::Scheme::HTTPS;
    conf.caPath = "/etc/ssl/certs/";
    conf.endpointOverride = host;
    conf.verifySSL = true;
    conf.requestTimeoutMs = timeout.MilliSeconds();

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> authProvider;
    if (credentials) {
        authProvider = std::make_shared<Aws::Auth::SimpleAWSCredentialsProvider>(credentials->Id, credentials->Secret);
    } else {
        authProvider = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
    }

    return MakeHolder<Aws::S3::S3Client>(authProvider, conf);
}

class TAwsApiStorageImpl : public IS3Storage {
public:
    TAwsApiStorageImpl(TStringBuf host, TStringBuf bucket, TDuration timeout, const TCredentials* credentials)
        : Bucket_{bucket}
        , Client_{ConstructClient(host, timeout, credentials)}
    {
        if (credentials) {
            BucketChecks();
        }
    }

    TStatus Get(TStringBuf key, IOutputStream& out) override {
        auto req = Aws::S3::Model::GetObjectRequest{}.WithBucket(Bucket_).WithKey(TString{key});
        auto resp = Client_->GetObject(req);
        if (!resp.IsSuccess()) {
            if (Aws::Http::HttpResponseCode::NOT_FOUND == resp.GetError().GetResponseCode()) {
                return TError{HTTP_NOT_FOUND} << "Key not found in s3 '" << key << "'";
            }
            return TError{TError::EType::Logic} << "Unable to get object '" << key << "' from bucket '" << Bucket_ << "': " << resp.GetError().GetMessage();
        }
        auto& bodyStream = resp.GetResult().GetBody();
        char buf[BUFSIZE];
        while (true) {
            bodyStream.read(buf, BUFSIZE);
            size_t count = bodyStream.gcount();
            if (count > 0) {
                out.Write(buf, count);
            } else {
                break;
            }
        }
        return Success();
    }

    TStatus Put(TStringBuf key, TBuffer buf) override {
        if (buf.Empty()) {
            return TError{TError::EType::Logic} << "Won't save empty data to s3";
        }

        auto req = Aws::S3::Model::PutObjectRequest{}.WithBucket(Bucket_).WithKey(TString{key});
        req.SetBody(std::make_shared<std::strstream>(buf.data(), buf.size()));
        auto resp = Client_->PutObject(req);
        if (resp.IsSuccess()) {
            return Success();
        }

        return TError{TError::EType::Logic} << "Unable to put key '" << key << "' into bucket '" << Bucket_ << "': " << resp.GetError().GetMessage();
    }

    TStatus Delete(TStringBuf key) override {
        auto req = Aws::S3::Model::DeleteObjectRequest{}.WithBucket(Bucket_).WithKey(TString{key});
        auto resp = Client_->DeleteObject(req);
        if (resp.IsSuccess()) {
            return Success();
        }

        return TError{TError::EType::Logic} << "Unable to put key '" << key << "' into bucket '" << Bucket_ << "': " << resp.GetError().GetMessage();
    }

private:
    void BucketChecks() {
        const auto headResp = Client_->HeadBucket(Aws::S3::Model::HeadBucketRequest{}.WithBucket(Bucket_));
        if (headResp.IsSuccess()) {
            return;
        }

        if (headResp.GetError().GetErrorType() == Aws::S3::S3Errors::RESOURCE_NOT_FOUND) {
            const auto createResp = Client_->CreateBucket(Aws::S3::Model::CreateBucketRequest{}.WithBucket(Bucket_));
            if (createResp.IsSuccess()) {
                return;
            }

            ythrow TBucketCheckException() << "Unable to create bucket '" << Bucket_ << "': " << createResp.GetError().GetMessage();
        }

        ythrow TBucketCheckException() << "Unable to head bucket '" << Bucket_ << "': " << headResp.GetError().GetMessage();
    }

private:
    TString Bucket_;
    THolder<Aws::S3::S3Client> Client_;
};

} // namespace

THolder<IS3Storage> IS3Storage::Create(TStringBuf host, TStringBuf bucket, TDuration timeout, const TMaybe<TCredentials>& credentials) {
    return MakeHolder<TAwsApiStorageImpl>(host, bucket, timeout, credentials.Get());
}

} // namespace NJoker
