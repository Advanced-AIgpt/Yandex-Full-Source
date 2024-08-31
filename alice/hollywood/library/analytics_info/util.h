#pragma once

#include <util/generic/string.h>

namespace NAlice::NHollywood {

// Use it in cases like "До таймера осталось '2 минуты 11 секунд'{time amount}"
TString TimeAmountToStr(ui32 timeSec);

// Use it in cases like "Перемотай на '2 минуту 11 секунду'{time point when}"
TString TimePointWhenToStr(ui32 timeSec);

// Use it in cases like "Игра окончилась на '2 минуте 11 секунде'{time point at}"
TString TimePointAtToStr(ui32 timeSec);

} // NAlice::NHollywood
