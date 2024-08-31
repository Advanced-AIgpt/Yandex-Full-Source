#pragma once

namespace NAlice::NHollywood::NWeather {

enum class ERequestPhase {
    Before,
    After,
};

template<ERequestPhase RequestPhase, typename TBeforeHelper, typename TAfterHelper>
using TRequestHelperChooser = std::conditional_t<RequestPhase == ERequestPhase::Before, TBeforeHelper, TAfterHelper>;

} // NAlice::NHollywood::NWeather
