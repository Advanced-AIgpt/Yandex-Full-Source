#pragma once

#include "asr_hypothesis_picker.h"
#include "util.h"

namespace NAlice {

bool IsIoTCommand(const TAsrHypothesis& asrHypothesis, const TVector<TString>& userIoTScenariosNamesAndTriggers = {});

bool IsItemSelectorCommand(const TAsrHypothesis& asrHypothesis);

bool IsArithmeticsCommand(const TAsrHypothesis& asrHypothesis);

bool IsShortCommand(const TAsrHypothesis& asrHypothesis);

}  // namespace NAlice
