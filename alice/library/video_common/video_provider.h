#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NVideoCommon {

bool IsInternetVideoProvider(const TStringBuf provider);

bool IsPaidProvider(const TStringBuf provider);

bool IsDeprecatedProvider(const TStringBuf provider);

} // namespace NAlice::NVideoCommon
