#pragma once

#include <alice/bass/libs/analytics/analytics.h>

namespace NBASS {

#define CREATE_AND_REGISTER_DIRECTIVE(className, analyticsTag) \
class className : public IDirective { \
};

#include "directives.inc"

#undef CREATE_AND_REGISTER_DIRECTIVE

} // NBASS
