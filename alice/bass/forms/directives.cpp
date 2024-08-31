#include "directives.h"

#include <alice/bass/libs/analytics/analytics.h>

namespace NBASS {

#define CREATE_AND_REGISTER_DIRECTIVE(className, analyticsTag) \
REGISTER_DIRECTIVE(className, analyticsTag)

#include "directives.inc"

#undef CREATE_AND_REGISTER_DIRECTIVE

}
