#pragma once

#include <alice/megamind/protos/common/events.pb.h>

#include <alice/megamind/library/request/event/event.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

struct TEventComponent {
    using TEventProto = NAlice::TEvent;
    using TEventProtoPtr = TSimpleSharedPtr<TEventProto>;
    using TEventProtoConstPtr = TSimpleSharedPtr<const NAlice::TEvent>;
    using TEventWrapper = TSimpleSharedPtr<const NAlice::IEvent>;

    class TView {
    public:
        explicit TView(const TEventComponent& event)
            : Event_{event}
        {
        }

        const TEventProto& Event() const {
            return Event_.Event();
        }

        TEventWrapper EventWrapper() const {
            return Event_.EventWrapper();
        }

        bool HasEvent() const;

        TMaybe<bool> IsEOU() const;

    private:
        const TEventComponent& Event_;
    };

    virtual ~TEventComponent() = default;

    virtual const TEventProto& Event() const = 0;
    virtual TEventWrapper EventWrapper() const = 0;
};

} // namespace NAlice::NMegamind
