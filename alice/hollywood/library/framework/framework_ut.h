#pragma once

//
// HOLLYWOOD FRAMEWORK ENTRY POINT (Unit tests)
//

#include "framework.h"

#include <alice/hollywood/library/framework/unittest/test_environment.h>
#include <alice/hollywood/library/framework/unittest/test_nodes.h>

namespace NAlice::NHollywoodFw {

// Initialize scenario, declared as local variable instead of HW_REGISTER()
extern void InitializeLocalScenario(TScenario& localScenarioVar);

} // namespace NAlice::NHollywoodFw
