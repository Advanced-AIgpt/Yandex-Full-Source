#include "common.h"

TTestFixture::TTestFixture()
    : AppHostContext(MakeIntrusive<NAppHost::NService::TTestContext>())
{
}

bool TTestFixture::HasFlag(const TStringBuf flag) {
    return AppHostContext->GetFlags().contains(flag);
}
