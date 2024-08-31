#pragma once

#include <util/generic/string.h>
#include <library/cpp/yson/node/node.h>

namespace NAlice {

TString GetIntentName(const NYT::TNode& analyticsInfo, const TString& formName);

}; // namespace NAlice
