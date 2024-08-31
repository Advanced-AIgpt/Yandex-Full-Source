#include "util.h"

#include <util/string/builder.h>

#include <algorithm>
#include <array>

namespace {

const std::array NUMS_2_3_4{2, 3, 4};
const std::array NUMS_12_13_14{12, 13, 14};

TString TimeToStr(ui32 timeSec, bool at, bool when) {

    auto isIn = [](int n, const auto& nums) {
        return std::find(begin(nums), end(nums), n) != end(nums);
    };

    TStringBuilder result;
    if (timeSec == 0) {
        if (at) {
            result << "самом начале";
        } else if (when) {
            result << "самое начало";
        } else {
            result << "0 секунд";
        }
        return result;
    }

    const auto minutes = timeSec / 60;
    if (timeSec >= 60) {
        result << minutes;
        if (at) {
            result << " минуте";
        } else if (when) {
            result << " минуту";
        } else if (minutes % 10 == 1) {
            result << " минута";
        } else if (isIn(minutes % 10, NUMS_2_3_4) && !isIn(minutes % 100, NUMS_12_13_14)) {
            result << " минуты";
        } else {
            result << " минут";
        }
    }

    const auto secondsRem = timeSec % 60;
    if (secondsRem != 0) {
        if (minutes != 0) {
            result << " ";
        }
        result << secondsRem;
        if (at) {
            result << " секунде";
        } else if (when) {
            result << " секунду";
        } else if (secondsRem % 10 == 1 && secondsRem / 10 != 1) {
            result << " секунда";
        } else if (isIn(secondsRem % 10, NUMS_2_3_4) && !isIn(secondsRem % 100, NUMS_12_13_14)) {
            result << " секунды";
        } else {
            result << " секунд";
        }
    }
    return result;
}

} // namespace

namespace NAlice::NHollywood {

TString TimeAmountToStr(ui32 timeSec) {
    return TimeToStr(timeSec, /* at = */ false, /* when = */ false);
}

TString TimePointWhenToStr(ui32 timeSec) {
    return TimeToStr(timeSec, /* at = */ false, /* when = */ true);
}

TString TimePointAtToStr(ui32 timeSec) {
    return TimeToStr(timeSec, /* at = */ true, /* when = */ false);
}

} // NAlice::NHollywood
