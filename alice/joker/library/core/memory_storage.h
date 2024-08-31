#pragma once

#include <alice/joker/library/stub/stub.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/hash.h>
#include <util/system/spinlock.h>

namespace NAlice::NJoker {

class TMemoryStorage {
public:
    using TFutureResult = NThreading::TFuture<TStubItemPtr>;
    using TOnNotFound = std::function<TStubItemPtr()>;
    using TOnEachStub = std::function<void(const TString&, TFutureResult::value_type)>;

public:
    ~TMemoryStorage();

    TFutureResult Get(TStringBuf sessionId, TStringBuf key, TOnNotFound onNotFound);

    void ForEach(TStringBuf sessionId, TOnEachStub onEachStub);

private:
    using TPromiseResult = NThreading::TPromise<TFutureResult::value_type>;

    TAdaptiveLock Lock_;
    THashMap<TString, THashMap<TString, TPromiseResult>> SessionStubs_;
};

} // namespace NJoker
