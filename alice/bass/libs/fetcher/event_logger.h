#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/system/types.h>

namespace NHttpFetcher {

class IEventLogger : public TThrRefBase {
public:
    struct TNehId {
        TNehId(ui64 reqId)
            : Id{reqId}
        {
        }

        TNehId(ui64 reqId, TStringBuf name)
            : Id{reqId}, Name{name}
        {
        }

        ui64 Id;
        TStringBuf Name;

        TString AsString() const;
    };
public:
    virtual ~IEventLogger() = default;

    void OnAttemptRegistered(TNehId requestId, ui64 multiFetcherId, TStringBuf address) const;

    void OnAttemptStarted(TNehId requestId, ui32 attemptId, ui32 maxAttempts) const;

    void OnAttemptSuccess(TNehId requestId, ui32 attemptId, const TDuration& duration) const;
    void OnAttemptError(TNehId requestId, ui32 attemptId, TStringBuf data) const;
    void OnAttemptError(TNehId requestId, ui32 attemptId, const TDuration& duration, TStringBuf error) const;

    void OnAbandonedRequest(ui64 requestId) const;

protected:
    virtual void Error(TStringBuf msg) const = 0;
    virtual void Debug(TStringBuf msg) const = 0;
};

using IEventLoggerPtr = TIntrusivePtr<IEventLogger>;

class TBassEventLogger : public IEventLogger {
protected:
    // IEventLogger overrides:
    void Error(TStringBuf msg) const override;
    void Debug(TStringBuf msg) const override;
};

} // namespace NHttpFetcher
