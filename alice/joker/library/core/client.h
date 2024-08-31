#pragma once

#include <library/cpp/http/server/http.h>

namespace NAlice::NJoker {

class TGlobalContext;

class TClient : public TRequestReplier {
public:
    explicit TClient(TGlobalContext& globalCtx);

    bool DoReply(const TReplyParams& params) override;

private:
    TGlobalContext& GlobalCtx_;
};

} // namespace NJoker
