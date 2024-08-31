#include "context.h"

#include <alice/joker/library/log/log.h>

namespace NAlice::NJokerLight {

TContext::TContext(const TFsPath& configFilePath)
    : Config_{configFilePath}
    , Ydb_{*this}
{
}

} // namespace NAlice::NJokerLight
