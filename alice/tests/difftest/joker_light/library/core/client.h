#pragma once

#include "context.h"

#include <library/cpp/http/server/http.h>

namespace NAlice::NJokerLight {

class TClient : public TRequestReplier {
public:
    TClient(TContext& context);

    bool DoReply(const TReplyParams& params) override;

private:
    TContext& Context_;
};

} // namespace NAlice::NJokerLight
