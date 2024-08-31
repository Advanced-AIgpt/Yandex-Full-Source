#pragma once

#include <util/generic/string.h>


namespace NCachalot {


struct TStorageStats {
    float SchedulingTime { 0.0 };
    float FetchingTime { 0.0 };
    TString ErrorMessage;
};


}   // namespace NCachalot
