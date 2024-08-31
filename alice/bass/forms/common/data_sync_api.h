#pragma once

#include "personal_data.h"

#include <alice/bass/util/error.h>

#include <util/generic/strbuf.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

namespace NBASS {

class TContext;

// A lightweight polymorphic wrapper for TPersonalDataHelper, mostly
// needed to mock communications with DataSync in tests.
struct TDataSyncAPI {
public:
    using EKey = TPersonalDataHelper::EKey;
    using TAsyncResult = std::variant<TError, TString>;
    using TAsyncGetter = std::function<TAsyncResult()>;
    using TKeyValue = TPersonalDataHelper::TKeyValue;

    virtual ~TDataSyncAPI() = default;

    virtual TResultValue Save(TContext& context, TStringBuf uid, const TVector<TKeyValue>& kvs);

    virtual TResultValue Get(TContext& context, TStringBuf uid, EKey key, TString& value);

    virtual TAsyncGetter GetAsync(TContext& context, TStringBuf uid, EKey key, const TDuration& additionalTimeout);
};

// Mocked version of TDataSyncAPI
struct TDataSyncAPIStub : public TDataSyncAPI {
    TResultValue Save(TContext& /* context */, TStringBuf uid, const TVector<TKeyValue>& kvs) override;

    TResultValue Save(TStringBuf uid, const TVector<TKeyValue>& kvs);

    TResultValue Get(TContext& /* context */, TStringBuf uid, EKey key, TString& value) override;

    TDataSyncAPI::TAsyncGetter GetAsync(TContext& context, TStringBuf uid, EKey key,
                                        const TDuration& /* additionalTimeout */) override;

    void FailOnGet(TStringBuf uid, EKey key);

    THashMap<TString, THashMap<TString, TString>> Data;
    THashMap<TString, THashSet<EKey>> ErrorUidKeys;
    size_t NumGetCalled = 0;
};
} // namespace NBASS
