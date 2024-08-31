#include "globalctx.h"

#include "http_session.h"

#include <alice/joker/library/log/log.h>

#include <util/stream/file.h>
#include <util/stream/output.h>

namespace NAlice::NJoker {

TGlobalContext::TGlobalContext(const TString& configFileName)
    : Config_{TFileInput{configFileName}.ReadAll(), {}}
    , RequestsHistory_{Config_.RequestsHistorySize()}
{
}

TGlobalContext::~TGlobalContext() = default;

TMemoryStorage& TGlobalContext::MemoryStorage() {
    return MemoryStorage_;
}

TRequestsHistory& TGlobalContext::RequestsHistory() {
    return RequestsHistory_;
}

} // namespace NJoker
