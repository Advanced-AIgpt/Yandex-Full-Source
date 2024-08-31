#pragma once

#include "request_parts.h"

#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/request_composite/client/client.h>

namespace NAlice {

template <typename... TComponents>
class TSpeechKitRequestView : public NMegamind::TRequestComponentsView<TComponents...> {
public:
    class TCompositeHolder {
    public:
        virtual ~TCompositeHolder() = default;

        virtual TSpeechKitRequestView<TComponents...> View() const = 0;
    };
    using TCompositeHolderPtr = std::unique_ptr<TCompositeHolder>;

public:
    using NMegamind::TRequestComponentsView<TComponents...>::TRequestComponentsView;
};

using TSpeechKitRequest = TSpeechKitRequestView<NMegamind::TEventComponent, NMegamind::TRequestParts, NMegamind::TClientComponent>;

} // namespace NAlice
