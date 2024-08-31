#pragma once

#include <alice/cuttlefish/library/digest/murmur.h>


namespace NSM {


struct TEventId {
    constexpr TEventId() = default;

    constexpr TEventId(const char *name) : EventId(NTL::MurmurHash64(name)) { }

    constexpr TEventId(uint64_t id) : EventId(id) { }

    constexpr TEventId(const TEventId&) = default;

    constexpr TEventId& operator= (const TEventId&) = default;


    constexpr TEventId& operator= (const char* name) {
        EventId = NTL::MurmurHash64(name);
        return *this;
    }


    constexpr TEventId& operator= (uint64_t id) {
        EventId = id;
        return *this;
    }

    constexpr bool operator==(const TEventId& other) const {
        return EventId == other.EventId;
    }

    constexpr bool operator==(uint64_t id) const {
        return EventId == id;
    }

private:
    uint64_t EventId { 0 };
};


struct TEventBase {
    constexpr TEventBase(TEventId id)
        : EventId(id)
    { }

    constexpr TEventBase(const char *name)
        : EventId(name)
    { }

    constexpr TEventBase(const TEventBase&) = default;


    constexpr TEventBase& operator=(const TEventBase&) = default;


    template <class T>
    const T* As() const {
        if (EventId == T::Id) {
            return reinterpret_cast<const T*>(this);
        }
        return nullptr;
    }


    constexpr bool Is(const TEventId id) const {
        return id == EventId;
    }


    constexpr bool Is(const char *name) const {
        return TEventId(name) == EventId;
    }


    template <class T>
    constexpr bool Is() const {
        return Is(T::Id);
    }

protected:
    TEventId EventId;
};


#define NSM_EVENT(x) \
    static constexpr const char* Name = x; \
    static constexpr const uint64_t Id = NTL::MurmurHash64(x);


template <class T>
struct TEvent : TEventBase {
    constexpr TEvent() : TEventBase(T::Id) { }

    const T* Get() const {
        return TEventBase::As<T>();
    }
};


struct TStartEv : public TEvent<TStartEv> {
    NSM_EVENT("ev.event.start");
};


struct TShutdownEv : public TEvent<TShutdownEv> {
    NSM_EVENT("ev.event.shutdown");
};


}   // namespace NSM
