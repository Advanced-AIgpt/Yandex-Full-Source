#include "memory_storage.h"

namespace NAlice::NJoker {

TMemoryStorage::~TMemoryStorage() = default;

TMemoryStorage::TFutureResult TMemoryStorage::Get(TStringBuf sessionId, TStringBuf key, TOnNotFound onNotFound) {
    bool isNew = false;
    TPromiseResult* promise = nullptr;
    with_lock (Lock_) {
        auto* stubsMap = SessionStubs_.FindPtr(sessionId);
        if (!stubsMap) {
            stubsMap = &SessionStubs_.emplace(sessionId, THashMap<TString, TPromiseResult>{}).first->second;
        }

        Y_ASSERT(stubsMap);

        promise = stubsMap->FindPtr(key);
        if (!promise) {
            promise = &stubsMap->emplace(key, NThreading::NewPromise<TFutureResult::value_type>()).first->second;
            isNew = true;
        }
    }

    if (isNew) {
        try {
            promise->SetValue(onNotFound());
        }
        catch (...) {
            promise->SetException(CurrentExceptionMessage());
        }
    }

    return promise->GetFuture();
}

void TMemoryStorage::ForEach(TStringBuf sessionId, TOnEachStub onEachStub) {
    auto* stubsMap = SessionStubs_.FindPtr(sessionId);
    if (!stubsMap) {
        return;
    }

    for (auto& kv : *stubsMap) {
        onEachStub(kv.first, kv.second.GetValue());
    }
}

} // namespace NJoker
