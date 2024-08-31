#pragma once

#include "fm_radio_resources.h"

#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NMusic {

i32 ChooseSupportedRegionId(
    const TScenarioRunRequestWrapper& request,
    const TFmRadioResources& fmRadioResources);

i32 GetRegionId(
    const TScenarioRunRequestWrapper& request,
    const TFmRadioResources& fmRadioResources);

TMaybe<TString> GetFmRadioByFreq(
    const TScenarioRunRequestWrapper& request,
    const double radioFreq,
    const TFmRadioResources& fmRadioResources);

TMaybe<TString> GetFmRadioName(
    const TScenarioRunRequestWrapper& request,
    bool isFmRadio,
    const TPtrWrapper<TSlot>& slotFmRadio,
    const TPtrWrapper<TSlot>& slotFmRadioFreq,
    const TFmRadioResources& fmRadioResources);

TMaybe<TString> GetFmRadioId(
    const TMaybe<TString>& namedFmRadioStation,
    const TFmRadioResources& fmRadioResources);

} // namespace NAlice::NHollywood::NMusic
