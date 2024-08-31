#pragma once

#include <cstddef>


namespace NCachalot {

struct TActivationServiceConfig {
};

struct TActivationOperationOptions {
public:
    bool IgnoreRms = false;

public:
    size_t CombineFlags() const;
};

}   // namespace NCachalot
