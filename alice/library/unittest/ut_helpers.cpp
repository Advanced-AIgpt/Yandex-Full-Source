#include "ut_helpers.h"

namespace NTestingHelpers {

bool EqualJson(const NSc::TValue& expected, const NSc::TValue& actual) {
    if (NSc::TValue::Equal(expected, actual))
        return true;
    Cerr << "Expected: " << expected.ToJsonPretty() << Endl;
    Cerr << "Actual: " << actual.ToJsonPretty() << Endl;
    return false;
}

} // namespace NTestingHelpers
