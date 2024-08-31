#pragma once

#include "session.h"
#include "globalctx.h"

#include <util/folder/path.h>
#include <util/generic/maybe.h>

namespace NAlice::NJoker {

class TSessionControl final : public TSession {
public:
    static TStatus Load(const TSessionId& sessionId, const TConfig& config, THolder<TSessionControl>& session);

public:
    TStatus Push(TGlobalContext& globalCtx, IOutputStream& outputStream = Cout);
    TStatus Clear();
    TStatus OutTo(IOutputStream& out) const;

private:
    TSessionControl(TSessionId id, const TConfig& config, const TString& runId, TInstant startedAt, TFsPath currentRunDirectory);

private:
    const TFsPath CurrentRunLogDirectory_;
};

} // namespace NAlice::NJoker
