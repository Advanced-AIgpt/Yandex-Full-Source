#pragma once

#include <alice/joker/library/status/status.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/buffer.h>
#include <util/generic/yexception.h>
#include <util/stream/fwd.h>

namespace NAlice::NJoker {

class IS3Storage {
public:
    struct TCredentials {
        TString Id;
        TString Secret;
    };

    struct TBucketCheckException : public yexception {
    };

public:
    virtual ~IS3Storage() = default;

    virtual TStatus Get(TStringBuf key, IOutputStream& out) = 0;
    virtual TStatus Put(TStringBuf key, TBuffer buf) = 0;
    virtual TStatus Delete(TStringBuf key) = 0;

    static THolder<IS3Storage> Create(TStringBuf host, TStringBuf bucket, TDuration timeout, const TMaybe<TCredentials>& credentials);
};

} // namespace NJoker
