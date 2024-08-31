#include "ptune_storage.h"

#include <alice/boltalka/generative/inference/core/util.h>

#include <util/memory/blob.h>
#include <util/system/env.h>

namespace NGenerativeBoltalka {

TPtuneStorage::TPtuneStorage(const size_t& maxSize, const THashMap<TString, TString>& precharged)
    : Cache(maxSize)
{
    NYT::JoblessInitialize();
    YTClient = NYT::CreateClient("hahn");

    TMaybe<NAlice::NJoker::IS3Storage::TCredentials> creds;
    if (!GetEnv("ACCESS_KEY_ID").empty() && !GetEnv("SECRET_ACCESS_KEY").empty()) {
        creds = NAlice::NJoker::IS3Storage::TCredentials{GetEnv("ACCESS_KEY_ID"), GetEnv("SECRET_ACCESS_KEY")};
    }
    S3Client = NAlice::NJoker::IS3Storage::Create("", "zeliboba-p-tunes", TDuration::Seconds(5), creds);

    for (const auto& [key, path] : precharged) {
        const auto proto = std::make_shared<NDict::NMT::NYNMT::TModelProtoWithMeta>(NDict::NMT::NYNMT::ReadModelProtoWithMeta(TBlob::FromFile(path)));

        const auto ptuneEmbeddings = std::make_shared<TVector<TVector<float>>>(LoadPtuneFromNpz(proto.get()));
        Cache.Update(std::make_pair(YT, key), {
            .PtuneEmbeddings = ptuneEmbeddings,
            .Proto = proto});
        Cache.Update(std::make_pair(S3, key), {
            .PtuneEmbeddings = ptuneEmbeddings,
            .Proto = proto});
    }
}

TPtuneStorage::TStorageItem TPtuneStorage::DownloadFromYT(const TString& path) {
    if (!YTClient->Exists(path)) {
        ythrow yexception() << "Given YT file doesn't exist";
    }
    if (YTClient->Get(path + "/@uncompressed_data_size").AsInt64() > 1024 * 1024) {  // 1 MB
        ythrow yexception() << "Given YT file is too big";
    }
    auto reader = YTClient->CreateFileReader(path);
    const auto proto = std::make_shared<NDict::NMT::NYNMT::TModelProtoWithMeta>(NDict::NMT::NYNMT::ReadModelProtoWithMeta(TBlob::FromStream(*reader)));
    auto ptuneEmbeddings = std::make_shared<TVector<TVector<float>>>(LoadPtuneFromNpz(proto.get()));
    return {
        .PtuneEmbeddings = ptuneEmbeddings,
        .Proto = proto
    };
}

TPtuneStorage::TStorageItem TPtuneStorage::DownloadFromS3(const TString& objectName) {
    TStringStream ptuneDataStream;
    const auto error = S3Client->Get(objectName, ptuneDataStream);
    if (error) {
        ythrow yexception() << error->AsString();
    }
    const auto blob = TBlob::FromStream(ptuneDataStream);
    const auto proto = std::make_shared<NDict::NMT::NYNMT::TModelProtoWithMeta>(NDict::NMT::NYNMT::ReadModelProtoWithMeta(blob));
    auto ptuneEmbeddings = std::make_shared<TVector<TVector<float>>>(LoadPtuneFromNpz(proto.get()));
    return {
        .PtuneEmbeddings = ptuneEmbeddings,
        .Proto = proto
    };
}

TPtuneStorage::TStorageItem TPtuneStorage::Get(const TString& path, const EStorageType& storageType) {
    {
        const TGuard<TMutex> cacheGuard(CacheLock);
        if (const auto& resultIter = Cache.Find(std::make_pair(storageType, path)); resultIter != Cache.End()) {
            return resultIter.Value();
        }
    }

    {
        const TGuard<TMutex> downloadGuard(DownloadLock);
        if (const auto& resultIter = Cache.Find(std::make_pair(storageType, path)); resultIter != Cache.End()) {
            return resultIter.Value();
        }

        TPtuneStorage::TStorageItem result;
        if (storageType == EStorageType::YT) {
            result = DownloadFromYT(path);
        } else if (storageType == EStorageType::S3) {
            result = DownloadFromS3(path);
        } else {
            ythrow yexception() << "Unknown storage type";
        }
        const TGuard<TMutex> cacheGuard(CacheLock);
        Cache.Update(std::make_pair(storageType, path), result);
        return result;
    }
    ythrow yexception() << "Problem with given ptune path: " << path;
}

} // namespace NGenerativeBoltalka
