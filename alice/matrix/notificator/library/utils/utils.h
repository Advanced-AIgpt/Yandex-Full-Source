#pragma once

#include <infra/libs/outcome/result.h>

namespace NMatrix::NNotificator {

TExpected<ui64, TString> TryParseFromString(const TString& st, const TString& name);

} // namespace NMatrix::NNotificator
