#pragma once

#include "flags.h"

#include <library/cpp/json/json_value.h>

#include <unordered_set>

namespace NAlice::NUniproxy {
    using namespace NJson;

    typedef std::unordered_set<TString> TFlags;

    class TFlagsContainer {
    protected:
        TFlags Flags_ = {DUMMY_FLAG};

    protected:
        void ReduceInversions();

    public:
        TFlagsContainer() = default;
        explicit TFlagsContainer(const TString& flags);
        explicit TFlagsContainer(const TJsonValue& flags);
        explicit TFlagsContainer(const TFlags& flags);
        bool Has(const TString& flag) const;
        TString ToString() const;
    };
}
