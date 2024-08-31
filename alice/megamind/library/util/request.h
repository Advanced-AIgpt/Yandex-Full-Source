#pragma once

#include <util/generic/string.h>


namespace NAlice {
namespace NMegamind {

struct TDialogIdSplit {
    TString ScenarioName;
    TString ScenarioDialogId;
};

TDialogIdSplit SplitDialogId(const TString& dialogId);

} // namespace NMegamind
} // namespace NAlice
