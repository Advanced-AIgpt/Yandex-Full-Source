#pragma once

#include <alice/joker/library/core/request.h>

#include <library/cpp/cache/cache.h>

#include <util/system/rwlock.h>

namespace NAlice::NJoker {

class TRequestsHistory {
public:
    struct TRequestEntry : public TAtomicRefCount<TRequestEntry> {
        TString Action;
        THashMap<TString, TString> Query;
        THashMap<TString, TString> Headers;
        TString Body;
    };
    using TRequestEntries = TVector<TIntrusivePtr<TRequestEntry>>;

public:
    TRequestsHistory(size_t maxSize);

    void Add(const TString& groupId, const IHttpContext& ctx);
    TMaybe<TRequestEntries> Get(const TString& groupId);

    static TString ToJson(const TRequestsHistory::TRequestEntries& history);

private:
    using TStorage = TLRUCache<TString, TRequestEntries>;

private:
    TStorage Storage_;
    TRWMutex Lock_;
};

} // namespace NJoker
