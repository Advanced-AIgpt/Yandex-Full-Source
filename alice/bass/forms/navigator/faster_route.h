#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TNavigatorFasterRouteHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

private:
    static constexpr TStringBuf NAVI_FASTER_ROUTE = "personal_assistant.navi.faster_route";
    static constexpr TStringBuf NAVI_FASTER_ROUTE_ELLIPSIS = "personal_assistant.navi.faster_route__ellipsis";
};

}
