#pragma once

#include "vins.h"

namespace NBASS {

/** This form is used in the default request json in UnitTests and
 * does absolutelly nothing.
 * @see NTestingHelpers::TRequestJson
 */
class TUnitTestFormHandler : public IHandler {
public:
    static const TStringBuf DEFAULT_FORM_NAME;

public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace
