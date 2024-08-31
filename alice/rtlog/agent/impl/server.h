#pragma once

#include <util/generic/ptr.h>

namespace NRTLogAgent {
    class TConfig;

    class IServer: public TAtomicRefCount<IServer> {
    public:
        virtual ~IServer() = default;

        virtual void Start() = 0;

        virtual void Stop() = 0;
    };

    TIntrusivePtr<IServer> MakeServer(const TConfig& config);
}
