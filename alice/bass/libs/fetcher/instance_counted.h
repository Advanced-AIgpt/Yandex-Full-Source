#pragma once

#include <util/generic/singleton.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/types.h>

template <class T>
class TInstanceCounted {
public:
    TInstanceCounted()
        : InstanceId(Singleton<TInstanceCounter>()->GetNextId())
    {
    }

    ui64 GetInstanceId() const {
        return InstanceId;
    }

private:
    class TInstanceCounter {
    public:
        TInstanceCounter() {
            AtomicSet(NextId, 0);
        }

        ui64 GetNextId() {
            return AtomicGetAndIncrement(NextId);
        }

    private:
        TAtomic NextId;
    };

private:
    const ui64 InstanceId;
};
