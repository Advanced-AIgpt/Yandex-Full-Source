#pragma once

#include <alice/joker/library/s3/s3.h>
#include <mapreduce/yt/interface/client.h>

#include <dict/mt/libs/nn/ynmt/config_helper/model_reader.h>

#include <library/cpp/cache/cache.h>

#include <util/generic/ptr.h>
#include <util/system/mutex.h>

namespace NGenerativeBoltalka {

class TPtuneStorage {
public:
    enum EStorageType {
        YT,
        S3,
    };
    struct TStorageItem {
        std::shared_ptr<TVector<TVector<float>>> PtuneEmbeddings;
        std::shared_ptr<NDict::NMT::NYNMT::TModelProtoWithMeta> Proto;
    };

public:
    TPtuneStorage(const size_t& maxSize, const THashMap<TString, TString>& precharged = {});

    TStorageItem Get(const TString& path, const EStorageType& storageType);

private:
    TMutex CacheLock;
    TMutex DownloadLock;
    TLRUCache<std::pair<EStorageType, TString>, TStorageItem> Cache;
    NYT::IClientPtr YTClient;
    THolder<NAlice::NJoker::IS3Storage> S3Client;

private:
    TStorageItem DownloadFromYT(const TString& path);
    TStorageItem DownloadFromS3(const TString& objectName);
};

} // namespace NGenerativeBoltalka
