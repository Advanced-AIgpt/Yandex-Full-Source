#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TNavigatorHowLongHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    static TResultValue SetAsResponse(TContext& ctx, TStringBuf formName = NAVI_HOW_LONG_DRIVE);

    static void Register(THandlersMap* handlers);

private:
    static constexpr TStringBuf NAVI_WHEN_WE_GET = "personal_assistant.navi.when_we_get_there";
    static constexpr TStringBuf NAVI_HOW_LONG_DRIVE = "personal_assistant.navi.how_long_to_drive";
    static constexpr TStringBuf NAVI_HOW_LONG_TRAFFIC_JAM = "personal_assistant.navi.how_long_traffic_jam";
};

}
