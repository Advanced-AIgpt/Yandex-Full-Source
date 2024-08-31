#include "common.h"

TTestFixture::TTestFixture()
    : AppHostContext(MakeIntrusive<NAppHost::NService::TTestContext>())
{
}
