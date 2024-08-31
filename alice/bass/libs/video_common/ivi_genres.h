#pragma once

#include "utils.h"

#include <alice/bass/libs/fetcher/request.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/system/rwlock.h>
#include <util/system/types.h>

namespace NVideoCommon {

class ISourceRequestFactory;

// This class *IS* thread-safe.
class TIviGenres : NNonCopyable::TNonCopyable {
public:
    // This class *IS* thead-safe.
    struct TDelegate {
        virtual ~TDelegate() = default;
        virtual THolder<NHttpFetcher::TRequest> MakeRequest(TStringBuf path) = 0;
    };

    explicit TIviGenres(TDelegate& delegate);
    virtual ~TIviGenres() = default;

    virtual TMaybe<TString> GetIviGenreById(ui32 id);
    virtual bool Update();
    virtual void Clear();

private:
    TDelegate& Delegate;
    THashMap<ui32, TString> Data;
    TRWMutex Mutex;
};

class TAutoUpdateIviGenres : public TIviGenres {
public:
    explicit TAutoUpdateIviGenres(TIviGenres::TDelegate& delegate)
        : TIviGenres(delegate) {
    }

    // TIviGenres overrides:
    TMaybe<TString> GetIviGenreById(ui32 id) override;
    bool Update() override;

private:
    TMaybe<TTimePoint> LastUpdate;
    TRWMutex Mutex;
};

} // NVideoCommon
