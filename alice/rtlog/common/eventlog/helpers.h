#pragma once

#include <library/cpp/eventlog/eventlog.h>

#include <util/generic/cast.h>

namespace NRTLog {
    template<typename TProto>
    const TProto& Cast(const TEvent* e) {
        return *VerifyDynamicCast<const TProto*>(e->GetProto());
    }
}
