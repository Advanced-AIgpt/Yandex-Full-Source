#include "fake.h"

#include <alice/cuttlefish/library/logging/dlog.h>

#include <util/generic/hash.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>

using namespace NAlice::NTtsCache;

namespace {

// Fake in-memory cache
// Reset only by restart or OOM
static TMutex CACHE_MUTEX;
static THashMap<TString, NProtobuf::TCacheEntry> CACHE;

void AddCacheRecord(const TString& key, const NProtobuf::TCacheEntry& cacheRecord) {
    TGuard<TMutex> g(CACHE_MUTEX);
    CACHE[key] = cacheRecord;

    {
        auto cacheRecordCopy = cacheRecord;
        cacheRecordCopy.SetAudio(TStringBuilder() << "size=" << cacheRecord.GetAudio().size());
        DLOG("Add cache record with key '" << key << "': " << cacheRecordCopy.ShortUtf8DebugString());
    }
}

TMaybe<NProtobuf::TCacheEntry> GetCacheRecord(const TString& key) {
    TGuard<TMutex> g(CACHE_MUTEX);
    if (auto ptr = CACHE.FindPtr(key)) {
        DLOG("Cache hit with key '" << key << "'");
        return *ptr;
    }
    DLOG("Cache miss with key '" << key << "'");
    return Nothing();
}

} // namespace

void TFake::ProcessCacheSetRequest(const NProtobuf::TCacheSetRequest& cacheSetRequest) {
    if (Closed_) {
        return;
    }

    AddCacheRecord(cacheSetRequest.GetKey(), cacheSetRequest.GetCacheEntry());
    Callbacks_->OnCacheSetRequestCompleted(cacheSetRequest.GetKey(), /* error = */ Nothing());
}

void TFake::ProcessCacheGetRequest(const NProtobuf::TCacheGetRequest& cacheGetRequest) {
    if (Closed_) {
        return;
    }

    NProtobuf::TCacheGetResponse cacheGetResponse;
    cacheGetResponse.SetKey(cacheGetRequest.GetKey());

    if (TMaybe<NProtobuf::TCacheEntry> cacheEntry = GetCacheRecord(cacheGetRequest.GetKey()); cacheEntry.Defined()) {
        cacheGetResponse.SetStatus(NProtobuf::ECacheGetResponseStatus::HIT);
        cacheGetResponse.MutableCacheEntry()->CopyFrom(*cacheEntry);
    } else {
        cacheGetResponse.SetStatus(NProtobuf::ECacheGetResponseStatus::MISS);
    }

    Callbacks_->OnCacheGetResponse(cacheGetResponse);
}

void TFake::ProcessCacheWarmUpRequest(const NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) {
    if (Closed_) {
        return;
    }

    Y_UNUSED(cacheWarmUpRequest);
    Callbacks_->OnCacheWarmUpRequestCompleted(cacheWarmUpRequest.GetKey(), /* error = */ Nothing());
}

void TFake::Cancel() {
    if (Closed_) {
        return;
    }

    Callbacks_->OnClosed();
    Closed_ = true;
}
