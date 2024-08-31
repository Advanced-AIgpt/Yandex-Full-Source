#pragma once

#include <util/generic/maybe.h>

namespace NAlice::NJokerLight {

class TContext;
class TSession;
class TError;
class TYdb;

using TStatus = TMaybe<TError>;

} // namespace NAlice::NJokerLight
